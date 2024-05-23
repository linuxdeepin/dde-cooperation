// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileclient.h"
#include "tokencache.h"

#include "string/string_utils.h"
#include "filesystem/file.h"
#include "filesystem/path.h"
#include "filesystem/directory.h"

#include <iostream>
#include <regex>

using ResponseHandler = std::function<bool(int status, const char *buffer, size_t size)>;

using CppServer::HTTP::HTTPRequest;
using CppServer::HTTP::HTTPResponse;

enum HandleResult {
    RES_OKHEADER = 200,
    RES_NOTFOUND = 404,
    RES_ERROR = 444,
    RES_BODY = 555,
    RES_FINISH = 666,
};

class HTTPFileClient : public CppServer::HTTP::HTTPClientEx
{
public:
    using CppServer::HTTP::HTTPClientEx::HTTPClientEx;

    void setResponseHandler(ResponseHandler cb)
    {
        _handler = std::move(cb);
    }

protected:
    void onReceivedResponseHeader(const HTTPResponse &response) override
    {
//        std::cout << "Response status: " << response.status() << std::endl;
//        std::cout << "Response header body len: " << response.body_length() << std::endl;
//        std::cout << "------------------" << std::endl;
//        std::cout << "Response header Content: \n" << response.string() << std::endl;
//        std::cout << "------------------" << std::endl;

        if (_handler) {
            // get body by stream, so mark response arrived.
            HTTPClientEx::onReceivedResponse(response);

            if (_handler(response.status() == 200 ? RES_OKHEADER : RES_NOTFOUND, response.string().data(), response.body_length())) {
                // cancel
                DisconnectAsync();
            }
            _response.ClearCache();
        }
    }

    // it has been marked response arrived when geting its header. Mark all done and clear
    void onReceivedResponse(const HTTPResponse &response) override
    {
        std::cout << "Response done! data len:" << response.body_length() << std::endl;

        if (_handler) {
            std::string cache = response.cache();
            size_t size = cache.size();
            if (_handler(RES_FINISH, cache.data(), size)) {
                // cancel
                DisconnectAsync();
            }
            _response.Clear();
            // donot disconnect at here, this connection may be continue to downlad other.
        } else {
            HTTPClientEx::onReceivedResponse(response);
        }
    }

    bool onReceivedResponseBody(const HTTPResponse &response) override
    {
//        std::cout << "Response BODY cache: " << response.cache().size() << std::endl;
        if (_handler) {
            std::string cache = response.cache();
            size_t size = cache.size();
            if (_handler(RES_BODY, cache.data(), size)) {
                // cancel
                DisconnectAsync();
            }
            _response.ClearCache();
        }

        return true;
    }

    void onReceivedResponseError(const HTTPResponse &response, const std::string &error) override
    {
        std::cout << "Response error: " << error << std::endl;
        if (_handler) {
            _handler(RES_ERROR, nullptr, 0);
        }
    }

private:
    ResponseHandler _handler { nullptr };
};

FileClient::FileClient(const std::shared_ptr<CppServer::Asio::Service> &service, const std::string &address, int port)
{
    if (_httpClient) {
        //discontect current one
        _httpClient->DisconnectAsync();
    }
    // Create HTTP client if the current one is empty
    _httpClient = std::make_shared<HTTPFileClient>(service, address, port);

    //_httpClient->IsConnected() ? _httpClient->ReconnectAsync() : _httpClient->ConnectAsync();
}

FileClient::~FileClient()
{
    if (_httpClient) {
        _httpClient->Disconnect();
        _httpClient = nullptr;
    }
}

std::vector<std::string> FileClient::parseWeb(const std::string &token)
{
    return TokenCache::GetInstance().getWebfromToken(token);
}

void FileClient::setConfig(const std::string &token, const std::string &savedir)
{
    _token = token;
    _savedir = savedir;
}

void FileClient::cancel(bool cancel)
{
    _cancel = cancel;
//    _running = !_cancel;
}

