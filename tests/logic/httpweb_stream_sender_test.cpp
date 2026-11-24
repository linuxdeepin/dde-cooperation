// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// End to end test for the DTP2 sender endpoint: a real FileServer over TLS is
// asked for stream/<name>, and the raw response is decoded exactly like a
// receiver would. This covers the HTTP framing (open ended body), the worker
// thread handoff and the producer pipeline together.

#include <gtest/gtest.h>

#include "fileserver.h"
#include "streamcodec.h"
#include "streamframe.h"
#include "tokencache.h"
#include "webbinder.h"

#include "asio/service.h"
#include "asio/ssl_client.h"
#include "asio/ssl_context.h"
#include "sslcertconf.h"
#include "string/encoding.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace dtp;
namespace fs = std::filesystem;

namespace {

constexpr int kTestPort = 18457;

class StreamSenderTest : public ::testing::Test
{
protected:
    std::shared_ptr<NetUtil::Asio::Service> service;
    std::shared_ptr<NetUtil::Asio::SSLContext> srvCtx;
    std::shared_ptr<NetUtil::Asio::SSLContext> cliCtx;
    std::shared_ptr<FileServer> server;
    fs::path srvDir;

    void SetUp() override
    {
        srvDir = fs::temp_directory_path() / ("dtp_sender_" + std::to_string(::getpid()));
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

        server = std::make_shared<FileServer>(service, srvCtx, "127.0.0.1", kTestPort);
    }

    void TearDown() override
    {
        if (server && server->IsStarted())
            server->stop();
        service->Stop();
        server.reset();
        WebBinder::GetInstance().clear();
        TokenCache::GetInstance().clearTokens();
        std::error_code ec;
        fs::remove_all(srvDir, ec);
    }

