// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fileserver.h"
#include "streamcodec.h"
#include "streamproducer.h"
#include "tokencache.h"
#include "webbinder.h"

#include "http/https_session.h"
#include "http/http_response.h"

#include "webproto.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

// Strict unsigned parser: no sign, no trailing garbage. Used for the stream
// endpoint's resume parameters.
bool parseUnsigned(const std::string &text, uint64_t *out)
{
    if (text.empty()) {
        *out = 0;
        return true;
    }
    if (text.front() == '-')
        return false;
    try {
        size_t used = 0;
        const unsigned long long value = std::stoull(text, &used);
        if (used != text.size())
            return false;
        *out = static_cast<uint64_t>(value);
        return true;
    } catch (const std::exception &) {
        return false;
    }
}

} // namespace

class HTTPFileSession : public NetUtil::HTTP::HTTPSSession
{
public:
    using NetUtil::HTTP::HTTPSSession::HTTPSSession;

    void setResponseHandler(ResponseHandler cb)
    {
        _handler = std::move(cb);
    }

    // Runs a producer task on a server owned worker thread.
    using TaskRunner = std::function<void(std::function<void()>)>;

    void setTaskRunner(TaskRunner runner)
    {
        _taskRunner = std::move(runner);
    }

    void setCancelCheck(std::function<bool()> check)
    {
        _cancelCheck = std::move(check);
    }

    // Producer thread. Blocks while the socket send queue is above the high
    // water mark, which is the backpressure signal for the whole pipeline:
    // disk read -> codec -> bounded queue -> socket.
    bool pushStreamData(const char *data, size_t size)
    {
        // 2 MB queued is enough to keep loopback and LAN links busy without
        // letting a slow peer grow memory without bound.
        static constexpr size_t kHighWater = 2 * 1024 * 1024;
        static constexpr size_t kChunk = 256 * 1024;

        size_t offset = 0;
        while (offset < size) {
            if (streamCancelRequested() || !IsConnected())
                return false;

            {
                std::unique_lock<std::mutex> lock(_streamMutex);
                _streamCv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return streamCancelRequested() || !IsConnected()
                            || _streamPending <= kHighWater;
                });
            }

            const size_t chunk = std::min(kChunk, size - offset);
            {
                std::lock_guard<std::mutex> lock(_streamMutex);
                _streamPending += chunk;
            }
            if (!SendResponseBodyAsync(data + offset, chunk)) {
                std::lock_guard<std::mutex> lock(_streamMutex);
                _streamPending = (_streamPending > chunk) ? (_streamPending - chunk) : 0;
                return false;
            }
            offset += chunk;
        }
        return true;
    }

    // Waits until every queued byte was handed to the socket. Disconnect()
    // clears the send buffer, so closing before this returns truncates the END
    // frame on the peer.
    bool waitStreamDrained(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(_streamMutex);
        return _streamCv.wait_for(lock, timeout, [this] {
            return _streamAborted || _streamPending == 0;
        }) && !_streamAborted;
    }

    void abortStream()
    {
        {
            std::lock_guard<std::mutex> lock(_streamMutex);
            _streamAborted = true;
        }
        _streamCv.notify_all();
    }

    bool streamAborted() const
    {
        return _streamAborted;
    }

    bool streamCancelRequested() const
    {
        return _streamAborted || (_cancelCheck && _cancelCheck());
    }

    // Queue a UI callback on the IO thread. Producer threads must never call
    // the WebInterface callbacks directly: the UI side is not thread safe.
    void postEvent(int status, const std::string &path, uint64_t value)
    {
        auto service = io_service();
        if (!service)
            return;
        auto self = std::dynamic_pointer_cast<HTTPFileSession>(shared_from_this());
        asio::post(*service, [self, status, path, value] {
            if (self->_handler)
                self->_handler(status, path.empty() ? nullptr : path.c_str(), value);
        });
    }

