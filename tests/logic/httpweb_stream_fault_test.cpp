// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Fault injection: a TCP proxy cuts the first stream connection after a few
// kilobytes. FileClient must retry with from=<completed frames>, the server
// must skip those frames, and the final tree must be byte identical.
//
// The proxy forwards TLS bytes verbatim; it never terminates TLS.

#include <gtest/gtest.h>

#include "fileserver.h"
#include "fileclient.h"
#include "tokencache.h"
#include "webbinder.h"
#include "webproto.h"

#include "asio/service.h"
#include "asio/ssl_context.h"
#include "sslcertconf.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

// Relays one connection at a time. The first accepted connection is cut after
// `cutAfter` bytes travelled from the server to the client; later connections
// are relayed until EOF.
class CuttingProxy
{
public:
    CuttingProxy(int listenPort, int targetPort, size_t cutAfter, bool stallFirst = false)
        : _listenPort(listenPort)
        , _targetPort(targetPort)
        , _cutAfter(cutAfter)
        , _stallFirst(stallFirst)
    {
    }

    ~CuttingProxy()
    {
        stop();
    }

    bool start()
    {
        _listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (_listenFd < 0)
            return false;
        int yes = 1;
        ::setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(_listenPort));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::bind(_listenFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
            return false;
        if (::listen(_listenFd, 8) != 0)
            return false;

        _thread = std::thread([this]() { run(); });
        return true;
    }

    void stop()
    {
        _stop.store(true);
        if (_thread.joinable())
            _thread.join();
        if (_listenFd >= 0) {
            ::close(_listenFd);
            _listenFd = -1;
        }
    }

    int connections() const { return _connections.load(); }

private:
    void run()
    {
        bool first = true;
        while (!_stop.load()) {
            pollfd pfd {};
            pfd.fd = _listenFd;
            pfd.events = POLLIN;
            const int ready = ::poll(&pfd, 1, 100);
            if (ready <= 0)
                continue;

            const int client = ::accept(_listenFd, nullptr, nullptr);
            if (client < 0)
                continue;
            _connections.fetch_add(1);
            handle(client, first);
            first = false;
        }
    }

    void handle(int client, bool first)
    {
        const int server = ::socket(AF_INET, SOCK_STREAM, 0);
        if (server < 0) {
            ::close(client);
            return;
        }
        sockaddr_in address {};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(_targetPort));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(server, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
            ::close(server);
            ::close(client);
            return;
        }

        std::atomic<bool> stop { false };
        const auto relay = [&stop](int from, int to, bool cut, size_t limit, bool stall) {
            std::vector<char> buffer(16 * 1024);
            size_t forwarded = 0;
            while (!stop.load()) {
                const ssize_t got = ::recv(from, buffer.data(), buffer.size(), 0);
                if (got <= 0)
                    break;
                if (cut) {
                    forwarded += static_cast<size_t>(got);
                    if (forwarded > limit) {
                        // Keep the socket open without forwarding: the client's
                        // idle watchdog must abort and retry.
                        while (stall && !stop.load())
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        break;
                    }
                }
                size_t offset = 0;
                while (offset < static_cast<size_t>(got)) {
                    const ssize_t sent = ::send(to, buffer.data() + offset,
                                                static_cast<size_t>(got) - offset, MSG_NOSIGNAL);
                    if (sent <= 0) {
                        stop.store(true);
                        break;
                    }
                    offset += static_cast<size_t>(sent);
                }
            }
            stop.store(true);
            ::shutdown(from, SHUT_RDWR);
            ::shutdown(to, SHUT_RDWR);
        };

        std::thread up(relay, client, server, false, 0, false);
        std::thread down(relay, server, client, first, _cutAfter, first && _stallFirst);
        up.join();
        down.join();
        ::close(client);
        ::close(server);
    }

    int _listenPort;
    int _targetPort;
    size_t _cutAfter;
    bool _stallFirst;
    int _listenFd { -1 };
    std::thread _thread;
    std::atomic<bool> _stop { false };
    std::atomic<int> _connections { 0 };
};

class StreamFaultTest : public ::testing::Test
{
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> srvCtx;
    std::shared_ptr<NetUtil::Asio::SSLContext> cliCtx;
    std::shared_ptr<FileServer> server;
    std::unique_ptr<CuttingProxy> proxy;
    fs::path srvDir;
    const int serverPort = 19001;
    const int proxyPort = 19000;

    void SetUp() override
    {
        srvDir = fs::temp_directory_path() / ("dtp_fault_" + std::to_string(::getpid()));
        fs::remove_all(srvDir);
        fs::create_directories(srvDir);

        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();

        SslCertConf::ins()->generateCertificate(srvDir.string());
        const std::string certPath = (srvDir / "SSL" / "Barrier.pem").string();

        service = std::make_shared<NetUtil::Asio::Service>();
        service->Start();

        srvCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        srvCtx->use_certificate_chain_file(certPath);
        srvCtx->use_private_key_file(certPath, asio::ssl::context::pem);

        cliCtx = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv12);
        cliCtx->set_verify_mode(asio::ssl::verify_none);
        try {
            cliCtx->load_verify_file(certPath);
        } catch (...) {
        }

