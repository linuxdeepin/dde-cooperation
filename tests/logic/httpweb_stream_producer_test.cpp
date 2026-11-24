// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "streamproducer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace dtp;
namespace fs = std::filesystem;

namespace {

class StreamProducerTest : public ::testing::Test
{
protected:
    fs::path root;

    void SetUp() override
    {
        static int counter = 0;
        root = fs::temp_directory_path()
                / ("dtp_producer_" + std::to_string(::getpid()) + "_" + std::to_string(++counter));
        fs::remove_all(root);
        fs::create_directories(root);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(root, ec);
    }

    void writeFile(const fs::path &path, const std::string &content)
    {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    struct Parsed {
        bool ok {false};
        std::map<std::string, std::string> files;
        std::vector<std::string> dirs;
        std::vector<std::string> errors;
        FrameEnd end;
        std::string error;
    };

    // Decodes an archive the same way a receiver would.
    Parsed parse(const std::string &archive)
    {
        Parsed parsed;
        if (archive.size() < kStreamPreambleSize) {
            parsed.error = "no preamble";
            return parsed;
        }
        StreamPreamble preamble;
        if (!decodePreamble(archive.data(), archive.size(), &preamble, &parsed.error))
            return parsed;

        StreamDecoder decoder;
        if (!decoder.init(preamble.codec)) {
            parsed.error = decoder.lastError();
            return parsed;
        }
        std::string raw;
        if (!decoder.decode(archive.data() + kStreamPreambleSize,
                            archive.size() - kStreamPreambleSize, &raw)) {
            parsed.error = decoder.lastError();
            return parsed;
        }
        if (!decoder.finished() && preamble.codec != Codec::None) {
            parsed.error = "decoder did not finish";
            return parsed;
        }

        FrameParser parser;
        std::string currentPath;
        parser.onHeader([&](const FrameHeader &header) {
            if (header.type == FrameType::Dir || header.type == FrameType::RootDir) {
                parsed.dirs.push_back(header.path);
            } else if (header.type == FrameType::File || header.type == FrameType::RootFile) {
                // Register the path before any data: an empty file produces no
                // data callback at all.
                currentPath = header.path;
                parsed.files[header.path];
            }
            return true;
        });
        parser.onData([&](const char *data, size_t size) {
            parsed.files[currentPath].append(data, size);
            return true;
        });
        parser.onError([&](const std::string &path, const std::string &message) {
            parsed.errors.push_back(path + "|" + message);
            return true;
        });
        parser.onEnd([&](const FrameEnd &end) { parsed.end = end; return true; });
        if (!parser.feed(raw.data(), raw.size())) {
            parsed.error = parser.lastError();
            return parsed;
        }
        parsed.ok = parser.finished();
        if (!parsed.ok && parsed.error.empty())
            parsed.error = "stream has no END frame";
        return parsed;
    }

    struct RunResult {
        bool ok {false};
        std::string archive;
        StreamProducerStats stats;
        std::string error;
        std::vector<std::string> reported;
    };

    RunResult produce(const fs::path &source, const std::string &name,
                      StreamProducerOptions options = {}, int cancelAfterSinks = -1,
                      int sinkRefuseAfter = -1)
    {
        RunResult result;
        StreamProducer producer(options);
        int sinks = 0;
        const bool ok = producer.run(
                source.u8string(), name,
                [&](const char *data, size_t size) {
                    if (sinkRefuseAfter >= 0 && sinks >= sinkRefuseAfter)
                        return false;
                    ++sinks;
                    result.archive.append(data, size);
                    return true;
                },
                [&]() { return cancelAfterSinks >= 0 && sinks >= cancelAfterSinks; },
                [&](const std::string &path, const std::string &message) {
                    result.reported.push_back(path + "|" + message);
                },
                &result.stats, &result.error);
        result.ok = ok;
        return result;
    }
};

} // namespace

TEST_F(StreamProducerTest, ProducesAParseableArchive)
{
    writeFile(root / "src" / "a.txt", "alpha");
    writeFile(root / "src" / "sub" / "b.bin", std::string(5000, 'b'));
    writeFile(root / "src" / "empty.txt", "");
    fs::create_directories(root / "src" / "sub" / "emptydir");

    const RunResult run = produce(root / "src", "src");
    ASSERT_TRUE(run.ok) << run.error;

    const Parsed parsed = parse(run.archive);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.at("a.txt"), "alpha");
    EXPECT_EQ(parsed.files.at("sub/b.bin"), std::string(5000, 'b'));
    EXPECT_TRUE(parsed.files.count("empty.txt") == 1);
    ASSERT_EQ(parsed.files.at("empty.txt"), "");
    EXPECT_EQ(parsed.dirs.size(), size_t(3));  // root, sub, sub/emptydir
    EXPECT_EQ(parsed.end.entryCount, run.stats.frames);
    EXPECT_EQ(run.stats.files, 3u);
    EXPECT_EQ(run.stats.bytesRead, 5005u);
    EXPECT_GT(run.stats.bytesSent, 0u);
}