protected:
    InfoEntry putFileInfo(const BaseKit::Path &entry)
    {
        InfoEntry info;
        auto name = entry.filename().string();
        info.name = name;
        if (entry.IsDirectory()) {
            info.size = -1; // mask as folder flag.
        } else if (entry.IsRegularFile()) {
            BaseKit::File temp(entry);
            info.size = temp.size(); // mask as file flag.
        } else {
            std::cout << "this is link file: " << entry.string() << std::endl;
        }

        return info;
    }

    void serveInfo(const BaseKit::Path &path)
    {
        BaseKit::File info(path);
        if (info.IsExists()) {
            InfoEntry fileInfo = putFileInfo(info);
            if (info.IsDirectory()) {
                for (const auto &item : BaseKit::Directory(path)) {
                    const BaseKit::Path entry = item.IsSymlink() ? BaseKit::Symlink(item).target() : item;

                    InfoEntry subInfo = putFileInfo(entry);
                    fileInfo.datas.push_back(subInfo);
                }
            }

            std::string json_str = fileInfo.as_json().serialize();
            //std::cout << "serveInfo >>: " << json_str << std::endl;

            // Response with all dir/file values
            SendResponseAsync(response().MakeGetResponse(json_str, "application/json; charset=UTF-8"));
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
        }
    }

    void serveContent(const BaseKit::Path &path, size_t offset)
    {
        BaseKit::File info(path);
        if (info.IsExists()) {
            response().Clear();
            response().SetBegin(200);

            if (info.IsDirectory()) {
                response().SetBodyLength(0);
                response().SetBody("");

                SendResponseAsync(response());
            } else if (info.IsRegularFile()){
                const size_t sz = info.size();
                // A range beyond the end of the file would make sz - offset
                // underflow into an enormous Content-Length and leave the peer
                // waiting for bytes that never come.
                if (offset > sz) {
                    SendResponseAsync(response().MakeErrorResponse(416, "Requested range not satisfiable."));
                    _handler(RES_ERROR, nullptr, 0);
                    return;
                }

                info.Open(true, false);
                uint64_t total = 0;

                if (offset > 0) {
                    //std::cout << "breakpoint continue transfer from:" << offset << std::endl;
                    info.Seek(offset); // seek to offset for breakpoint continue
                }

                response().SetContentType(info.extension().string());
                response().SetBodyLength(sz - offset); // set the remaining size as body lenght

                total = response().body_length();

                // send headers first
                SendResponse(response());

                _handler(RES_OKHEADER, info.string().data(), total);

                bool cancel = false;
                size_t read_sz = 0;
                char buff[BLOCK_SIZE];
                while (!cancel) {
                    memset(buff, 0, BLOCK_SIZE);
                    read_sz = info.Read(buff, sizeof(buff));
                    if (read_sz <= 0) {
                        break;
                    }
                    SendResponseBody(buff, read_sz);
                    // notify progress：size total
                    // return true to cancel download from outside.
                    cancel = _handler(RES_BODY, nullptr, read_sz);
                };

                info.Close();

                _handler(RES_FINISH, info.string().data(), total);
            } else {
                std::cout << "unsupported file type: " << path.absolute() << std::endl;
                SendResponseAsync(response().MakeErrorResponse(404, "Not a regular file."));
                _handler(RES_NOTFOUND, path.string().data(), 0);
            }
        } else {
            SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
            _handler(RES_NOTFOUND, info.string().data(), 0);
        }

        //std::cout << "response body end:" << total << std::endl;
    }

    // One request = one archive stream for the whole selection. The response
    // has no Content-Length (the body ends when the connection closes), and
    // the producer runs on its own thread: this handler only sends the header.
    void serveStream(const BaseKit::Path &path, const std::string &webName,
                     dtp::Codec codec, int level, uint64_t from)
    {
        BaseKit::File info(path);
        if (!info.IsExists() || (!info.IsRegularFile() && !info.IsDirectory())) {
            SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
            _handler(RES_NOTFOUND, path.string().data(), 0);
            return;
        }

        _streaming.store(true);

        response().Clear();
        response().SetBegin(200);
        response().SetHeader("Content-Type", "application/octet-stream");
        response().SetHeader("X-DTP-Stream", "1");
        response().SetHeader("X-DTP-Version", "1");
        response().SetHeader("X-DTP-Codec", dtp::codecName(codec));
        response().SetHeader("Connection", "close");
        response().SetHeader("Cache-Control", "no-store");
        response().SetOpenEndedBody();

        if (!SendResponseAsync(response())) {
            _streaming.store(false);
            return;
        }

        auto self = std::dynamic_pointer_cast<HTTPFileSession>(shared_from_this());
        const std::string source = path.string();

        // Preserve legacy save-path semantics: the receiver names the root
        // after the requested web name, not after the server's disk path (they
        // can differ when a bind maps several names onto one directory).
        std::string displayName = webName;
        while (!displayName.empty()
               && (displayName.back() == '/' || displayName.back() == '\\'))
            displayName.pop_back();
        const size_t lastSeparator = displayName.find_last_of("/\\");
        if (lastSeparator != std::string::npos)
            displayName = displayName.substr(lastSeparator + 1);
        if (displayName.empty()) {
            std::string trimmed = source;
            while (!trimmed.empty() && (trimmed.back() == '/' || trimmed.back() == '\\'))
                trimmed.pop_back();
            const size_t sourceSeparator = trimmed.find_last_of("/\\");
            displayName = (sourceSeparator == std::string::npos)
                    ? trimmed : trimmed.substr(sourceSeparator + 1);
        }
        if (displayName.empty())
            displayName = "root";

        auto task = [self, source, displayName, codec, level, from]() {
            dtp::StreamProducerOptions options;
            options.codec = codec;
            options.level = level;
            options.skipEntries = from;
            dtp::StreamProducer producer(options);

            // The events feed the same RES_* mapping the legacy download path
            // uses, so TransferWorker's progress handling stays unchanged.
            producer.setEventCallback([self](dtp::StreamEvent event, const std::string &wirePath,
                                             const std::string &absolutePath, uint64_t value) {
                (void)wirePath;
                switch (event) {
                case dtp::StreamEvent::FileBegin:
                    self->postEvent(RES_OKHEADER, absolutePath, value);
                    break;
                case dtp::StreamEvent::FileProgress:
                    self->postEvent(RES_BODY, std::string(), value);
                    break;
                case dtp::StreamEvent::FileEnd:
                    self->postEvent(RES_FINISH, absolutePath, value);
                    break;
                }
            });

            dtp::StreamProducerStats stats;
            std::string streamError;
            const bool ok = producer.run(
                    source, displayName,
                    [self](const char *data, size_t size) {
                        return self->pushStreamData(data, size);
                    },
                    [self]() {
                        return self->streamCancelRequested() || !self->IsConnected();
                    },
                    [self](const std::string &file, const std::string &message) {
                        std::cout << "stream: " << file << " skipped: " << message << std::endl;
                    },
                    &stats, &streamError);

            if (ok) {
                // Never close with bytes still queued: Disconnect() clears the
                // send buffer and the peer would see a truncated stream.
                if (!self->waitStreamDrained(std::chrono::seconds(60)))
                    std::cout << "stream: drain timed out" << std::endl;
            } else {
                std::cout << "stream aborted: " << streamError << std::endl;
                self->abortStream();
            }

            auto service = self->io_service();
            if (service) {
                asio::post(*service, [self] {
                    if (self->IsConnected())
                        self->Disconnect();
                });
            }
        };

        if (_taskRunner)
            _taskRunner(std::move(task));
        else
            std::thread(std::move(task)).detach();
    }

    // 解析URL中的query参数
    std::unordered_map<std::string, std::string> parseQueryParams(const std::string &query)
    {
        std::unordered_map<std::string, std::string> params;

        size_t start = 0;
        size_t end = 0;

        while ((end = query.find('&', start)) != std::string::npos) {
            std::string param = query.substr(start, end - start);
            size_t eqPos = param.find('=');
            if (eqPos != std::string::npos) {
                params[param.substr(0, eqPos)] = param.substr(eqPos + 1);
            }
            start = end + 1;
        }

        std::string lastParam = query.substr(start);
        size_t eqPos = lastParam.find('=');
        if (eqPos != std::string::npos) {
            params[lastParam.substr(0, eqPos)] = lastParam.substr(eqPos + 1);
        }

        return params;
    }

    void onReceivedRequest(const NetUtil::HTTP::HTTPRequest &request) override
    {
        // Show HTTP request content
        //std::cout << std::endl << request;

        // Process HTTP request methods
        if (request.method() == "HEAD") {
            // [HEAD]webstart>|webfinish>|webindex><name>
            std::string key = std::string(request.url());
            // std::cout << "------------HEADER:" << key << std::endl;

            size_t dePos = key.find('>');
            if (dePos == std::string::npos || dePos + 1 > key.size()) {
                // std::cout << "------------HEADER:" << key << " size:" << dePos + 1 << std::endl;
                // error format request!
                SendResponseAsync(response().MakeHeadResponse());
                return;
            }

            std::string method = key.substr(0, dePos);
            if (method == s_headerInfos[INFO_WEB_START]) {
                _handler(RES_WEB_START, nullptr, 0);
            } else if (method == s_headerInfos[INFO_WEB_FINISH]) {
                _handler(RES_WEB_FINISH, nullptr, 0);
            } else if (method == s_headerInfos[INFO_WEB_INDEX]) {
                std::string name = key.substr(dePos + 1);
                BaseKit::Path diskpath = WebBinder::GetInstance().getPath(name);
                if (diskpath.empty()) {
                    std::cout << "webindex : " << name << " > " << diskpath.string() << std::endl;
                    SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
                    return;
                }

                bool found = WebBinder::GetInstance().containWeb(key);
                if (found) {
                    // this web index changed
                    _handler(RES_INDEX_CHANGE, diskpath.string().data(), 0);
                }
            }

            SendResponseAsync(response().MakeHeadResponse());
        } else if (request.method() == "GET") {
            // std::string url = "info/pathname&token=xxx";
            // std::string url = "download/pathname&token=xxx&offset=xxx";
            std::string url = std::string(request.url());
            // The parser keeps the protocol token in url(); drop it so a query
            // parameter in the last position is not parsed as "zstd HTTP/1.1".
            const size_t urlEnd = url.find(' ');
            if (urlEnd != std::string::npos)
                url.resize(urlEnd);

            const size_t tokenPos = url.find("&token");
            if (tokenPos == std::string::npos) {
                SendResponseAsync(response().MakeErrorResponse(400, "Missing token."));
                return;
            }
            std::string path = url.substr(0, tokenPos);

            size_t queryStart = tokenPos + 1;
            std::string query = url.substr(queryStart);

            std::unordered_map<std::string, std::string> queryParams = parseQueryParams(query);
            std::string ename = path.substr(path.find('/') + 1);
            // dcode realname from base64, which may include '&'
            std::string name = BaseKit::Encoding::Base64Decode(ename);
            std::string token = queryParams["token"];

            std::string method = path.substr(0, path.find('/'));

            // 检查token是否正确
            if (TokenCache::GetInstance().verifyToken(token)) {
                BaseKit::Path diskpath = WebBinder::GetInstance().getPath(name);

                if (diskpath.empty()) {
                    std::cout << "request >> name: " << name << " > " << diskpath.string() << std::endl;
                    SendResponseAsync(response().MakeErrorResponse(404, "Not found."));
                    return;
                }

                // 处理predownload或download请求的name
                if (method == "info") {
                    // 处理predownload请求的name
                    serveInfo(diskpath);
                } else if (method == "download") {
                    // 处理download请求的name
                    const std::string offstr = queryParams["offset"];
                    size_t offset = 0;
                    if (!offstr.empty()) {
                        try {
                            const long long parsed = std::stoll(offstr);
                            if (parsed < 0) {
                                SendResponseAsync(response().MakeErrorResponse(416, "Invalid offset."));
                                return;
                            }
                            offset = static_cast<size_t>(parsed);
                        } catch (const std::exception &) {
                            // Malformed offsets must not escape into the IO
                            // thread as an exception.
                            SendResponseAsync(response().MakeErrorResponse(400, "Invalid offset."));
                            return;
                        }
                    }

                    serveContent(diskpath, offset);
                } else if (method == "stream") {
                    // [GET]stream/<base64name>&token=xxx&codec=zstd&level=3
                    //                      &from=N&off=M
                    dtp::Codec codec = dtp::codecSupported(dtp::Codec::Zstd)
                            ? dtp::Codec::Zstd
                            : dtp::Codec::Deflate;
                    const std::string codecParam = queryParams["codec"];
                    if (!codecParam.empty()
                            && (!dtp::codecFromName(codecParam, &codec)
                                || !dtp::codecSupported(codec))) {
                        SendResponseAsync(response().MakeErrorResponse(400, "Unsupported codec."));
                        return;
                    }

                    int level = -1;
                    const std::string levelParam = queryParams["level"];
                    if (!levelParam.empty()) {
                        try {
                            size_t used = 0;
                            level = std::stoi(levelParam, &used);
                            if (used != levelParam.size())
                                throw std::invalid_argument("trailing characters");
                        } catch (const std::exception &) {
                            SendResponseAsync(response().MakeErrorResponse(400, "Invalid level."));
                            return;
                        }
                    }
                    level = dtp::clampCompressionLevel(codec, level);

                    uint64_t from = 0;
                    uint64_t off = 0;
                    if (!parseUnsigned(queryParams["from"], &from)
                            || !parseUnsigned(queryParams["off"], &off)) {
                        SendResponseAsync(response().MakeErrorResponse(400, "Invalid resume offset."));
                        return;
                    }
                    // v1 resumes at whole frame granularity; per-file offsets
                    // are deliberately not guessed by the sender.
                    if (off != 0) {
                        SendResponseAsync(response().MakeErrorResponse(
                                501, "Per-file resume is not supported by this server."));
                        return;
                    }

                    serveStream(diskpath, name, codec, level, from);
                } else {
                    SendResponseAsync(response().MakeErrorResponse("Unsupported HTTP request: " + method));
                }
            } else {
                std::cout << "Token invalid" << std::endl;
                // Response reject
                SendResponseAsync(response().MakeErrorResponse(404, "Invalid auth token!"));
            }
        } else
            SendResponseAsync(response().MakeErrorResponse("Unsupported HTTP method: " + std::string(request.method())));
    }

    void onReceivedRequestError(const NetUtil::HTTP::HTTPRequest &request, const std::string &error) override
    {
        std::cout << "Request error: " << error << std::endl;
        _handler(RES_ERROR, nullptr, 0);
    }

    void onError(int error, const std::string &category, const std::string &message) override
    {
//        std::cout << "HTTP session caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
        _handler(RES_ERROR, nullptr, 0);
    }

    // IO thread: the socket queue shrank, wake the producer waiting for room.
    void onSent(size_t sent, size_t pending) override
    {
        (void)sent;
        if (!_streaming.load())
            return;
        {
            std::lock_guard<std::mutex> lock(_streamMutex);
            _streamPending = pending;
        }
        _streamCv.notify_all();
    }

    void onDisconnected() override
    {
        abortStream();
        NetUtil::HTTP::HTTPSSession::onDisconnected();
    }

