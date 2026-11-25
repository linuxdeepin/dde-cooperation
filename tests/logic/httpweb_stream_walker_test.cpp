// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "streamwalker.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace dtp;
namespace fs = std::filesystem;

namespace {

class StreamWalkerTest : public ::testing::Test
{
protected:
    fs::path root;

    void SetUp() override
    {
        static int counter = 0;
        root = fs::temp_directory_path()
                / ("dtp_walker_" + std::to_string(::getpid()) + "_" + std::to_string(++counter));
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
        out << content;
    }

    struct Result {
        std::vector<WalkEntry> entries;
        std::vector<std::string> errors;
        WalkStats stats;
    };

    Result run(const fs::path &rootPath, const std::string &displayName, StreamWalker::Options options = {})
    {
        Result result;
        StreamWalker walker(options);
        const bool ok = walker.walk(rootPath.u8string(), displayName,
                                    [&result](const WalkEntry &entry) {
                                        result.entries.push_back(entry);
                                        return true;
                                    },
                                    [&result](const std::string &path, const std::string &message) {
                                        result.errors.push_back(path + "|" + message);
                                    },
                                    &result.stats);
        EXPECT_TRUE(ok);
        return result;
    }
};

std::vector<std::string> pathsOf(const std::vector<WalkEntry> &entries)
{
    std::vector<std::string> paths;
    paths.reserve(entries.size());
    for (const auto &entry : entries)
        paths.push_back(entry.path);
    return paths;
}

} // namespace

TEST_F(StreamWalkerTest, EmitsRootThenSortedEntries)
{
    writeFile(root / "src" / "b.txt", "bb");
    writeFile(root / "src" / "a.txt", "a");
    fs::create_directories(root / "src" / "zdir");
    fs::create_directories(root / "src" / "adir" / "nested");

    const Result result = run(root / "src", "src");

    const std::vector<std::string> expected = {
            "src",          // RootDir
            "a.txt",        // files and dirs in sorted name order
            "adir",
            "b.txt",
            "zdir",
            "adir/nested",  // depth first, sorted
    };
    EXPECT_EQ(pathsOf(result.entries), expected);
    EXPECT_EQ(result.entries.front().type, FrameType::RootDir);
    EXPECT_EQ(result.stats.directories, 4u);  // root + adir + adir/nested + zdir
    EXPECT_EQ(result.stats.files, 2u);
    EXPECT_EQ(result.stats.bytes, 3u);
}

TEST_F(StreamWalkerTest, EmptyDirectoriesAreEmitted)
{
    fs::create_directories(root / "tree" / "empty");

    const Result result = run(root / "tree", "tree");
    const std::vector<std::string> expected = {"tree", "empty"};
    EXPECT_EQ(pathsOf(result.entries), expected);
    EXPECT_EQ(result.stats.directories, 2u);
    EXPECT_EQ(result.stats.files, 0u);
}

TEST_F(StreamWalkerTest, RootFileIsASingleFrame)
{
    writeFile(root / "movie.mkv", std::string(1024, 'x'));

    const Result result = run(root / "movie.mkv", "movie.mkv");
    ASSERT_EQ(result.entries.size(), size_t(1));
    EXPECT_EQ(result.entries[0].type, FrameType::RootFile);
    EXPECT_EQ(result.entries[0].path, "movie.mkv");
    EXPECT_EQ(result.entries[0].size, 1024u);
    EXPECT_EQ(result.stats.files, 1u);
    EXPECT_EQ(result.stats.bytes, 1024u);
}

TEST_F(StreamWalkerTest, MissingRootFails)
{
    StreamWalker walker;
    bool called = false;
    const bool ok = walker.walk((root / "nope").u8string(), "nope",
                                [&called](const WalkEntry &) {
                                    called = true;
                                    return true;
                                },
                                nullptr, nullptr);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(called);
}

TEST_F(StreamWalkerTest, UnsafeDisplayNameIsRejected)
{
    writeFile(root / "src" / "a.txt", "a");
    StreamWalker walker;
    EXPECT_FALSE(walker.walk((root / "src").u8string(), "../escape",
                             [](const WalkEntry &) { return true; }, nullptr, nullptr));
    EXPECT_FALSE(walker.walk((root / "src").u8string(), "a/b",
                             [](const WalkEntry &) { return true; }, nullptr, nullptr));
}

TEST_F(StreamWalkerTest, UnsafeChildNameIsSkippedNotFatal)
{
    writeFile(root / "src" / "good.txt", "ok");
    writeFile(root / "src" / "bad\\name.txt", "bad");

    const Result result = run(root / "src", "src");

    EXPECT_EQ(result.stats.files, 1u);
    EXPECT_EQ(result.stats.skippedUnsafe, 1u);
    EXPECT_EQ(result.stats.errors, 1u);
    ASSERT_EQ(result.entries.size(), size_t(2));  // root + good.txt
    EXPECT_EQ(result.entries[1].path, "good.txt");
}

