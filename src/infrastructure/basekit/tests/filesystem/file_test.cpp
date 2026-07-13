// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// File 生命周期与读写覆盖：Create/Open/OpenOrCreate/Close、状态查询、
// Seek/Resize/Flush、Read/Write 往返、不存在文件抛异常、静态读写助手。

#include <gtest/gtest.h>
#include "filesystem/file.h"
#include "filesystem/exceptions.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

using namespace BaseKit;

namespace {
// RAII 临时文件助手（来自 path_advanced_test.cpp）
class TempFile
{
public:
    explicit TempFile(const std::string &content = "")
    {
        char t[] = "/tmp/bk_file_XXXXXX";
        int fd = mkstemp(t);
        if (fd < 0) throw std::runtime_error("mkstemp");
        _p = t;
        close(fd);
        if (!content.empty()) File::WriteAllText(_p, content);
    }
    ~TempFile() { std::remove(_p.c_str()); }
    const std::string &str() const { return _p; }
private:
    std::string _p;
};

// 生成一个尚不存在的临时路径（用于 Create 测试，Create 要求文件不存在）
std::string freshPath()
{
    char t[] = "/tmp/bk_file_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    close(fd);
    std::remove(t);
    return t;
}
}   // namespace

// ===== Create / Close 生命周期 =====
TEST(FileTest, CreateOpensForWriteAndReportsState)
{
    std::string p = freshPath();
    File f(p);
    EXPECT_FALSE(f.IsFileOpened());
    f.Create(false, true);   // 只写
    EXPECT_TRUE(f.IsFileOpened());
    EXPECT_TRUE(f.IsFileWriteOpened());
    EXPECT_FALSE(f.IsFileReadOpened());
    f.Close();
    EXPECT_FALSE(f.IsFileOpened());
    Path::Remove(Path(p));
}

TEST(FileTest, CreateReadWrite)
{
    std::string p = freshPath();
    File f(p);
    f.Create(true, true);
    EXPECT_TRUE(f.IsFileReadOpened());
    EXPECT_TRUE(f.IsFileWriteOpened());
    EXPECT_TRUE(f.IsFileExists());
    EXPECT_TRUE(f.IsFileEmpty());
    f.Close();
    Path::Remove(Path(p));
}

TEST(FileTest, CreateExistingFileThrows)
{
    TempFile tf;   // 已存在
    File f(tf.str());
    EXPECT_THROW(f.Create(false, true), FileSystemException);
}

// ===== Open =====
TEST(FileTest, OpenExistingReadWrite)
{
    TempFile tf("hello");
    File f(tf.str());
    f.Open(true, true);
    EXPECT_TRUE(f.IsFileOpened());
    EXPECT_TRUE(f.IsFileReadOpened());
    EXPECT_TRUE(f.IsFileWriteOpened());
    EXPECT_FALSE(f.IsFileEmpty());
    f.Close();
}

TEST(FileTest, OpenNonexistentThrows)
{
    File f("/tmp/__bk_definitely_not_existing_file__");
    EXPECT_FALSE(f.IsFileExists());
    EXPECT_THROW(f.Open(true, false), FileSystemException);
}

TEST(FileTest, OpenOrCreateCreatesIfMissing)
{
    std::string p = freshPath();
    File f(p);
    EXPECT_NO_THROW(f.OpenOrCreate(true, true));
    EXPECT_TRUE(f.IsFileOpened());
    f.Close();
    EXPECT_TRUE(File(p).IsFileExists());
    Path::Remove(Path(p));
}

// ===== Seek / offset / Resize / Flush =====
TEST(FileTest, SeekUpdatesOffset)
{
    TempFile tf("0123456789");
    File f(tf.str());
    f.Open(true, true);
    f.Seek(4u);
    EXPECT_EQ(f.offset(), 4u);
    char buf[4] = {0};
    EXPECT_EQ(f.Read(buf, 3), 3u);
    EXPECT_EQ(std::string(buf, 3), "456");
    f.Close();
}

TEST(FileTest, ResizeGrowAndShrink)
{
    std::string p = freshPath();
    File f(p);
    f.Create(true, true);
    f.Resize(100);
    EXPECT_EQ(f.size(), 100u);
    f.Resize(10);
    EXPECT_EQ(f.size(), 10u);
    f.Close();
    Path::Remove(Path(p));
}

TEST(FileTest, FlushOnOpenedWriteDoesNotThrow)
{
    TempFile tf;
    File f(tf.str());
    f.Open(true, true, true);   // truncate
    const char data[] = "abc";
    EXPECT_EQ(f.Write(data, 3), 3u);
    EXPECT_NO_THROW(f.Flush());
    f.Close();
}

// ===== Read / Write 往返 =====
TEST(FileTest, WriteThenReadRoundTrip)
{
    std::string p = freshPath();
    File f(p);
    f.Create(true, true);
    const uint8_t out[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(f.Write(out, 5), 5u);
    f.Flush();
    f.Seek(0);
    uint8_t in[5] = {0};
    EXPECT_EQ(f.Read(in, 5), 5u);
    EXPECT_EQ(memcmp(in, out, 5), 0);
    f.Close();
    Path::Remove(Path(p));
}

TEST(FileTest, ReadWriteEmptyBufferIsNoop)
{
    TempFile tf("x");
    File f(tf.str());
    f.Open(true, true);
    EXPECT_EQ(f.Write(nullptr, 0), 0u);
    EXPECT_EQ(f.Read(nullptr, 0), 0u);
    f.Close();
}

// ===== 静态助手 =====
TEST(FileTest, StaticReadAllBytesAndText)
{
    TempFile tf("the quick brown fox");
    Path p(tf.str());
    EXPECT_EQ(File::ReadAllText(p), "the quick brown fox");
    auto bytes = File::ReadAllBytes(p);
    EXPECT_EQ(bytes.size(), 19u);
    EXPECT_EQ(bytes.front(), 't');
}

TEST(FileTest, StaticWriteAllTextAndWriteEmpty)
{
    std::string p = freshPath();
    Path pp(p);
    EXPECT_EQ(File::WriteAllText(pp, "abc"), 3u);
    EXPECT_EQ(File::ReadAllText(pp), "abc");
    File::WriteEmpty(pp);
    EXPECT_EQ(File(pp).size(), 0u);
    Path::Remove(pp);
}

TEST(FileTest, StaticWriteAllBytesRoundTrip)
{
    std::string p = freshPath();
    Path pp(p);
    std::vector<uint8_t> raw = {9, 8, 7, 6};
    EXPECT_EQ(File::WriteAllBytes(pp, raw.data(), raw.size()), raw.size());
    auto back = File::ReadAllBytes(pp);
    EXPECT_EQ(back, raw);
    Path::Remove(pp);
}

TEST(FileTest, StaticWriteAllLines)
{
    std::string p = freshPath();
    Path pp(p);
    std::vector<std::string> lines = {"one", "two", "three"};
    File::WriteAllLines(pp, lines);
    auto read = File::ReadAllLines(pp);
    EXPECT_GE(read.size(), 1u);
    EXPECT_EQ(read.front(), "one");
    Path::Remove(pp);
}

// ===== bool 转换与 size =====
TEST(FileTest, BoolConversionReflectsOpenedState)
{
    TempFile tf("data");
    File f(tf.str());
    EXPECT_FALSE(bool(f));
    f.Open(true, false);
    EXPECT_TRUE(bool(f));
    EXPECT_EQ(f.size(), 4u);
    f.Close();
    EXPECT_FALSE(bool(f));
}
