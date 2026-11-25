// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "streamcodec.h"
#include "streamconsumer.h"
#include "streamframe.h"
#include "streamproducer.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace dtp;
namespace fs = std::filesystem;

namespace {

class StreamConsumerTest : public ::testing::Test
{
protected:
    fs::path root;

    void SetUp() override
    {
        static int counter = 0;
        root = fs::temp_directory_path()
                / ("dtp_consumer_" + std::to_string(::getpid()) + "_" + std::to_string(++counter));
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

    struct Archive {
        bool ok {false};
        std::string bytes;
        std::string error;
    };

    Archive produce(const fs::path &source, const std::string &name,
                    Codec codec = Codec::Deflate, int level = -1,
                    bool preserveLinks = false)
    {
        Archive archive;
        StreamProducerOptions options;
        options.codec = codec;
        options.level = level;
        options.walker.preserveSymlinks = preserveLinks;
        options.walker.followSymlinks = !preserveLinks;
        StreamProducer producer(options);
        StreamProducerStats stats;
        archive.ok = producer.run(
                source.u8string(), name,
                [&archive](const char *data, size_t size) {
                    archive.bytes.append(data, size);
                    return true;
                },
                [] { return false; },
                [](const std::string &, const std::string &) {},
                &stats, &archive.error);
        return archive;
    }

    static void feedPattern(StreamConsumer &consumer, const std::string &bytes,
                            const std::vector<size_t> &pattern)
    {
        size_t offset = 0;
        size_t index = 0;
        while (offset < bytes.size()) {
            const size_t size = std::min(pattern[index % pattern.size()], bytes.size() - offset);
            ASSERT_TRUE(consumer.feed(bytes.data() + offset, size));
            offset += size;
            ++index;
        }
    }

    static std::map<std::string, std::string> readTree(const fs::path &base)
    {
        std::map<std::string, std::string> files;
        std::error_code ec;
        for (auto it = fs::recursive_directory_iterator(base, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (!it->is_regular_file())
                continue;
            std::ifstream in(it->path(), std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
            files[it->path().lexically_relative(base).generic_u8string()] = content;
        }
        return files;
    }

    static size_t countParts(const fs::path &base)
    {
        size_t count = 0;
        std::error_code ec;
        for (auto it = fs::recursive_directory_iterator(base, ec);
             !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (it->is_regular_file() && it->path().extension() == ".part")
                ++count;
        }
        return count;
    }
};

} // namespace

TEST_F(StreamConsumerTest, RoundTripThroughOneByteChunks)
{
    writeFile(root / "src" / "a.txt", "alpha");
    writeFile(root / "src" / "sub" / "b.bin", std::string(5000, 'b'));
    writeFile(root / "src" / "empty.txt", "");
    writeFile(root / "src" / u8"文档" / u8"é.txt", "unicode");
    fs::create_directories(root / "src" / "sub" / "emptydir");

    const Archive archive = produce(root / "src", "src");
    ASSERT_TRUE(archive.ok) << archive.error;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    feedPattern(consumer, archive.bytes, {1, 3, 7, 2, 11});

    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;
    EXPECT_EQ(consumer.status(), StreamConsumer::Status::Ok);

    const fs::path out = consumer.rootPath();
    EXPECT_EQ(out.filename().u8string(), "src");
    EXPECT_TRUE(fs::is_directory(out / "sub" / "emptydir"));

    const auto files = readTree(out);
    EXPECT_EQ(files.at("a.txt"), "alpha");
    EXPECT_EQ(files.at("sub/b.bin"), std::string(5000, 'b'));
    ASSERT_EQ(files.count("empty.txt"), size_t(1));
    EXPECT_EQ(files.at("empty.txt"), "");
    EXPECT_EQ(files.at(u8"文档/é.txt"), "unicode");

    EXPECT_EQ(consumer.stats().files, 4u);
    EXPECT_EQ(consumer.stats().directories, 4u);
    EXPECT_EQ(consumer.stats().bytes, 5012u);  // 5 + 5000 + 0 + 7
    EXPECT_EQ(countParts(target), size_t(0));
}

#ifndef _WIN32
TEST_F(StreamConsumerTest, RoundTripPreservesSymlinks)
{
    writeFile(root / "src" / "real.txt", "payload");
    fs::create_directories(root / "src" / "sub");
    std::error_code ec;
    fs::create_symlink("../real.txt", root / "src" / "sub" / "file-link", ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_directory_symlink(root / "src" / "sub", root / "src" / "dir-link", ec);
    ASSERT_FALSE(ec) << ec.message();

    const Archive archive = produce(root / "src", "src", Codec::Deflate, -1, true);
    ASSERT_TRUE(archive.ok) << archive.error;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    feedPattern(consumer, archive.bytes, {3, 17, 1});

    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;
    const fs::path out = consumer.rootPath();

    ASSERT_TRUE(fs::is_symlink(out / "sub" / "file-link"));
    EXPECT_EQ(fs::read_symlink(out / "sub" / "file-link").generic_u8string(), "../real.txt");
    EXPECT_EQ(fs::read_symlink(out / "sub" / "file-link").generic_u8string(),
              fs::read_symlink(root / "src" / "sub" / "file-link").generic_u8string());

    ASSERT_TRUE(fs::is_symlink(out / "dir-link"));
    EXPECT_TRUE(fs::is_directory(out / "dir-link"));
    EXPECT_EQ(fs::read_symlink(out / "dir-link").generic_u8string(), "sub");

    EXPECT_EQ(consumer.stats().files, 1u);
    EXPECT_EQ(consumer.stats().symlinks, 2u);
    EXPECT_EQ(consumer.stats().bytes, 7u);
    EXPECT_EQ(countParts(target), size_t(0));
}
#endif

TEST_F(StreamConsumerTest, RoundTripWithZstd)
{
    if (!codecSupported(Codec::Zstd))
        GTEST_SKIP() << "zstd not compiled in";

    writeFile(root / "src" / "a.txt", std::string(200000, 'a'));
    const Archive archive = produce(root / "src", "src", Codec::Zstd);
    ASSERT_TRUE(archive.ok) << archive.error;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    feedPattern(consumer, archive.bytes, {4096, 333, 65536});

    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;
    const auto files = readTree(consumer.rootPath());
    EXPECT_EQ(files.at("a.txt"), std::string(200000, 'a'));
}

TEST_F(StreamConsumerTest, RootFileIsRestoredAtTopLevel)
{
    writeFile(root / "movie.bin", std::string(4096, 'm'));
    const Archive archive = produce(root / "movie.bin", "movie.bin");
    ASSERT_TRUE(archive.ok);

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    ASSERT_TRUE(consumer.feed(archive.bytes.data(), archive.bytes.size()));

    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;
    EXPECT_EQ(fs::path(consumer.rootPath()), target / "movie.bin");
    EXPECT_EQ(fs::file_size(target / "movie.bin"), 4096u);
    EXPECT_EQ(consumer.stats().directories, 0u);
}

TEST_F(StreamConsumerTest, ExistingRootNameGetsSuffix)
{
    writeFile(root / "src" / "a.txt", "new");
    writeFile(root / "dst" / "src" / "keep.txt", "old");

    const Archive archive = produce(root / "src", "src");
    ASSERT_TRUE(archive.ok);

    StreamConsumer consumer({(root / "dst").u8string()});
    ASSERT_TRUE(consumer.feed(archive.bytes.data(), archive.bytes.size()));
    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;

    EXPECT_EQ(consumer.rootPath(), (root / "dst" / "src(1)").u8string());
    EXPECT_EQ(fs::file_size(root / "dst" / "src" / "keep.txt"), 3u);
    EXPECT_EQ(fs::file_size(root / "dst" / "src(1)" / "a.txt"), 3u);
}

TEST_F(StreamConsumerTest, TruncatedStreamFailsWithoutPartFiles)
{
    writeFile(root / "src" / "a.txt", std::string(10000, 'a'));
    const Archive archive = produce(root / "src", "src");
    ASSERT_TRUE(archive.ok);

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    const std::string truncated = archive.bytes.substr(0, archive.bytes.size() - 4);
    // The parser cannot know the stream is truncated until EOF: feed() simply
    // consumes what it got, finish() is the authoritative check.
    consumer.feed(truncated.data(), truncated.size());

    std::string error;
    EXPECT_FALSE(consumer.finish(&error));
    EXPECT_EQ(consumer.status(), StreamConsumer::Status::Error);
    EXPECT_EQ(countParts(target), size_t(0));
    for (const auto &entry : readTree(target))
        EXPECT_EQ(entry.first.find(".part"), std::string::npos);
}

TEST_F(StreamConsumerTest, CorruptedFileCrcIsRejected)
{
    writeFile(root / "src" / "a.txt", std::string(64, 'a'));
    const Archive archive = produce(root / "src", "src", Codec::None);
    ASSERT_TRUE(archive.ok);

    // Preamble(8) + fixed header(36) + path + data. Flip one data byte.
    const size_t dataOffset = kStreamPreambleSize + kFrameHeaderFixedSize + std::string("src").size();
    ASSERT_GT(archive.bytes.size(), dataOffset + 8);
    std::string corrupted = archive.bytes;
    corrupted[dataOffset + 5] ^= 0x40;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    EXPECT_FALSE(consumer.feed(corrupted.data(), corrupted.size()));

    std::string error;
    EXPECT_FALSE(consumer.finish(&error));
    EXPECT_FALSE(fs::exists(target / "src" / "a.txt"));
    EXPECT_EQ(countParts(target), size_t(0));
}

TEST_F(StreamConsumerTest, ProgressCallbackCanCancel)
{
    writeFile(root / "src" / "big.bin", std::string(200000, 'x'));
    const Archive archive = produce(root / "src", "src");
    ASSERT_TRUE(archive.ok);

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    uint64_t seen = 0;
    consumer.setProgressCallback([&seen](uint64_t bytes) {
        seen += bytes;
        return seen < 1000;
    });

    EXPECT_FALSE(consumer.feed(archive.bytes.data(), archive.bytes.size()));
    EXPECT_EQ(consumer.status(), StreamConsumer::Status::Canceled);
    consumer.abort();
    EXPECT_EQ(countParts(target), size_t(0));
    EXPECT_FALSE(fs::exists(target / "src" / "big.bin"));
}

TEST_F(StreamConsumerTest, PathTraversalIsRejected)
{
    // Build a hostile archive by hand: the parser must reject the name before
    // the consumer ever maps it to disk.
    FrameWriter::Options options;
    options.validatePaths = false;
    FrameWriter writer(options);
    std::string frames;
    ASSERT_TRUE(writer.writeRootDir(&frames, "src"));
    ASSERT_TRUE(writer.beginFile(&frames, "../evil.txt", 4));
    ASSERT_TRUE(writer.appendFileData(&frames, "evil", 4));
    ASSERT_TRUE(writer.endFile(&frames));
    ASSERT_TRUE(writer.writeEnd(&frames));

    StreamPreamble preamble;
    preamble.codec = Codec::None;
    std::string archive;
    encodePreamble(preamble, &archive);
    archive += frames;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    EXPECT_FALSE(consumer.feed(archive.data(), archive.size()));

    std::string error;
    EXPECT_FALSE(consumer.finish(&error));
    EXPECT_FALSE(fs::exists(root / "evil.txt"));
    EXPECT_FALSE(fs::exists(target / "src" / "evil.txt"));
}

TEST_F(StreamConsumerTest, DuplicateFileNamesWithinArchiveDoNotOverwrite)
{
    FrameWriter writer;
    std::string frames;
    ASSERT_TRUE(writer.writeRootDir(&frames, "src"));
    for (const std::string &content : {std::string("first"), std::string("second")}) {
        ASSERT_TRUE(writer.beginFile(&frames, "a.txt", content.size()));
        ASSERT_TRUE(writer.appendFileData(&frames, content));
        ASSERT_TRUE(writer.endFile(&frames));
    }
    ASSERT_TRUE(writer.writeEnd(&frames));

    StreamPreamble preamble;
    preamble.codec = Codec::None;
    std::string archive;
    encodePreamble(preamble, &archive);
    archive += frames;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    ASSERT_TRUE(consumer.feed(archive.data(), archive.size()));
    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;

    const auto files = readTree(consumer.rootPath());
    ASSERT_EQ(files.count("a.txt"), size_t(1));
    ASSERT_EQ(files.count("a(1).txt"), size_t(1));
    EXPECT_EQ(files.at("a.txt"), "first");
    EXPECT_EQ(files.at("a(1).txt"), "second");
}

TEST_F(StreamConsumerTest, ResumeContinuesAfterCompletedFrames)
{
    writeFile(root / "src" / "a.txt", "alpha");
    writeFile(root / "src" / "big.bin", std::string(500000, 'x'));
    const Archive full = produce(root / "src", "src");
    ASSERT_TRUE(full.ok) << full.error;

    const fs::path target = root / "dst";

    // First attempt: stop in the middle of big.bin. a.txt is complete, the
    // partial big.bin must be removed.
    uint64_t seen = 0;
    {
        StreamConsumer consumer({target.u8string()});
        consumer.setProgressCallback([&seen](uint64_t bytes) {
            seen += bytes;
            return seen < 1000;  // a.txt is 5 bytes, so this is inside big.bin
        });
        EXPECT_FALSE(consumer.feed(full.bytes.data(), full.bytes.size()));
        EXPECT_EQ(consumer.status(), StreamConsumer::Status::Canceled);
        EXPECT_EQ(consumer.completedEntries(), 2u);  // RootDir + a.txt
        EXPECT_TRUE(fs::exists(consumer.rootPath() + "/a.txt"));
        EXPECT_FALSE(fs::exists(consumer.rootPath() + "/big.bin"));
    }

    // Second attempt: sender enumerates deterministically and skips the two
    // completed frames; receiver continues inside the existing root.
    StreamProducerOptions options;
    options.skipEntries = 2;
    StreamProducer producer(options);
    std::string resumed;
    StreamProducerStats stats;
    std::string error;
    ASSERT_TRUE(producer.run(
            (root / "src").u8string(), "src",
            [&resumed](const char *data, size_t size) {
                resumed.append(data, size);
                return true;
            },
            [] { return false; },
            [](const std::string &, const std::string &) {},
            &stats, &error)) << error;

    StreamConsumerOptions consumerOptions;
    consumerOptions.targetDir = target.u8string();
    consumerOptions.resumeRoot = (target / "src").u8string();
    StreamConsumer consumer(consumerOptions);
    ASSERT_TRUE(consumer.feed(resumed.data(), resumed.size()));
    ASSERT_TRUE(consumer.finish(&error)) << error;
    EXPECT_EQ(consumer.completedEntries(), 1u);  // only big.bin

    EXPECT_TRUE(fs::exists(target / "src" / "a.txt"));
    EXPECT_EQ(fs::file_size(target / "src" / "big.bin"), 500000u);
    EXPECT_EQ(countParts(target), size_t(0));
    // No duplicate root and no duplicate a.txt.
    EXPECT_FALSE(fs::exists(target / "src(1)"));
    EXPECT_FALSE(fs::exists(target / "src" / "a(1).txt"));
}

TEST_F(StreamConsumerTest, ResumedStreamRejectsRootFrame)
{
    writeFile(root / "src" / "a.txt", "a");
    const Archive archive = produce(root / "src", "src");
    ASSERT_TRUE(archive.ok);

    StreamConsumerOptions options;
    options.targetDir = (root / "dst").u8string();
    options.resumeRoot = (root / "dst" / "src").u8string();
    fs::create_directories(root / "dst" / "src");
    StreamConsumer consumer(options);

    EXPECT_FALSE(consumer.feed(archive.bytes.data(), archive.bytes.size()));
    EXPECT_EQ(consumer.status(), StreamConsumer::Status::Error);
    EXPECT_NE(consumer.lastError().find("root frame"), std::string::npos);
}

TEST_F(StreamConsumerTest, ErrorFramesDoNotAbortTheArchive)
{
    FrameWriter writer;
    std::string frames;
    ASSERT_TRUE(writer.writeRootDir(&frames, "src"));
    ASSERT_TRUE(writer.writeError(&frames, "locked.txt", "cannot open source file"));
    const std::string content = "ok";
    ASSERT_TRUE(writer.beginFile(&frames, "ok.txt", content.size()));
    ASSERT_TRUE(writer.appendFileData(&frames, content));
    ASSERT_TRUE(writer.endFile(&frames));
    ASSERT_TRUE(writer.writeEnd(&frames));

    StreamPreamble preamble;
    preamble.codec = Codec::None;
    std::string archive;
    encodePreamble(preamble, &archive);
    archive += frames;

    const fs::path target = root / "dst";
    StreamConsumer consumer({target.u8string()});
    std::vector<std::string> reported;
    consumer.setErrorCallback([&reported](const std::string &path, const std::string &message) {
        reported.push_back(path + "|" + message);
    });

    ASSERT_TRUE(consumer.feed(archive.data(), archive.size()));
    std::string error;
    ASSERT_TRUE(consumer.finish(&error)) << error;
    ASSERT_EQ(reported.size(), size_t(1));
    EXPECT_NE(reported[0].find("locked.txt"), std::string::npos);
    EXPECT_EQ(readTree(consumer.rootPath()).at("ok.txt"), "ok");
}