    void writeFile(const fs::path &path, const std::string &content)
    {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    struct RawResponse {
        bool connected {false};
        std::string header;
        std::string body;
        std::string error;
    };

    // Reads a response: stream bodies end at EOF, error bodies at their
    // Content-Length (the legacy server keeps the connection open there).
    static std::string exchange(const std::shared_ptr<NetUtil::Asio::SSLClient> &client,
                                const std::string &request,
                                const BaseKit::Timespan &timeout)
    {
        std::string raw;
        if (client->Send(request) != request.size())
            return raw;

        char buffer[64 * 1024];
        for (;;) {
            const size_t got = client->Receive(buffer, sizeof(buffer), timeout);
            if (got == 0)
                break;
            raw.append(buffer, got);

            const size_t headerEnd = raw.find("\r\n\r\n");
            if (headerEnd == std::string::npos)
                continue;
            const size_t lengthPos = raw.find("Content-Length: ");
            if (lengthPos == std::string::npos)
                continue;
            const size_t valueStart = lengthPos + 16;
            const size_t valueEnd = raw.find("\r\n", valueStart);
            if (valueEnd == std::string::npos)
                continue;
            const size_t length = static_cast<size_t>(
                    std::strtoull(raw.substr(valueStart, valueEnd - valueStart).c_str(),
                                  nullptr, 10));
            if (raw.size() >= headerEnd + 4 + length)
                break;
        }
        return raw;
    }

    // Sends one GET and reads until the server closes the connection, which is
    // the v1 end-of-stream marker.
    RawResponse get(const std::string &name, const std::string &query)
    {
        RawResponse result;
        // shared_ptr is required: SSLClient::Disconnect() calls
        // shared_from_this() when the server closes the connection.
        // server->start() posts the acceptor creation asynchronously; retry
        // with a fresh client until the listener is ready.
        std::shared_ptr<NetUtil::Asio::SSLClient> client;
        for (int attempt = 0; attempt < 50; ++attempt) {
            client = std::make_shared<NetUtil::Asio::SSLClient>(
                    service, cliCtx, "127.0.0.1", kTestPort);
            if (client->Connect())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!client || !client->IsConnected()) {
            result.error = "connect failed";
            return result;
        }
        result.connected = true;

        const std::string request =
                "GET stream/" + BaseKit::Encoding::Base64Encode(name)
                + "&token=" + server->genToken(R"(["/files/"])") + query
                + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
        const std::string raw = exchange(client, request, BaseKit::Timespan::seconds(30));
        const size_t split = raw.find("\r\n\r\n");
        if (split == std::string::npos) {
            result.error = "no header terminator";
            return result;
        }
        result.header = raw.substr(0, split);
        result.body = raw.substr(split + 4);
        return result;
    }

    struct Parsed {
        bool ok {false};
        std::map<std::string, std::string> files;
        std::vector<std::string> dirs;
        FrameEnd end;
        std::string error;
    };

    static Parsed decode(const std::string &body)
    {
        Parsed parsed;
        if (body.size() < kStreamPreambleSize) {
            parsed.error = "no preamble";
            return parsed;
        }
        StreamPreamble preamble;
        if (!decodePreamble(body.data(), body.size(), &preamble, &parsed.error))
            return parsed;

        StreamDecoder decoder;
        if (!decoder.init(preamble.codec)) {
            parsed.error = decoder.lastError();
            return parsed;
        }
        std::string raw;
        if (!decoder.decode(body.data() + kStreamPreambleSize,
                            body.size() - kStreamPreambleSize, &raw)) {
            parsed.error = decoder.lastError();
            return parsed;
        }

        FrameParser parser;
        std::string currentPath;
        parser.onHeader([&](const FrameHeader &header) {
            if (header.type == FrameType::Dir || header.type == FrameType::RootDir) {
                parsed.dirs.push_back(header.path);
            } else if (header.type == FrameType::File || header.type == FrameType::RootFile) {
                currentPath = header.path;
                parsed.files[header.path];
            }
            return true;
        });
        parser.onData([&](const char *data, size_t size) {
            parsed.files[currentPath].append(data, size);
            return true;
        });
        parser.onEnd([&](const FrameEnd &end) { parsed.end = end; return true; });
        if (!parser.feed(raw.data(), raw.size())) {
            parsed.error = parser.lastError();
            return parsed;
        }
        parsed.ok = parser.finished();
        if (!parsed.ok)
            parsed.error = "stream has no END frame";
        return parsed;
    }
};

} // namespace

TEST_F(StreamSenderTest, DirectoryTreeRoundTripWithZstd)
{
    if (!codecSupported(Codec::Zstd))
        GTEST_SKIP() << "zstd not compiled in";

    writeFile(srvDir / "files" / "a.txt", "alpha");
    writeFile(srvDir / "files" / "sub" / "b.bin", std::string(70000, 'b'));
    writeFile(srvDir / "files" / "empty.txt", "");
    fs::create_directories(srvDir / "files" / "sub" / "emptydir");

    ASSERT_NO_THROW(server->webBind("/files/", (srvDir / "files").string() + "/"));
    ASSERT_TRUE(server->start());

    const RawResponse response = get("/files/", "&codec=zstd");
    ASSERT_TRUE(response.error.empty()) << response.error;

    EXPECT_NE(response.header.find("HTTP/1.1 200 OK"), std::string::npos) << response.header;
    EXPECT_NE(response.header.find("X-DTP-Stream: 1"), std::string::npos) << response.header;
    EXPECT_NE(response.header.find("X-DTP-Version: 1"), std::string::npos) << response.header;
    EXPECT_NE(response.header.find("X-DTP-Codec: zstd"), std::string::npos) << response.header;
    EXPECT_NE(response.header.find("Connection: close"), std::string::npos) << response.header;
    // Open ended body: a Content-Length would make the receiver stop early.
    EXPECT_EQ(response.header.find("Content-Length"), std::string::npos) << response.header;

    const Parsed parsed = decode(response.body);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.at("a.txt"), "alpha");
    EXPECT_EQ(parsed.files.at("sub/b.bin"), std::string(70000, 'b'));
    EXPECT_TRUE(parsed.files.count("empty.txt") == 1);
    EXPECT_EQ(parsed.files.at("empty.txt"), "");
    EXPECT_EQ(parsed.dirs.size(), size_t(3));  // root + sub + emptydir
    EXPECT_EQ(parsed.end.totalBytes, 70005u);
}