TEST_F(StreamProducerTest, OutputIsDeterministic)
{
    writeFile(root / "src" / "z.txt", "z");
    writeFile(root / "src" / "a.txt", "a");
    fs::create_directories(root / "src" / "m");

    const RunResult first = produce(root / "src", "src");
    const RunResult second = produce(root / "src", "src");
    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.archive, second.archive);
}

TEST_F(StreamProducerTest, WorksWithEverySupportedCodec)
{
    writeFile(root / "src" / "a.txt", "alpha");

    for (Codec codec : {Codec::None, Codec::Deflate, Codec::Zstd}) {
        if (!codecSupported(codec))
            continue;
        StreamProducerOptions options;
        options.codec = codec;
        const RunResult run = produce(root / "src", "src", options);
        ASSERT_TRUE(run.ok) << codecName(codec) << ": " << run.error;
        const Parsed parsed = parse(run.archive);
        ASSERT_TRUE(parsed.ok) << codecName(codec) << ": " << parsed.error;
        EXPECT_EQ(parsed.files.at("a.txt"), "alpha") << codecName(codec);
    }
}

TEST_F(StreamProducerTest, RootFileArchive)
{
    writeFile(root / "movie.bin", std::string(2048, 'm'));
    const RunResult run = produce(root / "movie.bin", "movie.bin");
    ASSERT_TRUE(run.ok) << run.error;

    const Parsed parsed = parse(run.archive);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.at("movie.bin"), std::string(2048, 'm'));
    EXPECT_EQ(parsed.end.totalBytes, 2048u);
}

TEST_F(StreamProducerTest, LargeFileIsChunkedCorrectly)
{
    std::string content;
    content.reserve(1024 * 1024);
    for (int i = 0; i < 1024 * 1024; ++i)
        content.push_back(static_cast<char>('a' + (i % 26)));
    writeFile(root / "src" / "big.bin", content);

    StreamProducerOptions options;
    options.readBlock = 4096;
    const RunResult run = produce(root / "src", "src", options);
    ASSERT_TRUE(run.ok) << run.error;

    const Parsed parsed = parse(run.archive);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.at("big.bin"), content);
    EXPECT_EQ(run.stats.bytesRead, content.size());
}

TEST_F(StreamProducerTest, CancellationEmitsNoEndFrame)
{
    writeFile(root / "src" / "a.txt", std::string(100000, 'a'));
    writeFile(root / "src" / "b.txt", "b");

    const RunResult run = produce(root / "src", "src", {}, 1);
    EXPECT_FALSE(run.ok);
    EXPECT_EQ(run.error, "cancelled");

    const Parsed parsed = parse(run.archive);
    EXPECT_FALSE(parsed.ok);  // no END frame
}

TEST_F(StreamProducerTest, SinkRefusalAbortsTheStream)
{
    writeFile(root / "src" / "a.txt", std::string(100000, 'a'));
    const RunResult run = produce(root / "src", "src", {}, -1, 1);
    EXPECT_FALSE(run.ok);
    EXPECT_FALSE(run.error.empty());
}

#ifndef _WIN32
TEST_F(StreamProducerTest, UnreadableFileBecomesAnErrorFrame)
{
    writeFile(root / "src" / "ok.txt", "ok");
    writeFile(root / "src" / "locked.txt", "secret");
    ASSERT_EQ(::chmod((root / "src" / "locked.txt").c_str(), 0), 0);

    const RunResult run = produce(root / "src", "src");
    ::chmod((root / "src" / "locked.txt").c_str(), 0600);

    ASSERT_TRUE(run.ok) << run.error;
    const Parsed parsed = parse(run.archive);
    ASSERT_TRUE(parsed.ok) << parsed.error;
    EXPECT_EQ(parsed.files.at("ok.txt"), "ok");
    EXPECT_EQ(parsed.files.count("locked.txt"), size_t(0));
    ASSERT_EQ(parsed.errors.size(), size_t(1));
    EXPECT_NE(parsed.errors[0].find("locked.txt"), std::string::npos);
}
#endif