TEST_F(StreamWalkerTest, UnicodeNamesArePreserved)
{
    writeFile(root / "src" / u8"文档" / u8"é.txt", "unicode");

    const Result result = run(root / "src", "src");
    const std::vector<std::string> expected = {"src", u8"文档", u8"文档/é.txt"};
    EXPECT_EQ(pathsOf(result.entries), expected);
}

TEST_F(StreamWalkerTest, DepthLimitStopsDescent)
{
    fs::path deep = root / "deep";
    for (int i = 0; i < 20; ++i)
        deep /= "d";
    fs::create_directories(deep);

    StreamWalker::Options options;
    options.maxDepth = 5;
    const Result result = run(root / "deep", "deep", options);

    EXPECT_GT(result.stats.skippedDeep, 0u);
    // The walk still terminates and reports the problem instead of aborting.
    EXPECT_GT(result.errors.size(), 0u);
}

#ifndef _WIN32
TEST_F(StreamWalkerTest, SymlinkLoopTerminates)
{
    fs::create_directories(root / "src" / "a");
    writeFile(root / "src" / "a" / "f.txt", "data");
    // a/loop -> .. (back to src): following it would revisit src forever.
    std::error_code ec;
    fs::create_directory_symlink(root / "src", root / "src" / "a" / "loop", ec);
    ASSERT_FALSE(ec) << ec.message();

    const Result result = run(root / "src", "src");

    EXPECT_EQ(result.stats.cycles, 1u);
    EXPECT_EQ(result.stats.files, 1u);
    // Every path in the result must be unique: no repeated descent.
    const auto paths = pathsOf(result.entries);
    EXPECT_EQ(std::adjacent_find(paths.begin(), paths.end()), paths.end());
}

TEST_F(StreamWalkerTest, SymlinkInsideRootIsFollowed)
{
    writeFile(root / "src" / "real.txt", "data");
    std::error_code ec;
    fs::create_symlink(root / "src" / "real.txt", root / "src" / "link.txt", ec);
    ASSERT_FALSE(ec) << ec.message();

    const Result result = run(root / "src", "src");

    EXPECT_EQ(result.stats.skippedOutside, 0u);
    EXPECT_EQ(result.stats.files, 2u);
    const auto paths = pathsOf(result.entries);
    EXPECT_NE(std::find(paths.begin(), paths.end(), "link.txt"), paths.end());
}

TEST_F(StreamWalkerTest, SymlinkToSiblingDirectoryKeepsBothPaths)
{
    writeFile(root / "src" / "real" / "data.txt", "payload");
    std::error_code ec;
    fs::create_directory_symlink(root / "src" / "real", root / "src" / "alias", ec);
    ASSERT_FALSE(ec) << ec.message();

    const Result result = run(root / "src", "src");

    EXPECT_EQ(result.stats.cycles, 0u);
    EXPECT_EQ(result.stats.files, 2u);
    const auto paths = pathsOf(result.entries);
    EXPECT_NE(std::find(paths.begin(), paths.end(), "real/data.txt"), paths.end());
    EXPECT_NE(std::find(paths.begin(), paths.end(), "alias/data.txt"), paths.end());
}

TEST_F(StreamWalkerTest, SymlinkOutsideRootIsSkipped)
{
    writeFile(root / "outside.txt", "secret");
    writeFile(root / "src" / "inside.txt", "ok");
    std::error_code ec;
    fs::create_symlink(root / "outside.txt", root / "src" / "link.txt", ec);
    ASSERT_FALSE(ec) << ec.message();

    const Result result = run(root / "src", "src");

    EXPECT_EQ(result.stats.skippedOutside, 1u);
    EXPECT_EQ(result.stats.files, 1u);
    const auto paths = pathsOf(result.entries);
    EXPECT_EQ(std::find(paths.begin(), paths.end(), "link.txt"), paths.end());
    ASSERT_EQ(result.errors.size(), 1u);
}

TEST_F(StreamWalkerTest, PreserveSymlinksEmitsLinksWithoutFollowing)
{
    writeFile(root / "src" / "real" / "data.txt", "payload");
    std::error_code ec;
    fs::create_symlink(root / "src" / "real" / "data.txt", root / "src" / "file-link", ec);
    ASSERT_FALSE(ec) << ec.message();
    fs::create_directory_symlink(root / "src" / "real", root / "src" / "dir-link", ec);
    ASSERT_FALSE(ec) << ec.message();

    StreamWalker::Options options;
    options.preserveSymlinks = true;
    const Result result = run(root / "src", "src", options);

    EXPECT_EQ(result.stats.symlinks, 2u);
    EXPECT_EQ(result.stats.files, 1u);
    EXPECT_EQ(result.stats.directories, 2u);  // root + real
    EXPECT_EQ(result.stats.cycles, 0u);

    const WalkEntry *fileLink = nullptr;
    const WalkEntry *dirLink = nullptr;
    for (const auto &entry : result.entries) {
        if (entry.path == "file-link")
            fileLink = &entry;
        if (entry.path == "dir-link")
            dirLink = &entry;
    }
    ASSERT_NE(fileLink, nullptr);
    ASSERT_NE(dirLink, nullptr);
    EXPECT_EQ(fileLink->type, FrameType::Symlink);
    EXPECT_EQ(fileLink->linkTarget, "real/data.txt");
    EXPECT_FALSE(fileLink->linkToDirectory);
    EXPECT_EQ(dirLink->type, FrameType::Symlink);
    EXPECT_EQ(dirLink->linkTarget, "real");
    EXPECT_TRUE(dirLink->linkToDirectory);

    const auto paths = pathsOf(result.entries);
    EXPECT_EQ(std::find(paths.begin(), paths.end(), "dir-link/data.txt"), paths.end());
}

