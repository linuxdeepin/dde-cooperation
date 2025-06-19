// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileclient.h"
#include "tokencache.h"

#include "filesystem/file.h"
#include "filesystem/path.h"
#include "filesystem/directory.h"

#include "http/https_client.h"

#include <iostream>

// timeout if no data arrived
inline constexpr int ExitCount = 5000;

using NetUtil::HTTP::HTTPRequest;
using NetUtil::HTTP::HTTPResponse;

class HTTPFileClient : public NetUtil::HTTP::HTTPSClientEx
{
public:
    using NetUtil::HTTP::HTTPSClientEx::HTTPSClientEx;

    void setResponseHandler(ResponseHandler cb)
    {
        if (_handler) {
            _handler = nullptr;
        }
        _handler = cb;
        // Must sleep for release something, or it will be blocked in request
        BaseKit::Thread::Yield();
        BaseKit::Thread::Sleep(1);
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
            try {
                HTTPSClientEx::onReceivedResponse(response);
            } catch (const std::future_error &e) {
                // 忽略"promise already satisfied"错误
                std::cerr << "Ignored future error in onReceivedResponseHeader: " << e.what() << std::endl;
            }

            if (_handler(response.status() == 200 ? RES_OKHEADER : RES_NOTFOUND, response.string().data(), response.body_length())) {
                // cancel
                DisconnectAsync();
            }
            _response.ClearCache();
        } else {
            try {
                HTTPSClientEx::onReceivedResponseHeader(response);
            } catch (const std::future_error &e) {
                // 忽略"promise already satisfied"错误
                std::cerr << "Ignored future error in onReceivedResponseHeader fallback: " << e.what() << std::endl;
            }
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
            // donot disconnect at here, this connection may be continue to downlad other, or cause mem leak
            _handler(RES_FINISH, cache.data(), size);
            _response.Clear();
        } else {
            try {
                HTTPSClientEx::onReceivedResponse(response);
            } catch (const std::future_error &e) {
                // 忽略"promise already satisfied"错误
                std::cerr << "Ignored future error in onReceivedResponse: " << e.what() << std::endl;
            }
        }
    }

    bool onReceivedResponseBody(const HTTPResponse &response) override
    {
       // std::cout << "Response BODY cache: " << response.cache().size() << std::endl;
        if (_handler) {
            std::string cache = response.cache();
            size_t size = cache.size();
            if (_handler(RES_BODY, cache.data(), size)) {
                _canceled = true;
                // cancel
                DisconnectAsync();
            }
            _response.ClearCache();
        } else {
            try {
                HTTPSClientEx::onReceivedResponseBody(response);
            } catch (const std::future_error &e) {
                // 忽略"promise already satisfied"错误
                std::cerr << "Ignored future error in onReceivedResponseBody: " << e.what() << std::endl;
            }
        }

        return true;
    }

    void onReceivedResponseError(const HTTPResponse &response, const std::string &error) override
    {
        std::cout << "Response error: " << error << std::endl;
        if (_handler) {
            _handler(RES_ERROR, nullptr, 0);
        } else {
            try {
                HTTPSClientEx::onReceivedResponseError(response, error);
            } catch (const std::future_error &e) {
                // 忽略"promise already satisfied"错误
                std::cerr << "Ignored future error: " << e.what() << std::endl;
            }
        }
    }

private:
    ResponseHandler _handler { nullptr };
    std::atomic<bool> _canceled { false };
};

FileClient::FileClient(const std::shared_ptr<NetUtil::Asio::Service> &service, const std::shared_ptr<NetUtil::Asio::SSLContext>& context, const std::string &address, int port)
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
    if (_downloadThread.joinable())
        _downloadThread.join();
    if (_httpClient) {
        try {
            _httpClient->DisconnectAsync();
            _httpClient->context().reset();
            _httpClient.reset();
        } catch (const std::exception &e) {
            std::cerr << "Exception in FileClient destructor: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in FileClient destructor" << std::endl;
        }
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
    _stop.store(true);
    try {
        _httpClient->DisconnectAsync();
    } catch (const std::exception &e) {
        std::cerr << "Exception during disconnect: " << e.what() << std::endl;
    }
    // Note: can not join this thread beause of it callback to main thread.
    // _downloadThread.join();
}

