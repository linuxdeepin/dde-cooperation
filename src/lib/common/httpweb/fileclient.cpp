// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileclient.h"
#include "streamcodec.h"
#include "streamconsumer.h"
#include "tokencache.h"

#include "filesystem/file.h"
#include "filesystem/path.h"
#include "filesystem/directory.h"

#include "http/https_client.h"

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <filesystem>
#include <mutex>

// timeout if no data arrived
inline constexpr int ExitCount = 5000;

namespace {

// Bounded hand-off from the IO thread to the consumer thread. Above the high
// water mark the client pauses re-arming reads instead of blocking the IO
// thread on disk I/O.
constexpr size_t kStreamHighWater = 4 * 1024 * 1024;
constexpr size_t kStreamLowWater = 1 * 1024 * 1024;

struct StreamQueue {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<std::string> chunks;
    size_t bytes {0};
    bool ended {false};
    bool failed {false};
    bool discarded {false};
    bool paused {false};
    std::string message;
};

bool streamTransferEnabled()
{
    const char *value = std::getenv("DDE_COOPERATION_HTTPWEB_STREAM");
    return !(value && std::string(value) == "0");
}

// Production always disables Nagle. The benchmark lowers this to reproduce
// the pre-D11 baseline where both ends had Nagle enabled.
bool clientNoDelayEnabled()
{
    const char *value = std::getenv("DDE_COOPERATION_HTTPWEB_CLIENT_NODELAY");
    return !(value && std::string(value) == "0");
}

} // namespace

using NetUtil::HTTP::HTTPRequest;
using NetUtil::HTTP::HTTPResponse;

class HTTPFileClient : public NetUtil::HTTP::HTTPSClientEx
{
public:
    using NetUtil::HTTP::HTTPSClientEx::HTTPSClientEx;

    // The cancel flag belongs to one request: a handler that asked to cancel
    // must not poison the next download on the same client.
    void resetCanceled()
    {
        _canceled.store(false);
    }

    uint64_t connections() const
    {
        return _connections.load();
    }

    void setResponseHandler(ResponseHandler cb)
    {
        {
            // The handler is replaced from the download thread while the IO
            // thread may be invoking it: never touch it without the lock.
            std::lock_guard<std::mutex> lock(_handlerMutex);
            _handler = std::move(cb);
        }
        // Must sleep for release something, or it will be blocked in request
        BaseKit::Thread::Yield();
        BaseKit::Thread::Sleep(1);
    }