        server = std::make_shared<FileServer>(service, srvCtx, "127.0.0.1", serverPort);
    }

    void TearDown() override
    {
        ::setenv("DDE_COOPERATION_HTTPWEB_STREAM_IDLE_TIMEOUT", "", 1);
        if (proxy)
            proxy->stop();
        proxy.reset();
        if (server && server->IsStarted())
            server->stop();
        service->Stop();
        server.reset();
        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();
        std::error_code ec;
        fs::remove_all(srvDir, ec);
    }

    static void writeRandomFile(const fs::path &path, size_t size)
    {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        uint32_t state = 0x12345678;
        std::string block;
        block.resize(64 * 1024);
        for (size_t written = 0; written < size;) {
            for (size_t i = 0; i < block.size(); ++i) {
                state = state * 1664525u + 1013904223u;
                block[i] = static_cast<char>(state >> 24);
            }
            const size_t chunk = std::min(block.size(), size - written);
            out.write(block.data(), static_cast<std::streamsize>(chunk));
            written += chunk;
        }
    }

    class WaitCallback : public ProgressCallInterface
    {
    public:
        std::promise<void> done;
        std::atomic<int> lastState { 0 };
        bool onProgress(uint64_t) override { return false; }
        void onWebChanged(int state, std::string, uint64_t) override
        {
            lastState.store(state);
            if (state == WEB_TRANS_FINISH || state == WEB_DISCONNECTED || state == WEB_IO_ERROR) {
                try {
                    done.set_value();
                } catch (...) {
                }
            }
        }
    };
};

} // namespace

TEST_F(StreamFaultTest, TruncatedStreamResumesAndCompletes)
{
    const fs::path sourceFile = srvDir / "files" / "big.bin";
    writeRandomFile(sourceFile, 2 * 1024 * 1024);

    ASSERT_NO_THROW(server->webBind("/files", (srvDir / "files").string()));
    ASSERT_TRUE(server->start());

    // 100 KB of server->client bytes cuts the first connection in the middle
    // of big.bin; the retry uses a fresh connection and is not cut.
    proxy = std::make_unique<CuttingProxy>(proxyPort, serverPort, 100 * 1024);
    ASSERT_TRUE(proxy->start());

    const std::string token = server->genToken(R"(["/files"])");
    const fs::path dst = srvDir / "dst";
    fs::create_directories(dst);

    auto callback = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", proxyPort);
    client.setCallback(callback);
    client.setConfig(token, dst.string());
    client.startFileDownload({ "/files" });

    auto future = callback->done.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::seconds(60)), std::future_status::ready);
    client.stop();

    EXPECT_EQ(callback->lastState.load(), WEB_TRANS_FINISH);
    EXPECT_GE(proxy->connections(), 2);

    const fs::path downloaded = dst / "files" / "big.bin";
    ASSERT_TRUE(fs::exists(downloaded));
    ASSERT_EQ(fs::file_size(downloaded), fs::file_size(sourceFile));

    std::ifstream expected(sourceFile, std::ios::binary);
    std::ifstream actual(downloaded, std::ios::binary);
    const std::string expectedBytes((std::istreambuf_iterator<char>(expected)),
                                    std::istreambuf_iterator<char>());
    const std::string actualBytes((std::istreambuf_iterator<char>(actual)),
                                  std::istreambuf_iterator<char>());
    EXPECT_EQ(actualBytes, expectedBytes);

    size_t parts = 0;
    for (const auto &entry : fs::recursive_directory_iterator(dst)) {
        if (entry.is_regular_file() && entry.path().extension() == ".part")
            ++parts;
    }
    EXPECT_EQ(parts, size_t(0));
}

TEST_F(StreamFaultTest, StalledStreamIsRetriedByWatchdog)
{
    ::setenv("DDE_COOPERATION_HTTPWEB_STREAM_IDLE_TIMEOUT", "1", 1);

    const fs::path sourceFile = srvDir / "files" / "big.bin";
    writeRandomFile(sourceFile, 512 * 1024);

    ASSERT_NO_THROW(server->webBind("/files", (srvDir / "files").string()));
    ASSERT_TRUE(server->start());

    // After 100 KB the proxy stops forwarding but keeps the connection open.
    // The 1 s idle watchdog must abort, reconnect and resume.
    proxy = std::make_unique<CuttingProxy>(proxyPort, serverPort, 100 * 1024, true);
    ASSERT_TRUE(proxy->start());

    const std::string token = server->genToken(R"(["/files"])");
    const fs::path dst = srvDir / "dst";
    fs::create_directories(dst);

    auto callback = std::make_shared<WaitCallback>();
    FileClient client(service, cliCtx, "127.0.0.1", proxyPort);
    client.setCallback(callback);
    client.setConfig(token, dst.string());
    client.startFileDownload({ "/files" });

    auto future = callback->done.get_future();
    ASSERT_EQ(future.wait_for(std::chrono::seconds(60)), std::future_status::ready);
    client.stop();

    EXPECT_EQ(callback->lastState.load(), WEB_TRANS_FINISH);
    EXPECT_GE(proxy->connections(), 2);

    const fs::path downloaded = dst / "files" / "big.bin";
    ASSERT_TRUE(fs::exists(downloaded));
    ASSERT_EQ(fs::file_size(downloaded), fs::file_size(sourceFile));

    std::ifstream expected(sourceFile, std::ios::binary);
    std::ifstream actual(downloaded, std::ios::binary);
    const std::string expectedBytes((std::istreambuf_iterator<char>(expected)),
                                    std::istreambuf_iterator<char>());
    const std::string actualBytes((std::istreambuf_iterator<char>(actual)),
                                  std::istreambuf_iterator<char>());
    EXPECT_EQ(actualBytes, expectedBytes);
}