private:
    ResponseHandler _handler { nullptr };
    TaskRunner _taskRunner;
    std::function<bool()> _cancelCheck;

    std::atomic<bool> _streaming { false };
    std::atomic<bool> _streamAborted { false };
    std::mutex _streamMutex;
    std::condition_variable _streamCv;
    uint64_t _streamPending { 0 };  // guarded by _streamMutex
};

FileServer::~FileServer()
{
    _stop.store(true);
    if (IsStarted())
        DisconnectAll();
    joinStreamTasks();
    this->context().reset();
}

std::shared_ptr<NetUtil::Asio::SSLSession>
FileServer::CreateSession(const std::shared_ptr<NetUtil::Asio::SSLServer> &server)
{
    ResponseHandler cb([this](int status, const char *buffer, uint64_t size) -> bool {
        if (auto callback = this->callback()) {
            if (RES_OKHEADER == status) {
                std::string path(buffer);
                callback->onWebChanged(WEB_FILE_BEGIN, path, size);
            } else if (RES_BODY == status) {
                // return true to canceled from outside.
                return callback->onProgress(size);
            } else if (RES_FINISH == status) {
                std::string path(buffer);
                callback->onWebChanged(WEB_FILE_END, path, size);
            } else if (RES_NOTFOUND == status) {
                std::string path(buffer);
                callback->onWebChanged(WEB_NOT_FOUND, "not_found");
            } else if (RES_ERROR == status) {
                callback->onWebChanged(WEB_DISCONNECTED, "net_error");
            } else if (RES_INDEX_CHANGE == status) {
                std::string path(buffer);
                callback->onWebChanged(WEB_INDEX_BEGIN, path);
            } else if (RES_WEB_START == status) {
                callback->onWebChanged(WEB_TRANS_START);
            } else if (RES_WEB_FINISH == status) {
                callback->onWebChanged(WEB_TRANS_FINISH);
            }
        }
        return _stop.load();
    });

    auto session = std::make_shared<HTTPFileSession>(std::dynamic_pointer_cast<NetUtil::HTTP::HTTPSServer>(server));
    session->setResponseHandler(std::move(cb));
    session->setTaskRunner([this](std::function<void()> task) {
        runStreamTask(std::move(task));
    });
    session->setCancelCheck([this]() { return _stop.load(); });
    return session;
}