void FileClient::startFileDownload(const std::vector<std::string> &webnames)
{
    if (_token.empty() || _savedir.empty()) {
        std::cout << "Must setConfig first!" << std::endl;
        return;
    }

    _stop.store(false);

    _downloadThread = BaseKit::Thread::Start([this, webnames]() { walkDownload(webnames); });
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
    
    try {
        _httpClient->SendHeadRequest(url, BaseKit::Timespan::seconds(3)).get();
    } catch (const std::exception &e) {
        std::cerr << "Exception during sending header info: " << e.what() << std::endl;
        // 头部信息发送失败，继续执行不影响主要功能
    }
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
    std::string ename = BaseKit::Encoding::Base64Encode(name);
    url.append(ename);
    url.append("&token=").append(_token);

    _httpClient->setResponseHandler(nullptr);

    try {
        auto response = _httpClient->SendGetRequest(url, BaseKit::Timespan::seconds(3)).get();
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
    } catch (const std::exception &e) {
        std::cerr << "Exception during requesting info: " << e.what() << std::endl;
        // 当请求信息失败时，返回空信息，调用方会处理
    }
    
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
    auto avaipath = createNextAvailableName(rename.empty() ? name : rename, true);
    if (avaipath.empty()) {
        //FS exception now
        std::cout << "createNextAvailableName exception now! " << name << std::endl;
        if (auto callback = _callback.lock()) {
            callback->onWebChanged(WEB_IO_ERROR, "fs_exception");
        }
        return false;
    }

    bool result = false;
    {
        std::atomic<int> timeout_done(0);

        uint64_t offset = 0;
        auto tempFile = BaseKit::File(avaipath);
        //    offset = tempFile.size();

        ResponseHandler cb([&](int status, const char *buffer, size_t size) -> bool {
            timeout_done.store(0); // reset when data arrived
            if (_stop.load()) {
                std::cout << "has been canceled from outside!" << std::endl;
                return true;
            }

            bool shouldExit = false;
            switch (status)
            {
            case RES_NOTFOUND: {
                std::cout << "File not Found!" << std::endl;

                // error：not found
                shouldExit = true;
                if (auto callback = _callback.lock()) {
                    callback->onWebChanged(WEB_NOT_FOUND, "not_found");
                }
            }
            break;
            case RES_OKHEADER: {
                //std::string flag = this->getHeadKey(buffer, "Flag");
                //std::cout << "head flag: " << flag << std::endl;
                try {
                    BaseKit::Path file_path = tempFile.absolute().RemoveExtension();

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

                    if (auto callback = _callback.lock()) {
                        callback->onWebChanged(WEB_FILE_BEGIN, file_path.string(), size);
                    }
                } catch (const BaseKit::FileSystemException &ex) {
                    std::cout << "Header create throw FS exception: " << ex.message() << std::endl;
                    shouldExit = true;
                    if (auto callback = _callback.lock()) {
                        callback->onWebChanged(WEB_IO_ERROR, "io_error");
                    }
                }
            }
            break;
            case RES_BODY: {
                if (tempFile.IsFileWriteOpened() && buffer && size > 0) {
                    try {
                        // 实现层已循环写全部
                        tempFile.Write(buffer, size);

                        if (auto callback = _callback.lock()) {
                            callback->onProgress(size);
                        }
                    } catch (const BaseKit::FileSystemException &ex) {
                        std::cout << "Write throw FS exception: " << ex.message() << std::endl;
                        shouldExit = true;
                        if (auto callback = _callback.lock()) {
                            callback->onWebChanged(WEB_IO_ERROR, "io_error");
                        }
                    }
                }
            }
            break;
            case RES_ERROR: {
                // error：break off
                shouldExit = true;

                if (auto callback = _callback.lock()) {
                    callback->onWebChanged(WEB_DISCONNECTED, "net_error");
                }
            }
            break;
            case RES_FINISH: {
                if (tempFile.IsFileWriteOpened()) {
                    try {
                        // 写入最后一块
                        tempFile.Write(buffer, size);
                    } catch (const BaseKit::FileSystemException &ex) {
                        std::cout << "Write&Close throw FS exception: " << ex.message() << std::endl;
                        if (auto callback = _callback.lock()) {
                            callback->onWebChanged(WEB_IO_ERROR, "io_error");
                        }
                    }
                }

                shouldExit = true;
                if (auto callback = _callback.lock()) {
                    callback->onWebChanged(WEB_FILE_END, tempFile.string(), size);
                }
            }
            break;

            default:
                std::cout << "error, unkonw status=" << status << std::endl;
                break;
            }

            if (shouldExit) {
                timeout_done.store(ExitCount);
            }

            return shouldExit;
        });

        // auto self(_httpClient->shared_from_this());
        _httpClient->setResponseHandler(std::move(cb));

        std::string url = "download/";
        // base64 the file name in order to keep original name, which may include '&'
        std::string ename = BaseKit::Encoding::Base64Encode(name);
        url.append(ename);
        url.append("&token=").append(_token);
        url.append("&offset=").append(std::to_string(offset));

        try {
            _httpClient->SendGetRequest(url).get(); // use get to sync download one by one
        } catch (const std::exception &e) {
            std::cerr << "Exception during file download: " << e.what() << std::endl;
            // 通知回调发生网络错误
            if (auto callback = _callback.lock()) {
                callback->onWebChanged(WEB_DISCONNECTED, "net_error");
            }
            timeout_done.store(ExitCount); // 确保退出下面的等待循环
        }

        // Wait for download finish or no data arrived more than many times
        while (!_stop.load()) {
            auto count = timeout_done.load();
            if (count >= ExitCount) {
                break;
            }
            timeout_done.fetch_add(1);
            BaseKit::Thread::Yield();
            BaseKit::Thread::Sleep(1);
        }

        // make sure the file has been closed
        if (tempFile.IsFileWriteOpened()) {
            try {
                // tempFile.Flush();
                tempFile.Close();
                tempFile.Clear();
            } catch (const BaseKit::FileSystemException &ex) {
                std::cout << "Close throw FS exception: " << ex.message() << std::endl;
            }
        }

        _httpClient->setResponseHandler(nullptr);
    }
    // std::cout << "$$$ file end: " << name << std::endl;

    return result;
}