TEST_F(StreamWalkerTest, PreserveOutsideSymlinkIsSkipped)
{
    writeFile(root / "outside.txt", "secret");
    writeFile(root / "src" / "inside.txt", "ok");
    std::error_code ec;
    fs::create_symlink(root / "outside.txt", root / "src" / "link.txt", ec);
    ASSERT_FALSE(ec) << ec.message();

    StreamWalker::Options options;
    options.preserveSymlinks = true;
    const Result result = run(root / "src", "src", options);

    EXPECT_EQ(result.stats.skippedOutside, 1u);
    EXPECT_EQ(result.stats.symlinks, 0u);
    EXPECT_EQ(result.stats.files, 1u);
    const auto paths = pathsOf(result.entries);
    EXPECT_EQ(std::find(paths.begin(), paths.end(), "link.txt"), paths.end());
    ASSERT_EQ(result.errors.size(), 1u);
}

TEST_F(StreamWalkerTest, PreserveDanglingInsideSymlink)
{
    writeFile(root / "src" / "real.txt", "x");
    std::error_code ec;
    fs::create_symlink("missing.txt", root / "src" / "dangling", ec);
    ASSERT_FALSE(ec) << ec.message();

    StreamWalker::Options options;
    options.preserveSymlinks = true;
    const Result result = run(root / "src", "src", options);

    EXPECT_EQ(result.stats.symlinks, 1u);
    ASSERT_EQ(result.entries.size(), size_t(3));  // root + dangling + real.txt
    const WalkEntry &link = result.entries[1];
    EXPECT_EQ(link.type, FrameType::Symlink);
    EXPECT_EQ(link.path, "dangling");
    EXPECT_EQ(link.linkTarget, "missing.txt");
}

TEST_F(StreamWalkerTest, SpecialFilesAreSkipped)
{
    fs::create_directories(root / "src");
    writeFile(root / "src" / "normal.txt", "x");
    const fs::path fifo = root / "src" / "pipe";
    ASSERT_EQ(::mkfifo(fifo.c_str(), 0600), 0);

    const Result result = run(root / "src", "src");

    EXPECT_EQ(result.stats.skippedSpecial, 1u);
    EXPECT_EQ(result.stats.files, 1u);
    ASSERT_EQ(result.entries.size(), size_t(2));
    EXPECT_EQ(result.entries[1].path, "normal.txt");
}
#endif

TEST_F(StreamWalkerTest, OversizedChildIsSkipped)
{
    writeFile(root / "src" / "small.txt", "x");
    writeFile(root / "src" / "big.bin", std::string(4096, 'b'));

    StreamWalker::Options options;
    options.maxFileSize = 1024;
    const Result result = run(root / "src", "src", options);

    EXPECT_EQ(result.stats.files, 1u);
    EXPECT_EQ(result.entries.size(), size_t(2));
    EXPECT_EQ(result.entries[1].path, "small.txt");
    EXPECT_GT(result.errors.size(), 0u);
}

TEST_F(StreamWalkerTest, CallbackCanAbort)
{
    writeFile(root / "src" / "a.txt", "a");
    writeFile(root / "src" / "b.txt", "b");

    StreamWalker walker;
    int seen = 0;
    const bool ok = walker.walk((root / "src").u8string(), "src",
                                [&seen](const WalkEntry &) {
                                    ++seen;
                                    return false;  // stop immediately
                                },
                                nullptr, nullptr);
    EXPECT_FALSE(ok);
    EXPECT_EQ(seen, 1);
}

TEST_F(StreamWalkerTest, OrderIsStableAcrossRuns)
{
    for (int i = 0; i < 20; ++i)
        writeFile(root / "src" / ("f" + std::to_string(i) + ".txt"), "x");
    for (int i = 0; i < 5; ++i)
        fs::create_directories(root / "src" / ("d" + std::to_string(i)));

    const Result first = run(root / "src", "src");
    const Result second = run(root / "src", "src");
    EXPECT_EQ(pathsOf(first.entries), pathsOf(second.entries));
}