TEST_F(StreamSenderTest, SingleFileWithDeflate)
{
    writeFile(srvDir / "files" / "movie.bin", std::string(4096, 'm'));

    ASSERT_NO_THROW(server->webBind("/files/", (srvDir / "files").string() + "/"));
    ASSERT_TRUE(server->start());

    const RawResponse response = get("/files/movie.bin", "&codec=deflate");
    ASSERT_TRUE(response.error.empty()) << response.error;
    EXPECT_NE(response.header.find("X-DTP-Codec: deflate"), std::string::npos) << response.header;

    const Parsed parsed = decode(response.body);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.at("movie.bin"), std::string(4096, 'm'));
    EXPECT_EQ(parsed.end.totalBytes, 4096u);
}

TEST_F(StreamSenderTest, UnsupportedCodecIsRejectedBeforeAnyBytes)
{
    writeFile(srvDir / "files" / "a.txt", "a");
    ASSERT_NO_THROW(server->webBind("/files/", (srvDir / "files").string() + "/"));
    ASSERT_TRUE(server->start());

    const RawResponse response = get("/files/", "&codec=lzma");
    ASSERT_TRUE(response.error.empty()) << response.error;
    EXPECT_NE(response.header.find("400"), std::string::npos) << response.header;
    EXPECT_TRUE(response.header.find("X-DTP-Stream") == std::string::npos) << response.header;
}

TEST_F(StreamSenderTest, PerFileOffsetIsRejectedByV1Server)
{
    writeFile(srvDir / "files" / "a.txt", "a");
    ASSERT_NO_THROW(server->webBind("/files/", (srvDir / "files").string() + "/"));
    ASSERT_TRUE(server->start());

    // v1 resumes at whole-frame granularity; an in-file offset is not guessed.
    const RawResponse response = get("/files/", "&off=3");
    ASSERT_TRUE(response.error.empty()) << response.error;
    EXPECT_NE(response.header.find("501"), std::string::npos) << response.header;
}

TEST_F(StreamSenderTest, FromSkipsCompletedFrames)
{
    writeFile(srvDir / "files" / "a.txt", "alpha");
    writeFile(srvDir / "files" / "sub" / "b.txt", "beta");

    ASSERT_NO_THROW(server->webBind("/files/", (srvDir / "files").string() + "/"));
    ASSERT_TRUE(server->start());

    // Frame order: RootDir, a.txt, Dir(sub), sub/b.txt. from=2 must skip the
    // root and a.txt, and start with the sub directory.
    const RawResponse response = get("/files/", "&codec=deflate&from=2");
    ASSERT_TRUE(response.error.empty()) << response.error;

    const Parsed parsed = decode(response.body);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.size(), size_t(1));
    EXPECT_EQ(parsed.files.at("sub/b.txt"), "beta");
    EXPECT_EQ(parsed.dirs.size(), size_t(1));
    EXPECT_EQ(parsed.dirs.front(), "sub");
    EXPECT_EQ(parsed.end.totalBytes, 4u);
}

TEST_F(StreamSenderTest, MissingTokenIsRejected)
{
    writeFile(srvDir / "files" / "a.txt", "a");
    ASSERT_NO_THROW(server->webBind("/files/", (srvDir / "files").string() + "/"));
    ASSERT_TRUE(server->start());

    auto client = std::make_shared<NetUtil::Asio::SSLClient>(
            service, cliCtx, "127.0.0.1", kTestPort);
    bool connected = false;
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (client->Connect()) {
            connected = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(connected);
    const std::string request =
            "GET stream/" + BaseKit::Encoding::Base64Encode("/files/")
            + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    const std::string raw = exchange(client, request, BaseKit::Timespan::seconds(10));
    EXPECT_NE(raw.find("400"), std::string::npos) << raw;
}