bool FileClient::downloading()
{
//    return _httpClient->IsConnected();
    return _running;
}

void FileClient::startFileDownload(const std::vector<std::string> &webnames)
{
    if (_running) {
        std::cout << "there is thread downloading..";
        return;
    }
    if (_token.empty() || _savedir.empty()) {
        std::cout << "Must setConfig first!" << std::endl;
        return;
    }

    _cancel = false;
    _running = true;

    // 创建新线程来运行walkDownload函数
    std::thread downloadThread(&FileClient::walkDownload, this, webnames);
    downloadThread.detach(); // 由于线程函数是成员函数，这里用detach分离线程
}

//-------------private-----------------

InfoEntry FileClient::requestInfo(const std::string &name)
{
    InfoEntry fileInfo;
//    std::vector<std::pair<std::string, int64_t>> infos;
    if (_token.empty()) {
        std::cout << "Must set access token!" << std::endl;
        return fileInfo;
    }

    // request the dir/file info
    // [GET]info/<name>&token
    std::string url = "info/";
    url.append(name);
    url.append("&token=").append(_token);

    _httpClient->setResponseHandler(nullptr);

    auto response = _httpClient->SendGetRequest(url).get();
    if (response.status() == 404) {
        return fileInfo;
    }
    std::string body_str = response.body().data();

    picojson::value v;
    std::string err = picojson::parse(v, body_str);
    if (!err.empty()) {
        std::cout << "Failed to parse JSON data: " << err << std::endl;
        return fileInfo;
    }

    fileInfo.from_json(v);
    return fileInfo;
}

std::string FileClient::getHeadKey(const std::string &headstrs, const std::string &keyfind)
{
    std::unordered_map<std::string, std::string> headermap;
    std::stringstream ss(headstrs);

    std::string line;
    while (std::getline(ss, line, '\n')) {
        size_t delimiterPos = line.find(':');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);
            // 移除末尾的空格
            key.erase(key.find_last_not_of(" \n\r\t") + 1);
            value.erase(0, value.find_first_not_of(" \n\r\t"));

            headermap[key] = value;
        }
    }
    std::string value = headermap[keyfind];
    return value;
}