void FileClient::walkDownload(const std::vector<std::string> &webnames)
{
    sendInfobyHeader(INFO_WEB_START);
    if (auto callback = _callback.lock()) {
        callback->onWebChanged(WEB_TRANS_START);
    }

    for (const auto& name : webnames) {
        //std::cout << "start download web: " << name << std::endl;

        sendInfobyHeader(INFO_WEB_INDEX, name);
        if (auto callback = _callback.lock()) {
            callback->onWebChanged(WEB_INDEX_BEGIN, name);
        }

        // do not sure the file type: floder or file
        auto info = requestInfo(name);
        if (info.size == 0) {
            std::cout << name << " walkDownload requestInfo return empty! " << std::endl;
            createNextAvailableName(name, true);
            continue;
        }

        // file: size > 0; dir: size < 0; default size = 0
        if (info.size > 0) {
            downloadFile(name);
        } else {
            walkFolder(name);
        }

        if (_stop.load()) {
            std::cout << "User stop to download!" << std::endl;
            return;
        }
    }
    std::cout << "whole download finished!" << std::endl;

    sendInfobyHeader(INFO_WEB_FINISH);
    if (auto callback = _callback.lock()) {
        callback->onWebChanged(WEB_TRANS_FINISH);
    }
    std::cout << "whole download finished end thread!" << std::endl;
}

