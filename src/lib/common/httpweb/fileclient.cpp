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

using CppServer::HTTP::HTTPRequest;
using CppServer::HTTP::HTTPResponse;

class HTTPFileClient : public CppServer::HTTP::HTTPSClientEx
{
public:
    using CppServer::HTTP::HTTPSClientEx::HTTPSClientEx;

    void setResponseHandler(ResponseHandler cb)
    {
        _handler = std::move(cb);
    }

protected:
    void onConnected() override
    {
        // must invoke supper fun, or cause canot connect server.
        HTTPSClientEx::onConnected();
    }

    void onDisconnected() override
    {
        HTTPSClientEx::onDisconnected();
    }

    void onReceivedResponseHeader(const HTTPResponse &response) override
    {
//        std::cout << "Response status: " << response.status() << std::endl;
//        std::cout << "Response header body len: " << response.body_length() << std::endl;
//        std::cout << "------------------" << std::endl;
//        std::cout << "Response header Content: \n" << response.string() << std::endl;
//        std::cout << "------------------" << std::endl;

        if (_handler) {
            // get body by stream, so mark response arrived.
            HTTPSClientEx::onReceivedResponse(response);

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
        //std::cout << "Response done! data len:" << response.body_length() << std::endl;
        if (_canceled)
            return;

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
            HTTPSClientEx::onReceivedResponse(response);
        }
    }

