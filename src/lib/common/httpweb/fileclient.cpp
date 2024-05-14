// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileclient.h"

#include "string/string_utils.h"
#include "filesystem/file.h"
#include "filesystem/path.h"
#include "filesystem/directory.h"

#include "picojson/picojson.h"

#include <iostream>

using ResponseHandler = std::function<bool(int status, const void *buffer, size_t size)>;

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
        std::cout << "Response status: " << response.status() << std::endl;
        std::cout << "Response header body len: " << response.body_length() << std::endl;
        std::cout << "------------------" << std::endl;
        std::cout << "Response header Content: " << response.string() << std::endl;
        std::cout << "------------------" << std::endl;

        if (_handler) {
            // get body by stream, so mark response arrived.
            HTTPClientEx::onReceivedResponse(response);

            if (_handler(response.status() == 200 ? RES_OKHEADER : RES_NOTFOUND, nullptr, response.body_length())) {
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
            _handler(RES_FINISH, nullptr, 0);
            _response.Clear();
            // DisconnectAsync();
            Disconnect();
        } else {
            HTTPClientEx::onReceivedResponse(response);
        }
    }

    bool onReceivedResponseBody(const HTTPResponse &response) override
    {
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
    // Create HTTP client if the current one is empty
    if (!_httpClient)
        _httpClient = std::make_shared<HTTPFileClient>(service, address, port);

//    auto response = client->SendGetRequest(commands[1]).get();
//    std::cout << response << std::endl;


//    client->IsConnected() ? client->ReconnectAsync() : client->ConnectAsync();
}

FileClient::~FileClient()
{
    if (_httpClient) {
        _httpClient->Disconnect();
        _httpClient = nullptr;
    }
}

//void FileClient::setCallback(std::shared_ptr<ProgressCallInterface> callback) {
//    _callback = callback;
//}

void FileClient::setConfig(const std::string &token, const std::string &savedir)
{
    _token = token;
    _savedir = savedir;
}

// request the dir/file info
// [GET]info/<name>&token
bool FileClient::getInfo(const std::string &name, std::string *body)
{
    if (_token.empty()) {
        std::cout << "Must set access token!" << std::endl;
        return false;
    }

    std::string url = "info/";
    url.append(name);
    url.append("&token=").append(_token);

    _httpClient->setResponseHandler(nullptr);

    auto response = _httpClient->SendGetRequest(url).get();
    if (response.status() == 404) {
        return false;
    }

    *body = response.body();
    std::cout << response << std::endl;

    return true;

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

    _running = true;

    uint64_t current, total = 0;
    uint64_t offset = 0;
    std::string url = "download/";
    url.append(name);

    CppCommon::Path path = CppCommon::Path(_savedir) / name + ".part";
    CppCommon::File tempFile(path);
    if (tempFile.IsFileExists()) {
        // get current file lenght and continue download.
        offset = tempFile.size();
    }
    url.append("&token=").append(_token);
    url.append("&offset=").append(std::to_string(offset));

    ResponseHandler cb([this, &tempFile, &current, &total, &offset](int status, const void *buffer, size_t size) -> bool {
        std::string filepath = tempFile.absolute().string();
        switch (status)
        {
        case RES_NOTFOUND:
            std::cout << "File not Found!" << std::endl;
            if (tempFile.IsFileWriteOpened())
                tempFile.Close();
            break;
        case RES_OKHEADER:
            total = size;
            if (_callback) {
                if (!_callback->onProgress(filepath, current, total)) {
                    // return true to cancel download from outside.
                    return true;
                }
            }
            tempFile.OpenOrCreate(false, true, true);
            tempFile.Seek(offset);
            break;
        case RES_BODY:
            if (tempFile.IsFileWriteOpened() && buffer && size > 0) {
                current += size;
                // 实现层已循环写全部
                tempFile.Write(buffer, size);
                // tempFile.Flush();
                // notify downloading：current total
                if (_callback) {
                    if (!_callback->onProgress(filepath, current, total)) {
                        // return true to cancel download from outside.
                        return true;
                    }
                }
            }
            break;
        case RES_ERROR:
            if (tempFile.IsFileWriteOpened())
                tempFile.Close();
            // notify download error：break off
            if (_callback) {
                _callback->onProgress(filepath, -1, total);
            }
            break;
        case RES_FINISH:
            if (tempFile.IsFileWriteOpened()) {
                tempFile.Close();
                std::cout << "File path:" << tempFile.absolute().RemoveExtension() << std::endl;

                CppCommon::Path::Rename(tempFile.absolute(), tempFile.absolute().RemoveExtension());
                tempFile.Clear();
            }
            // notify download finished
            if (_callback) {
                _callback->onProgress(filepath, total, total);
            }
            break;

        default:
            break;
        }
        return false;
    });

    _httpClient->setResponseHandler(std::move(cb));

    auto response = _httpClient->SendGetRequest(url).get();
    return true;
}

void FileClient::downloadFolder(const std::string &folderName)
{
    _running = true;

    // Step 1: 请求文件夹信息
    std::string res_body = {};
    if (getInfo(folderName, &res_body)) {
        std::cout << "Failed to prepare for downloading folder information!" << std::endl;
        return;
    }

    // Step 2: 解析响应数据
    picojson::value v;
    std::string err = picojson::parse(v, res_body);
    if (!err.empty()) {
        std::cout << "Failed to parse JSON data: " << err << std::endl;
        return;
    }

    picojson::array files = v.get("files").get<picojson::array>();

    // Step 3: 循环处理文件夹内文件
    for (const auto &file : files) {
        if (_cancel)
            break;

        std::string fileName = file.get("name").to_str();
        double fileSize = file.get("size").get<double>();

        if (fileSize == 0) {
            // 文件夹, 创建
            CppCommon::Path folderPath = CppCommon::Path(_savedir) / folderName;
            if (!folderPath.IsExists() || !folderPath.IsDirectory())
                CppCommon::Directory::Create(folderPath);

            std::string relName = folderName + "/" + fileName;
            downloadFolder(relName); // 递归下载文件夹
        } else {
            // 具体文件
            if (!downloadFile(fileName)) {
                std::cout << "Failed to download file: " << fileName << std::endl;
            }
        }
    }
}

void FileClient::cancel(bool cancel)
{
    _cancel = cancel;
//    _running = !_cancel;
}

bool FileClient::downloading()
{
    return _httpClient->IsConnected();
//    return _running;
}

//void FileClient::walkDownload(const std::string &name, const std::string &token, const std::string &savedir)
//{
//    _cancel = false;
//    _token = token;
//    _savedir = savedir;

//    // 此文件信息未知，文件夹或文件，先查询
//    downloadFolder(name, progress);
//}