void FileClient::walkFolder(const std::string &name)
{
    // check and create the first index dir, ex. a -> /xx/download/a(1)
    std::string replacePath = createNextAvailableName(name, false);
    if (replacePath.empty()) {
        // can not get a replace folder, skip.
        std::cout << name << "can not get a replace folder, skip!" << std::endl;
        return;
    }
    std::string avainame = BaseKit::Path(replacePath).filename().string();
    std::string rename = (name == avainame) ? "" : avainame;

    // walk all sub files and folders into queue
    std::queue<std::string> folderEntryQueue;

    {
        // request the fist folder's info
        auto info = requestInfo(name);
        // get all sub files and folders
        for (const auto &entry : info.datas) {
            if (_stop.load())
                break;

            std::string subName = name + "/" + entry.name;
            if (entry.size < 0) {
                walkFolderEntry(subName, &folderEntryQueue);
            } else if (entry.size > 0) {
                folderEntryQueue.push(subName);
            } else {
                std::string saveName = subName;
                if (!rename.empty()) {
                    // replace the first folder name with the new one
                    saveName.replace(0, name.length(), rename);
                }
                std::cout << "create empty file: " << saveName << std::endl;
                createNextAvailableName(saveName, true);
            }
        }
    }
    // std::cout << "folderEntryQueue size: " << folderEntryQueue.size() << std::endl;

    // download all files and folders in queue
    while (!folderEntryQueue.empty()) {
        if (_stop.load())
            break;

        std::string subName = folderEntryQueue.front();
        folderEntryQueue.pop();

        if (rename.empty()) {
            // do not need to change the save dir
            downloadFile(subName);
        } else {
            // replace the first folder name with the new one
            std::string saveName = subName;
            saveName.replace(0, name.length(), rename);
            downloadFile(subName, saveName);
        }
    }
}

void FileClient::walkFolderEntry(const std::string &name, std::queue<std::string> *entryQueue)
{
    // request get sub files's info
    auto info = requestInfo(name);

    // get all sub files and folders
    for (const auto &entry : info.datas) {
        if (_stop.load())
            break;

        std::string subName = name + "/" + entry.name;
        if (entry.size < 0) {
            walkFolderEntry(subName, entryQueue);
        } else if (entry.size > 0) {
            entryQueue->push(subName);
        }
    }
}

bool FileClient::createNotExistPath(std::string &abspath, bool isfile)
{
    BaseKit::Path path(abspath);
    try {
        if (!path.IsExists()) {
            // create its parent first.
            BaseKit::Directory::CreateTree(path.parent());
            if (isfile) {
                BaseKit::File::WriteEmpty(path);
            } else {
                BaseKit::Directory::Create(path);
            }

            return true;
        }
    } catch (const BaseKit::FileSystemException &ex) {
        // std::cout << "Create throw FS exception: " << abspath << std::endl;
#ifdef __linux__
        auto filename = path.filename().string();
        if (filename.length() >= NAME_MAX) {
            // 长文件名截取前后
            size_t catlen = NAME_MAX/3;
            std::string prefix = filename.substr(0, catlen);
            std::string suffix = filename.substr(filename.length() - catlen, catlen);

            filename = prefix + "..." + suffix;
            // std::cout << "rename to short: " << filename << std::endl;
            path = path.parent() / filename;
            std::string repath = path.string();
            auto check = createNotExistPath(repath, isfile);
            abspath = repath;
            return check;
        }
#endif
        // 捕获, 重置路径表明异常发生
        abspath = "";
        return false;
    }

    if (isfile && !path.IsDirectory()) {
        // empty file, use the exist one.
        BaseKit::File existFile(path);
        if (existFile.IsFileEmpty())
            return true;
    } else {
        // empty folder, use the exist one.
        BaseKit::Directory existDir(path);
        if (existDir.IsDirectoryEmpty())
            return true;
    }

    return false;
}

std::string FileClient::createNextAvailableName(const std::string &name, bool isfile)
{
    BaseKit::Path path = BaseKit::Path(_savedir) / name;
    path = path.canonical(); // remove all '.' and '..' properly

    // save file security check
    if (isfile) {
        try {
            // Redirect to Download folder if save into Home
            if (path.parent().IsEquivalent(path.home())) {
                std::cout << "Save dir is user Home, forbid! " << path.string() << std::endl;
                path = path.parent() / "Download" / path.filename();
            }
        } catch (const BaseKit::FileSystemException &ex) {
            // 捕获并处理异常
            // std::cout << "IsEquivalent throw FS exception: " << ex.message()<< std::endl;
        }
    }

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
        std::string newpath = BaseKit::Path(path.parent() / newfilename).string();
        if (createNotExistPath(newpath, isfile)) {
            return newpath;
        } else {
            if (newpath.empty()) {
                // FS exception hanppend
                return "";
            }
        }
        i++;
    }
}