void FileServer::onError(int error, const std::string &category, const std::string &message)
{
    std::cout << "HTTP server caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
}

bool FileServer::start()
{
    _stop.store(false);
    SetupReuseAddress(true);
    SetupReusePort(true);
    // Small legacy responses (header, then first body block) otherwise hit
    // Nagle + delayed ACK: ~40 ms per file on loopback. The archive stream
    // sends large chunks, so this mainly protects the legacy fallback path.
    SetupNoDelay(true);
    return IsStarted() ? Restart() : Start();
}

bool FileServer::stop()
{
    _stop.store(true);
    const bool stopped = Stop();
    // Stop() disconnects attached sessions asynchronously; producers notice the
    // cancel flag within one 100 ms poll and exit, so joining here is bounded.
    joinStreamTasks();
    return stopped;
}

void FileServer::runStreamTask(std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(_streamTasksMutex);

    // Reap finished tasks so a long lived server does not accumulate thread
    // objects. Joining a thread that already returned is cheap.
    for (auto it = _streamTasks.begin(); it != _streamTasks.end();) {
        if (it->done->load()) {
            if (it->thread.joinable())
                it->thread.join();
            it = _streamTasks.erase(it);
        } else {
            ++it;
        }
    }

    auto done = std::make_shared<std::atomic<bool>>(false);
    StreamTask entry;
    entry.done = done;
    entry.thread = std::thread([task = std::move(task), done]() {
        task();
        done->store(true);
    });
    _streamTasks.push_back(std::move(entry));
}