    // Snapshot the handler so the IO thread never invokes a std::function that
    // is being reassigned.
    ResponseHandler handlerSnapshot() const
    {
        std::lock_guard<std::mutex> lock(_handlerMutex);
        return _handler;
    }

protected:
    void onConnected() override
    {
        // must invoke supper fun, or cause canot connect server.
        HTTPSClientEx::onConnected();
        _connections.fetch_add(1);
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

        const ResponseHandler handler = handlerSnapshot();
        if (handler) {
            // get body by stream, so mark response arrived.
            try {
                HTTPSClientEx::onReceivedResponse(response);
            } catch (const std::future_error &e) {
                // 忽略"promise already satisfied"错误
                std::cerr << "Ignored future error in onReceivedResponseHeader: " << e.what() << std::endl;
            }

            if (handler(response.status() == 200 ? RES_OKHEADER : RES_NOTFOUND, response.string().data(), response.body_length())) {
                // cancel
                DisconnectAsync();
            }
            // Keep body bytes that arrived in the same read as the header.
            _response.ConsumeHeader();
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

        const ResponseHandler handler = handlerSnapshot();
        if (handler) {
            std::string cache = response.cache();
            size_t size = cache.size();
            // donot disconnect at here, this connection may be continue to downlad other, or cause mem leak
            handler(RES_FINISH, cache.data(), size);
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
        const ResponseHandler handler = handlerSnapshot();
        if (handler) {
            std::string cache = response.cache();
            size_t size = cache.size();
            if (handler(RES_BODY, cache.data(), size)) {
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
        const ResponseHandler handler = handlerSnapshot();
        if (handler) {
            handler(RES_ERROR, nullptr, 0);
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
    mutable std::mutex _handlerMutex;
    ResponseHandler _handler { nullptr };
    std::atomic<bool> _canceled { false };
    std::atomic<uint64_t> _connections { 0 };
};

FileClient::FileClient(const std::shared_ptr<NetUtil::Asio::Service> &service, const std::shared_ptr<NetUtil::Asio::SSLContext>& context, const std::string &address, int port)
{
    if (_httpClient) {
        //discontect current one
        _httpClient->DisconnectAsync();
    }
    // Create HTTP client if the current one is empty
    _httpClient = std::make_shared<HTTPFileClient>(service, context, address, port);
    // Requests are small and latency sensitive; keep them off Nagle. The
    // server side is the important half (its header/body split caused the
    // 40 ms/file stall), but disabling it on both ends is symmetric.
    _httpClient->SetupNoDelay(clientNoDelayEnabled());
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

FileClient::WebRequestCounters FileClient::requestCounters() const
{
    WebRequestCounters counters;
    counters.headerRequests = _headerRequests.load();
    counters.infoRequests = _infoRequests.load();
    counters.downloadRequests = _downloadRequests.load();
    counters.connections = _httpClient ? _httpClient->connections() : 0;
    return counters;
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
    _headerRequests.fetch_add(1);
    _httpClient->setResponseHandler(nullptr);

    std::string url = s_headerInfos[mask] + ">" + name;
    
    try {
        _httpClient->SendHeadRequest(url, BaseKit::Timespan::seconds(3)).get();
    } catch (const std::exception &e) {
        std::cerr << "Exception during sending header info: " << e.what() << std::endl;
        // 头部信息发送失败，继续执行不影响主要功能
    }
}

FileClient::InfoStatus FileClient::requestInfo(const std::string &name, InfoEntry *out)
{
    if (!out) {
        return InfoStatus::Error;
    }
    *out = InfoEntry();
    if (_token.empty()) {
        std::cout << "Must set access token!" << std::endl;
        return InfoStatus::Error;
    }

    // request the dir/file info
    // [GET]info/<name>&token
    std::string url = "info/";
    // base64 the file name in order to keep original name, which may include '&'
    std::string ename = BaseKit::Encoding::Base64Encode(name);
    url.append(ename);
    url.append("&token=").append(_token);

    _infoRequests.fetch_add(1);
    _httpClient->resetCanceled();
    _httpClient->setResponseHandler(nullptr);

    try {
        auto response = _httpClient->SendGetRequest(url, BaseKit::Timespan::seconds(3)).get();
        if (response.status() == 404)
            return InfoStatus::NotFound;
        if (response.status() != 200) {
            std::cout << "requestInfo(" << name << ") failed with status " << response.status() << std::endl;
            return InfoStatus::Error;
        }
        const std::string body_str(response.body());

        picojson::value v;
        std::string err = picojson::parse(v, body_str);
        if (!err.empty()) {
            std::cout << "Failed to parse JSON data: " << err << std::endl;
            return InfoStatus::Error;
        }

        out->from_json(v);
    } catch (const std::exception &e) {
        std::cerr << "Exception during requesting info: " << e.what() << std::endl;
        return InfoStatus::Error;
    }

    return InfoStatus::Ok;
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
        if (auto callback = this->callback()) {
            callback->onWebChanged(WEB_IO_ERROR, "fs_exception");
        }
        return false;
    }

    bool result = false;
    {
        std::atomic<int> timeout_done(0);
        // A failed download must not leave the empty placeholder file that
        // createNextAvailableName() just created behind. The flags are written
        // by the HTTP callback thread and read by the download thread.
        std::atomic<bool> completed { false };
        std::atomic<bool> failed { false };

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
                failed.store(true);
                if (auto callback = this->callback()) {
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

                    if (auto callback = this->callback()) {
                        callback->onWebChanged(WEB_FILE_BEGIN, file_path.string(), size);
                    }
                } catch (const BaseKit::FileSystemException &ex) {
                    std::cout << "Header create throw FS exception: " << ex.message() << std::endl;
                    shouldExit = true;
                    failed.store(true);
                    if (auto callback = this->callback()) {
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

                        if (auto callback = this->callback()) {
                            callback->onProgress(size);
                        }
                    } catch (const BaseKit::FileSystemException &ex) {
                        std::cout << "Write throw FS exception: " << ex.message() << std::endl;
                        shouldExit = true;
                        failed.store(true);
                        if (auto callback = this->callback()) {
                            callback->onWebChanged(WEB_IO_ERROR, "io_error");
                        }
                    }
                }
            }
            break;
            case RES_ERROR: {
                // error：break off
                shouldExit = true;
                failed.store(true);

                if (auto callback = this->callback()) {
                    callback->onWebChanged(WEB_DISCONNECTED, "net_error");
                }
            }
            break;
            case RES_FINISH: {
                if (tempFile.IsFileWriteOpened()) {
                    try {
                        // 写入最后一块
                        tempFile.Write(buffer, size);
                        // A small file can arrive entirely in one packet, in
                        // which case only RES_FINISH is delivered and these
                        // bytes would never be counted as progress.
                        if (size > 0) {
                            if (auto callback = this->callback()) {
                                callback->onProgress(size);
                            }
                        }
                    } catch (const BaseKit::FileSystemException &ex) {
                        std::cout << "Write&Close throw FS exception: " << ex.message() << std::endl;
                        failed.store(true);
                        if (auto callback = this->callback()) {
                            callback->onWebChanged(WEB_IO_ERROR, "io_error");
                        }
                    }
                }

                completed.store(!failed.load());
                shouldExit = true;
                if (auto callback = this->callback()) {
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
        _httpClient->resetCanceled();
        _httpClient->setResponseHandler(std::move(cb));

        std::string url = "download/";
        // base64 the file name in order to keep original name, which may include '&'
        std::string ename = BaseKit::Encoding::Base64Encode(name);
        url.append(ename);
        url.append("&token=").append(_token);
        url.append("&offset=").append(std::to_string(offset));

        _downloadRequests.fetch_add(1);
        try {
            _httpClient->SendGetRequest(url).get(); // use get to sync download one by one
        } catch (const std::exception &e) {
            std::cerr << "Exception during file download: " << e.what() << std::endl;
            // 通知回调发生网络错误
            if (auto callback = this->callback()) {
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

        // A request that timed out leaves a response in flight. Reusing the
        // same connection would let that late response complete the next
        // request's promise, mixing both files. Drop the connection first.
        if (!completed.load() && !failed.load() && !_stop.load()) {
            // Nobody will report this file otherwise: the response simply
            // stopped arriving. Tell the UI before dropping the connection.
            if (auto callback = this->callback()) {
                callback->onWebChanged(WEB_DISCONNECTED, "timeout");
            }
            _httpClient->DisconnectAsync();
            for (int i = 0; i < 200 && _httpClient->IsConnected(); ++i) {
                BaseKit::Thread::Yield();
                BaseKit::Thread::Sleep(1);
            }
        }

        // Remove the placeholder without touching a legitimately empty file:
        // it is only kept when the transfer completed successfully.
        if ((!completed.load() || failed.load()) && !avaipath.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(avaipath, ec) && !ec
                && std::filesystem::file_size(avaipath, ec) == 0 && !ec) {
                std::filesystem::remove(avaipath, ec);
                if (ec) {
                    std::cout << "Remove empty file failed: " << avaipath
                              << " error: " << ec.message() << std::endl;
                }
            }
        }

        _httpClient->setResponseHandler(nullptr);

        result = completed.load() && !failed.load();
    }
    // std::cout << "$$$ file end: " << name << std::endl;

    if (!result)
        std::cout << "downloadFile failed: " << name << std::endl;
    return result;
}

FileClient::StreamAttempt FileClient::downloadStream(const std::string &name)
{
    if (!streamTransferEnabled() || name.empty())
        return StreamAttempt::Fallback;

    StreamResumeState state = loadCheckpoint(name);
    static constexpr int kMaxAttempts = 3;
    static constexpr int kBackoffMs[2] = { 200, 800 };

    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        StreamResumeState next = state;
        bool retryable = false;
        const StreamAttempt result = downloadStreamOnce(name, state, &next, &retryable);

        if (result == StreamAttempt::Ok) {
            clearCheckpoint(name);
            return result;
        }
        if (result == StreamAttempt::Fallback || result == StreamAttempt::Canceled) {
            clearCheckpoint(name);
            return result;
        }

        if (next.nextFrame > 0)
            state = next;
        else
            state.error = next.error;

        // A checkpoint can lag behind the disk by up to one second (or a
        // previous process may have died). Re-sending an already final file is
        // detected as a conflict; restart under a fresh root instead of
        // overwriting or deleting the existing tree.
        if (!retryable && state.nextFrame > 0
                && (state.error.find("resume conflict") != std::string::npos
                    || state.error.find("resume root") != std::string::npos)
                && attempt + 1 < kMaxAttempts) {
            std::cout << "stream resume rejected, restarting: " << state.error << std::endl;
            state = StreamResumeState {};
            clearCheckpoint(name);
            continue;
        }

        saveCheckpoint(name, state);

        if (!retryable || attempt + 1 >= kMaxAttempts) {
            if (auto callback = this->callback()) {
                callback->onWebChanged(retryable ? WEB_DISCONNECTED : WEB_IO_ERROR,
                                       state.error.empty() ? "stream_failed" : state.error);
            }
            std::cout << "stream failed after " << (attempt + 1) << " attempt(s): "
                      << state.error << std::endl;
            return StreamAttempt::Error;
        }

        std::cout << "stream attempt " << (attempt + 1) << " failed ("
                  << state.error << "), resuming at frame " << state.nextFrame << std::endl;

        // Let DisconnectAsync settle before reusing the client.
        for (int i = 0; i < 20 && _httpClient->IsConnected(); ++i) {
            if (_stop.load())
                return StreamAttempt::Canceled;
            BaseKit::Thread::Sleep(50);
        }
        const int backoff = kBackoffMs[std::min(attempt, 1)];
        for (int waited = 0; waited < backoff; waited += 100) {
            if (_stop.load())
                return StreamAttempt::Canceled;
            BaseKit::Thread::Sleep(std::min(100, backoff - waited));
        }
    }
    return StreamAttempt::Error;
}

FileClient::StreamAttempt FileClient::downloadStreamOnce(
        const std::string &name, const StreamResumeState &resume,
        StreamResumeState *next, bool *retryable)
{
    *retryable = false;
    *next = resume;
    next->error.clear();

    auto queue = std::make_shared<StreamQueue>();
    _httpClient->resetCanceled();

    auto handler = [this, queue](int status, const char *buffer, size_t size) -> bool {
        if (status == RES_BODY) {
            if (_stop.load())
                return true;  // disconnect, the consumer thread reports cancel
            {
                std::lock_guard<std::mutex> lock(queue->mutex);
                if (queue->discarded)
                    return false;
                queue->chunks.emplace_back(buffer ? std::string(buffer, size) : std::string());
                queue->bytes += size;
                if (queue->bytes > kStreamHighWater && !queue->paused) {
                    queue->paused = true;
                    _httpClient->PauseReceive();
                }
            }
            queue->cv.notify_all();
            return false;
        }
        if (status == RES_FINISH) {
            {
                std::lock_guard<std::mutex> lock(queue->mutex);
                queue->ended = true;
            }
            queue->cv.notify_all();
            return false;
        }
        if (status == RES_ERROR) {
            {
                std::lock_guard<std::mutex> lock(queue->mutex);
                queue->failed = true;
                queue->message = "stream response failed";
                queue->ended = true;
            }
            queue->cv.notify_all();
            return false;
        }
        // RES_OKHEADER / RES_NOTFOUND: the waiting thread inspects the future.
        return false;
    };
    _httpClient->setResponseHandler(handler);
    _downloadRequests.fetch_add(1);

    std::string url = "stream/";
    url.append(BaseKit::Encoding::Base64Encode(name));
    url.append("&token=").append(_token);
    url.append("&codec=").append(dtp::codecName(
            dtp::codecSupported(dtp::Codec::Zstd) ? dtp::Codec::Zstd : dtp::Codec::Deflate));
    if (resume.nextFrame > 0)
        url.append("&from=").append(std::to_string(resume.nextFrame));

    NetUtil::HTTP::HTTPResponse response;
    try {
        // The whole selection is one request; the timeout only guards a stuck
        // peer, not the duration of a large but progressing transfer.
        response = _httpClient->SendGetRequest(url, BaseKit::Timespan::hours(12)).get();
    } catch (const std::exception &e) {
        _httpClient->setResponseHandler(nullptr);
        next->error = e.what();
        *retryable = true;
        std::cout << "stream request failed: " << e.what() << std::endl;
        return StreamAttempt::Error;
    }

    if (response.status() != 200 || !response.stream_mode()) {
        // Old server or rejected request. Drain/discard the small error body so
        // the connection returns to idle, then let the caller use legacy.
        {
            std::lock_guard<std::mutex> lock(queue->mutex);
            queue->discarded = true;
            queue->chunks.clear();
            queue->bytes = 0;
        }
        bool ended = false;
        {
            std::unique_lock<std::mutex> lock(queue->mutex);
            ended = queue->cv.wait_for(lock, std::chrono::seconds(3),
                                       [&queue] { return queue->ended; });
        }
        if (!ended)
            _httpClient->DisconnectAsync();
        _httpClient->setResponseHandler(nullptr);
        return StreamAttempt::Fallback;
    }

    // On a resumed request the server starts after the root frame; the
    // consumer continues inside the existing root and rejects RootDir.
    dtp::StreamConsumerOptions consumerOptions;
    consumerOptions.targetDir = _savedir;
    if (resume.nextFrame > 0)
        consumerOptions.resumeRoot = resume.rootPath;
    dtp::StreamConsumer consumer(consumerOptions);

    auto lastCheckpoint = std::chrono::steady_clock::now();
    consumer.setCheckpointCallback([&](uint64_t completed) {
        next->nextFrame = completed;
        next->rootPath = consumer.rootPath();
        const auto now = std::chrono::steady_clock::now();
        if (now - lastCheckpoint >= std::chrono::seconds(1)) {
            saveCheckpoint(name, *next);
            lastCheckpoint = now;
        }
    });
    consumer.setProgressCallback([this](uint64_t bytes) {
        if (_stop.load())
            return false;
        if (auto callback = this->callback())
            return !callback->onProgress(bytes);
        return true;
    });
    consumer.setFileCallback(
            [this](const std::string &path, uint64_t size) {
                if (auto callback = this->callback())
                    callback->onWebChanged(WEB_FILE_BEGIN, path, size);
            },
            [this](const std::string &path, uint64_t size) {
                if (auto callback = this->callback())
                    callback->onWebChanged(WEB_FILE_END, path, size);
            });
    consumer.setErrorCallback([](const std::string &path, const std::string &message) {
        // A file the sender could not read arrives as an ERROR frame; the rest
        // of the archive is still valid.
        std::cout << "stream: " << path << " skipped: " << message << std::endl;
    });

    const auto closeConnection = [this]() {
        _httpClient->setResponseHandler(nullptr);
        _httpClient->DisconnectAsync();
    };

    // Body stall watchdog. The request timeout only bounds connection setup
    // and header wait; a peer that keeps the socket open without sending must
    // be treated as a failed attempt so the retry/resume path can run.
    int idleSeconds = 60;
    if (const char *env = std::getenv("DDE_COOPERATION_HTTPWEB_STREAM_IDLE_TIMEOUT")) {
        try {
            idleSeconds = std::max(1, std::stoi(env));
        } catch (const std::exception &) {
        }
    }
    auto lastActivity = std::chrono::steady_clock::now();

    for (;;) {
        std::string chunk;
        bool ended = false;
        {
            std::unique_lock<std::mutex> lock(queue->mutex);
            queue->cv.wait_for(lock, std::chrono::milliseconds(100), [&queue] {
                return queue->ended || !queue->chunks.empty();
            });
            if (!queue->chunks.empty()) {
                chunk = std::move(queue->chunks.front());
                queue->chunks.pop_front();
                queue->bytes -= chunk.size();
            }
            ended = queue->ended;
        }

        if (_stop.load()) {
            consumer.abort();
            next->nextFrame = consumer.completedEntries();
            next->rootPath = consumer.rootPath();
            closeConnection();
            return StreamAttempt::Canceled;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!chunk.empty()) {
            lastActivity = now;
        } else if (!ended && now - lastActivity > std::chrono::seconds(idleSeconds)) {
            consumer.abort();
            next->nextFrame = consumer.completedEntries();
            next->rootPath = consumer.rootPath();
            next->error = "stream idle timeout";
            *retryable = true;
            closeConnection();
            return StreamAttempt::Error;
        }

        if (!chunk.empty() && !consumer.feed(chunk.data(), chunk.size())) {
            const bool canceled = consumer.status() == dtp::StreamConsumer::Status::Canceled;
            next->nextFrame = consumer.completedEntries();
            next->rootPath = consumer.rootPath();
            next->error = consumer.lastError();
            std::cout << "stream consumer failed: " << next->error << std::endl;
            consumer.abort();
            closeConnection();
            if (canceled)
                return StreamAttempt::Canceled;
            // Disk, path or protocol error: retrying the same request would
            // hit the same condition.
            *retryable = false;
            return StreamAttempt::Error;
        }

        bool resumeReceive = false;
        {
            std::lock_guard<std::mutex> lock(queue->mutex);
            if (queue->paused && queue->bytes <= kStreamLowWater) {
                queue->paused = false;
                resumeReceive = true;
            }
        }
        if (resumeReceive)
            _httpClient->ResumeReceive();

        if (chunk.empty() && ended)
            break;
    }

    bool failed = false;
    std::string message;
    {
        std::lock_guard<std::mutex> lock(queue->mutex);
        failed = queue->failed;
        message = queue->message;
    }

    next->nextFrame = consumer.completedEntries();
    next->rootPath = consumer.rootPath();

    if (failed) {
        consumer.abort();
        closeConnection();
        next->error = message;
        *retryable = true;
        return StreamAttempt::Error;
    }

    std::string error;
    const bool ok = consumer.finish(&error);
    closeConnection();

    if (!ok) {
        next->nextFrame = consumer.completedEntries();
        next->rootPath = consumer.rootPath();
        next->error = error.empty() ? "stream incomplete" : error;
        // A missing END frame is a transport truncation; the receiver's .part
        // files were already removed, so the request can be retried.
        *retryable = true;
        std::cout << "stream incomplete: " << next->error << std::endl;
        return StreamAttempt::Error;
    }
    return StreamAttempt::Ok;
}

std::string FileClient::checkpointPath(const std::string &name) const
{
    uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](const std::string &text) {
        for (unsigned char byte : text) {
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        hash ^= 0xff;
        hash *= 1099511628211ULL;
    };
    mix(_token);
    mix(name);

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%016llx",
                  static_cast<unsigned long long>(hash));
    return (std::filesystem::temp_directory_path()
            / "dde-cooperation-checkpoints" / (std::string(buffer) + ".ckpt")).u8string();
}

FileClient::StreamResumeState FileClient::loadCheckpoint(const std::string &name)
{
    StreamResumeState state;
    const std::string path = checkpointPath(name);
    std::ifstream in(path);
    if (!in.is_open())
        return state;

    std::string magic;
    std::string fromLine;
    std::string rootLine;
    std::getline(in, magic);
    std::getline(in, fromLine);
    std::getline(in, rootLine);
    if (magic != "DTPCP1" || fromLine.rfind("from=", 0) != 0
            || rootLine.rfind("root=", 0) != 0) {
        clearCheckpoint(name);
        return state;
    }

    try {
        state.nextFrame = std::stoull(fromLine.substr(5));
        state.rootPath = BaseKit::Encoding::Base64Decode(rootLine.substr(5));
    } catch (const std::exception &) {
        clearCheckpoint(name);
        return StreamResumeState {};
    }

    if (state.nextFrame == 0)
        return StreamResumeState {};

    std::error_code ec;
    if (state.rootPath.empty()
            || !std::filesystem::is_directory(std::filesystem::u8path(state.rootPath), ec)
            || ec) {
        // The partial tree is gone (reboot/temp cleanup): start over.
        clearCheckpoint(name);
        return StreamResumeState {};
    }
    return state;
}

void FileClient::saveCheckpoint(const std::string &name, const StreamResumeState &state)
{
    if (state.nextFrame == 0) {
        clearCheckpoint(name);
        return;
    }

    const std::string path = checkpointPath(name);
    const std::filesystem::path file(path);
    std::error_code ec;
    std::filesystem::create_directories(file.parent_path(), ec);
    if (ec)
        return;

    const std::string temp = path + ".tmp";
    {
        std::ofstream out(temp, std::ios::trunc);
        if (!out.is_open())
            return;
        out << "DTPCP1\n"
            << "from=" << state.nextFrame << "\n"
            << "root=" << BaseKit::Encoding::Base64Encode(state.rootPath) << "\n";
    }
    std::filesystem::rename(temp, file, ec);
    if (ec) {
        ec.clear();
        std::filesystem::remove(temp, ec);
    }
}

void FileClient::clearCheckpoint(const std::string &name)
{
    std::error_code ec;
    std::filesystem::remove(checkpointPath(name), ec);
}

void FileClient::walkDownload(const std::vector<std::string> &webnames)
{
    sendInfobyHeader(INFO_WEB_START);
    if (auto callback = this->callback()) {
        callback->onWebChanged(WEB_TRANS_START);
    }

    for (const auto& name : webnames) {
        //std::cout << "start download web: " << name << std::endl;

        sendInfobyHeader(INFO_WEB_INDEX, name);
        if (auto callback = this->callback()) {
            callback->onWebChanged(WEB_INDEX_BEGIN, name);
        }

        // DTP2: one request for the whole selection. Old servers answer with a
        // non stream response and we silently continue with the legacy path.
        const StreamAttempt attempt = downloadStream(name);
        if (attempt == StreamAttempt::Ok)
            continue;
        if (attempt == StreamAttempt::Error || attempt == StreamAttempt::Canceled)
            return;

        // do not sure the file type: floder or file
        InfoEntry info;
        const InfoStatus status = requestInfo(name, &info);
        if (status == InfoStatus::Error) {
            // Transport/protocol failure: abort, never send WEB_TRANS_FINISH.
            std::cout << name << " walkDownload requestInfo failed!" << std::endl;
            if (auto callback = this->callback()) {
                callback->onWebChanged(WEB_DISCONNECTED, "info_failed");
            }
            return;
        }
        if (status == InfoStatus::NotFound) {
            if (auto callback = this->callback()) {
                callback->onWebChanged(WEB_NOT_FOUND, name);
            }
            continue;
        }

        // file: size > 0; dir: size < 0; empty file: size == 0
        if (info.size > 0) {
            if (!downloadFile(name)) {
                return;
            }
        } else if (info.size < 0) {
            if (!walkFolder(name)) {
                return;
            }
        } else {
            createNextAvailableName(name, true);
        }

        if (_stop.load()) {
            std::cout << "User stop to download!" << std::endl;
            return;
        }
    }
    std::cout << "whole download finished!" << std::endl;

    sendInfobyHeader(INFO_WEB_FINISH);
    if (auto callback = this->callback()) {
        callback->onWebChanged(WEB_TRANS_FINISH);
    }
    std::cout << "whole download finished end thread!" << std::endl;
}

bool FileClient::walkFolder(const std::string &name)
{
    // check and create the first index dir, ex. a -> /xx/download/a(1)
    std::string replacePath = createNextAvailableName(name, false);
    if (replacePath.empty()) {
        // can not get a replace folder, skip.
        std::cout << name << "can not get a replace folder, skip!" << std::endl;
        return false;
    }
    std::string avainame = BaseKit::Path(replacePath).filename().string();
    std::string rename = (name == avainame) ? "" : avainame;

    // walk all sub files and folders into queue
    std::queue<std::string> folderEntryQueue;

    {
        // request the fist folder's info
        InfoEntry info;
        const InfoStatus status = requestInfo(name, &info);
        if (status != InfoStatus::Ok) {
            if (auto callback = this->callback()) {
                if (status == InfoStatus::NotFound)
                    callback->onWebChanged(WEB_NOT_FOUND, name);
                else
                    callback->onWebChanged(WEB_DISCONNECTED, "info_failed");
            }
            return status == InfoStatus::NotFound;
        }
        // get all sub files and folders
        for (const auto &entry : info.datas) {
            if (_stop.load())
                break;

            std::string subName = name + "/" + entry.name;
            if (entry.size < 0) {
                if (!walkFolderEntry(subName, &folderEntryQueue, 1))
                    return false;
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
            if (!downloadFile(subName))
                return false;
        } else {
            // replace the first folder name with the new one
            std::string saveName = subName;
            saveName.replace(0, name.length(), rename);
            if (!downloadFile(subName, saveName))
                return false;
        }
    }
    return true;
}

// A symlink loop makes the server list the same directory again under larger
// and larger web names, so the client cannot detect it by name. Bound the
// recursion depth instead.
static constexpr int kMaxWalkDepth = 64;

bool FileClient::walkFolderEntry(const std::string &name, std::queue<std::string> *entryQueue, int depth)
{
    if (depth > kMaxWalkDepth) {
        std::cout << "directory nesting deeper than " << kMaxWalkDepth
                  << ", stop descending: " << name << std::endl;
        if (auto callback = this->callback()) {
            callback->onWebChanged(WEB_IO_ERROR, name);
        }
        return false;
    }

    // request get sub files's info
    InfoEntry info;
    const InfoStatus status = requestInfo(name, &info);
    if (status != InfoStatus::Ok) {
        if (auto callback = this->callback()) {
            if (status == InfoStatus::NotFound)
                callback->onWebChanged(WEB_NOT_FOUND, name);
            else
                callback->onWebChanged(WEB_DISCONNECTED, "info_failed");
        }
        return status == InfoStatus::NotFound;
    }

    // get all sub files and folders
    for (const auto &entry : info.datas) {
        if (_stop.load())
            break;

        std::string subName = name + "/" + entry.name;
        if (entry.size < 0) {
            if (!walkFolderEntry(subName, entryQueue, depth + 1))
                return false;
        } else if (entry.size > 0) {
            entryQueue->push(subName);
        }
    }
    return true;
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
    namespace fs = std::filesystem;
    fs::path resolvedTarget = (fs::path(_savedir) / name).lexically_normal();
    fs::path resolvedBase = fs::path(_savedir).lexically_normal();

    std::string target = resolvedTarget.string();
    std::string base = resolvedBase.string();
    // GCC's lexically_normal() does not strip trailing separators —
    // _savedir comes from sessionmanager.cpp with a trailing QDir::separator(),
    // so base would be "/home/user/Downloads/" causing target[base.size()]
    // to land on the filename's first char instead of '/', falsely blocking
    // legitimate paths.
    while (!base.empty() && (base.back() == '/' || base.back() == '\\'))
        base.pop_back();

    if (target != base
        && !(target.size() > base.size()
             && target.compare(0, base.size(), base) == 0
             && (target[base.size()] == '/' || target[base.size()] == '\\'))) {
        std::cout << "path traversal blocked: " << target << " not within " << base << std::endl;
        return "";
    }

    BaseKit::Path path(target);

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