// download the file by name
// [GET]download/<name>&token
bool FileClient::downloadFile(const std::string &name)
{
    if (_savedir.empty()) {
        std::cout << "Must set save dir first!" << std::endl;
        return false;
    }
    if (_token.empty()) {
        std::cout << "Must set access token!" << std::endl;
        return false;
    }

    std::promise<bool> exitPromise;  // 创建一个 promise 用于给 cb 函数设置标志
    std::future<bool> exitFuture = exitPromise.get_future();  // 获取与 promise 关联的 future

    uint64_t current = 0, total = 0;
    uint64_t offset = 0;
    std::string url = "download/";
    url.append(name);

    CppCommon::Path savepath = _savedir + name;
    CppCommon::Directory::CreateTree(savepath.parent());
    tempFile = CppCommon::File(savepath + ".part");
    if (tempFile.IsFileExists()) {
        // get current file lenght and continue download.
        offset = tempFile.size();
        std::cout << "tempFile exist size=: " << offset << std::endl;
    } else {
        CppCommon::File::WriteEmpty(tempFile);
        std::cout << "tempFile created: " << offset << std::endl;
    }
    std::cout << "tempFile: " << tempFile.absolute().string() << std::endl;


    url.append("&token=").append(_token);
    url.append("&offset=").append(std::to_string(offset));

    ResponseHandler cb([this, &current, &total, &offset, &exitPromise](int status, const char *buffer, size_t size) -> bool {
        bool shouldExit = false;
        switch (status)
        {
        case RES_NOTFOUND: {
            std::cout << "File not Found!" << std::endl;
            if (tempFile.IsFileWriteOpened()) {
                tempFile.Close();
            }
            // error：not found
            total = -1;
            shouldExit = true;
        }
        break;
        case RES_OKHEADER: {
            std::string flag = this->getHeadKey(buffer, "Flag");
            std::cout << "head flag: " << flag << std::endl;

            if (!tempFile.IsFileWriteOpened()) {
                size_t cur_off = 0;
                if (tempFile.IsFileExists()) {
                    cur_off = tempFile.size();
                    std::cout << "Exists seek=: " << offset  << " cur_off=" << cur_off << std::endl;
                }
                tempFile.OpenOrCreate(false, true, true);

                // set offset and current size
                tempFile.Seek(cur_off);
            }
            total = size;
        }
        break;
        case RES_BODY: {
            if (tempFile.IsFileWriteOpened() && buffer && size > 0) {
                current += size;
                // 实现层已循环写全部
                size_t wr = tempFile.Write(buffer, size);
                if (wr != size)
                    std::cout << "!!!!!!!!! write offset=: " << tempFile.offset() << " size=" << size << " wr=" << wr << std::endl;
            }
        }
        break;
        case RES_ERROR: {
            if (tempFile.IsFileWriteOpened())
                tempFile.Close();
            // error：break off
            current = -1;
            shouldExit = true;
        }
        break;
        case RES_FINISH: {
            if (tempFile.IsFileWriteOpened()) {
                current += size;
                // 写入最后一块
                tempFile.Write(buffer, size);
//                tempFile.Flush();
                tempFile.Close();
            }
            std::cout << "RES_FINISH, current=" << current << " total:" << total << std::endl;

            // rename only for finished
            if (current >= total) {
                std::cout << "File path:" << tempFile.absolute().RemoveExtension() << std::endl;

                CppCommon::Path::Rename(tempFile.absolute(), tempFile.absolute().RemoveExtension());
                tempFile.Clear();
            }
            shouldExit = true;
        }
        break;

        default:
            std::cout << "error, unkonw status=" << status << std::endl;
            break;
        }

        bool cancel = false;
        // notify downloading：current total
        if (_callback)
        {
            // return true to cancel download from outside.
            cancel = _callback->onProgress(tempFile.string(), current, total);
        }
        if (cancel || shouldExit) {
            exitPromise.set_value(true);
        }

        return cancel;
    });

    _httpClient->setResponseHandler(std::move(cb));
    auto res = _httpClient->SendGetRequest(url).get(); // use get to sync download one by one

    exitFuture.get();  // 等待下载完成

    return true;
}

void FileClient::downloadFolder(const std::string &folderName)
{
    // request get sub files's info
    InfoEntry info = requestInfo(folderName);
    if (info.datas.empty()) {
        std::cout << folderName << " downloadFolder requestInfo return NULL! " << std::endl;
        return;
    }

    // 循环处理文件夹内文件
    for (const auto &entry : info.datas) {
        if (_cancel)
            break;

        std::string name = entry.name;
        int64_t size = entry.size;
        if (size == 0) {
            // 未初始化的文件类型，无效信息
            std::cout << "get uninit file! " << name << std::endl;
            return;
        }

        std::string relName = folderName + "/" + name;
        if (size > 0) {
            downloadFile(relName);
        } else {
            // 文件夹, 创建
            CppCommon::Path folderPath = CppCommon::Path(_savedir) / folderName;
            if (!folderPath.IsExists() && folderPath.IsDirectory())
                CppCommon::Directory::CreateTree(folderPath);
            downloadFolder(relName); // 递归下载文件夹
        }
    }
}

void FileClient::walkDownload(const std::vector<std::string> &webnames)
{
    for (const auto& name : webnames) {
        std::cout << "start download web: " << name << std::endl;

        // do not sure the file type: floder or file
        InfoEntry info = requestInfo(name);
        if (info.size == 0) {
            std::cout << name << " walkDownload requestInfo return NULL! " << std::endl;
            return;
        }

        // file: size > 0; dir: size < 0; default size = 0
        if (info.size > 0) {
            downloadFile(name);
        } else {
            downloadFolder(name);
        }
    }
    _running = false;
    std::cout << "whole download finished!" << std::endl;
}