    bool onReceivedResponseBody(const HTTPResponse &response) override
    {
//        std::cout << "Response BODY cache: " << response.cache().size() << std::endl;
        if (_handler) {
            std::string cache = response.cache();
            size_t size = cache.size();
            if (_handler(RES_BODY, cache.data(), size)) {
                _canceled = true;
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
    std::atomic<bool> _canceled { false };
};

FileClient::FileClient(const std::shared_ptr<CppServer::Asio::Service> &service, const std::shared_ptr<CppServer::Asio::SSLContext>& context, const std::string &address, int port)
{
    if (_httpClient) {
        //discontect current one
        _httpClient->DisconnectAsync();
    }
    // Create HTTP client if the current one is empty
    _httpClient = std::make_shared<HTTPFileClient>(service, context, address, port);
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

void FileClient::stop()
{
    _stop = true;
    _httpClient->DisconnectAsync();
}

void FileClient::startFileDownload(const std::vector<std::string> &webnames)
{
    if (_token.empty() || _savedir.empty()) {
        std::cout << "Must setConfig first!" << std::endl;
        return;
    }

    _stop = false;

    // 创建新线程来运行walkDownload函数
    std::thread downloadThread(&FileClient::walkDownload, this, webnames);
    downloadThread.detach(); // 由于线程函数是成员函数，这里用detach分离线程
}

//-------------private-----------------

// request the dir/file info
// [HEAD]webstart>|webfinish>|webindex><name>
void FileClient::sendInfobyHeader(uint8_t mask, const std::string &name)
{
    if (mask >= INFO_WEB_MAX) {
        return;
    }
    _httpClient->setResponseHandler(nullptr);

    std::string url = s_headerInfos[mask] + ">" + name;
    _httpClient->SendHeadRequest(url).get();
}

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
    // base64 the file name in order to keep original name, which may include '&'
    std::string ename = CppCommon::Encoding::Base64Encode(name);
    url.append(ename);
    url.append("&token=").append(_token);

    _httpClient->setResponseHandler(nullptr);
    // Must sleep for release something, or it will be blocked in request
    CppCommon::Thread::Yield();
    CppCommon::Thread::Sleep(1);

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
bool FileClient::downloadFile(const std::string &name, const std::string &rename)
{
    if (_savedir.empty()) {
        std::cout << "Must set save dir first!" << std::endl;
        return false;
    }
    if (_token.empty()) {
        std::cout << "Must set access token!" << std::endl;
        return false;
    }

    // use smart pointer in order to wait all free while client stop.
    std::shared_ptr<std::promise<bool>> exitPromisePtr = std::make_shared<std::promise<bool>>();
    std::future<bool> exitFuture = exitPromisePtr->get_future();  // 获取与 promise 关联的 future
    _promised.store(false, std::memory_order_relaxed); // reset flag

    uint64_t current = 0, total = 0;
    uint64_t offset = 0;
    std::string url = "download/";
    // base64 the file name in order to keep original name, which may include '&'
    std::string ename = CppCommon::Encoding::Base64Encode(name);
    url.append(ename);

    auto tempFile = CppCommon::File(createNextAvailableName(rename.empty() ? name : rename, true));
//    offset = tempFile.size();

    url.append("&token=").append(_token);
    url.append("&offset=").append(std::to_string(offset));

    ResponseHandler cb([exitPromisePtr, this, &current, &total, &tempFile](int status, const char *buffer, size_t size) -> bool {
        if (_stop) {
            std::cout << "has been canceled from outside!" << std::endl;
            exitPromisePtr->set_exception(std::make_exception_ptr(std::runtime_error("Download stopped")));
            return true;
        }

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
            _callback->onWebChanged(WEB_NOT_FOUND, tempFile.string());
        }
        break;
        case RES_OKHEADER: {
            //std::string flag = this->getHeadKey(buffer, "Flag");
            //std::cout << "head flag: " << flag << std::endl;
            CppCommon::Path file_path = tempFile.absolute().RemoveExtension();

            if (!tempFile.IsFileWriteOpened()) {
                size_t cur_off = 0;
                if (tempFile.IsFileExists()) {
                    cur_off = tempFile.size();
                    //std::cout << "Exists seek=: " << offset  << " cur_off=" << cur_off << std::endl;
                }
                tempFile.OpenOrCreate(false, true, true);

                // set offset and current size
                tempFile.Seek(cur_off);
            }
            total = size;
            _callback->onWebChanged(WEB_FILE_BEGIN, file_path.string(), total);
        }
        break;
        case RES_BODY: {
            if (tempFile.IsFileWriteOpened() && buffer && size > 0) {
                current += size;
                // 实现层已循环写全部
                tempFile.Write(buffer, size);

                shouldExit = _callback->onProgress(size);
            }
        }
        break;
        case RES_ERROR: {
            if (tempFile.IsFileWriteOpened())
                tempFile.Close();
            // error：break off
            current = -1;
            shouldExit = true;

            _callback->onWebChanged(WEB_IO_ERROR);
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
            //std::cout << "RES_FINISH, current=" << current << " total:" << total << std::endl;
//            CppCommon::Path file_path = tempFile.absolute().RemoveExtension();

//            // rename only for finished
//            if (current >= total) {
//                //std::cout << "File path:" << tempFile.absolute().RemoveExtension() << std::endl;

//                CppCommon::Path::Rename(tempFile.absolute(), file_path);
//                tempFile.Clear();
//            }

            shouldExit = true;
            _callback->onWebChanged(WEB_FILE_END, tempFile.string(), total);
            tempFile.Clear();
        }
        break;

        default:
            std::cout << "error, unkonw status=" << status << std::endl;
            break;
        }

        if (shouldExit && !_promised.load(std::memory_order_relaxed)) {
            exitPromisePtr->set_value(true);
            _promised.store(true, std::memory_order_relaxed);
        }

        return shouldExit;
    });

    _httpClient->setResponseHandler(std::move(cb));
    _httpClient->SendGetRequest(url).get(); // use get to sync download one by one

    bool result = false;
    try {
        result = exitFuture.get();  // 等待下载完成
    } catch (const std::exception &ex) {
        // 捕获并处理异常
        std::cout << "throw exception: " << ex.what() << std::endl;
    }
    // Must sleep for release something, or it will be blocked in request
    CppCommon::Thread::Yield();
    CppCommon::Thread::Sleep(1);
    return result;
}

void FileClient::downloadFolder(const std::string &foldername, const std::string &refoldername)
{
    // create a override folder
    if (refoldername.empty()) {
        CppCommon::Path path = CppCommon::Path(_savedir) / foldername;
        createNotExistPath(path.string(), false);
    }

    // request get sub files's info
    InfoEntry info = requestInfo(foldername);

    // 循环处理文件夹内文件
    for (const auto &entry : info.datas) {
        if (_stop)
            break;

        std::string relName = foldername + "/" + entry.name;
        std::string repName = refoldername + "/" + entry.name;
        int64_t size = entry.size;
        if (size > 0) {
            // replace with the new folder if need
            downloadFile(relName, refoldername.empty() ? "" : repName);
        } else if (size < 0) {
            downloadFolder(relName, refoldername.empty() ? "" : repName);
        } else {
            // 链接文件或大小为0的空文件
            createNextAvailableName(refoldername.empty() ? relName : repName, true);
        }
    }
}

void FileClient::walkDownload(const std::vector<std::string> &webnames)
{
    sendInfobyHeader(INFO_WEB_START);
    _callback->onWebChanged(WEB_TRANS_START);

    for (const auto& name : webnames) {
        //std::cout << "start download web: " << name << std::endl;

        sendInfobyHeader(INFO_WEB_INDEX, name);
        _callback->onWebChanged(WEB_INDEX_BEGIN, name);

        // do not sure the file type: floder or file
        InfoEntry info = requestInfo(name);
        if (info.size == 0) {
            std::cout << name << " walkDownload requestInfo return NULL! " << std::endl;
            continue;
        }

        // file: size > 0; dir: size < 0; default size = 0
        if (info.size > 0) {
            downloadFile(name);
        } else {
            // check and create the first index dir, ex. a -> /xx/download/a(1)
            std::string replacePath = createNextAvailableName(name, false);
            std::string avainame = CppCommon::Path(replacePath).filename().string();
            std::string rename = (name == avainame) ? "" : avainame;
            // if need, change all sub files storage dir, e.x <save>/a : <save>/a(1)
            downloadFolder(name, rename);
        }
    }
    //std::cout << "whole download finished!" << std::endl;

    sendInfobyHeader(INFO_WEB_FINISH);
    _callback->onWebChanged(WEB_TRANS_FINISH);
}



bool FileClient::createNotExistPath(const std::string &abspath, bool isfile)
{
    CppCommon::Path path(abspath);
    if (!path.IsExists()) {
        try {
            // create its parent first.
            CppCommon::Directory::CreateTree(path.parent());
            if (isfile) {
                CppCommon::File::WriteEmpty(path);
            } else {
                CppCommon::Directory::Create(path);
            }
        } catch (const CppCommon::FileSystemException &ex) {
            // 捕获并处理异常
            std::cout << "Create throw FS exception: " << ex.what() << std::endl;
            return false;
        }
        return true;
    }

    if (isfile && !path.IsDirectory()) {
        // empty file, use the exist one.
        CppCommon::File existFile(path);
        if (existFile.IsFileEmpty())
            return true;
    } else {
        // empty folder, use the exist one.
        CppCommon::Directory existDir(path);
        if (existDir.IsDirectoryEmpty())
            return true;
    }

    return false;
}

std::string FileClient::createNextAvailableName(const std::string &name, bool isfile)
{
    CppCommon::Path path = CppCommon::Path(_savedir) / name;
    auto abspath = path.string();
    if (createNotExistPath(abspath, isfile)) {
        return abspath;
    }

    std::string filename = path.filename().string();
    size_t dotPos = filename.find_last_of(".");
    std::string prefix = (dotPos == std::string::npos) ? filename : filename.substr(0, dotPos);
    std::string suffix = (dotPos == std::string::npos) ? "" : filename.substr(dotPos);

    int i = 1;
    while (true) {
        std::string newfilename = prefix + "(" + std::to_string(i) + ")" + suffix;
        std::string newpath = CppCommon::Path(path.parent() / newfilename).string();
        if (createNotExistPath(newpath, isfile)) {
            return newpath;
        }
        i++;
    }
}