void FileServer::joinStreamTasks()
{
    std::vector<StreamTask> tasks;
    {
        std::lock_guard<std::mutex> lock(_streamTasksMutex);
        tasks.swap(_streamTasks);
    }
    for (auto &task : tasks) {
        if (task.thread.joinable())
            task.thread.join();
    }
}

// bind: "/images", "C:/Users/username/Pictures/images"
// getpath(): return
//    "/images/photo.jpg" -> C:/Users/username/Pictures/images/photo.jpg
//    "/images/2022/" -> C:/Users/username/Pictures/images/2022/

// bind: "/images/*", "C:/Users/username/Pictures/"
// getpath(): return
//    "/images/photo.jpg" -> C:/Users/username/Pictures/photo.jpg
//    "/images/2022/" -> C:/Users/username/Pictures/2022/
int FileServer::webBind(std::string webDir, std::string diskDir)
{
    int bind =  WebBinder::GetInstance().bind(webDir, diskDir);
    if (bind == -1) throw std::invalid_argument("Web binding exists.");
    if (bind == -2) throw std::invalid_argument("Not a valid web path.");
    if (bind == -3) throw std::invalid_argument("Not a valid disk path.");
    if (bind == -4) throw std::invalid_argument("Not a valid combinaton of web and disk path.");
    return bind;
}

int FileServer::webUnbind(std::string webDir)
{
    return WebBinder::GetInstance().unbind(webDir);
}

void FileServer::clearBind()
{
    WebBinder::GetInstance().clear();
    TokenCache::GetInstance().clearTokens();
}

std::string FileServer::genToken(std::string info)
{
    return TokenCache::GetInstance().genToken(info);
}

bool FileServer::verifyToken(std::string &token)
{
    return TokenCache::GetInstance().verifyToken(token);
}
