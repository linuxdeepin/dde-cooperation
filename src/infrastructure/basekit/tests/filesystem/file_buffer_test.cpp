// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

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
std::string FreshPath()
{
    char t[] = "/tmp/bk_fbuf_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    close(fd);
    std::remove(t);
    return t;
}

class TempFile
{
public:
    explicit TempFile(const std::string& content = "")
    {
        _p = FreshPath();
        if (!content.empty()) File::WriteAllText(_p, content);
    }
    ~TempFile() { std::remove(_p.c_str()); }
    const std::string& str() const { return _p; }
private:
    std::string _p;
};
}

TEST(FileBufferTest, BufferedWriteFlushesOnClose)
{
    std::string p = FreshPath();
    File f(p);
    f.OpenOrCreate(false, true, false, File::DEFAULT_ATTRIBUTES, File::DEFAULT_PERMISSIONS, 4);

    std::string payload(100, 'X');
    EXPECT_EQ(f.Write(payload.data(), payload.size()), payload.size());
    f.Close();
    EXPECT_EQ(File::ReadAllText(Path(p)).size(), payload.size());
    std::remove(p.c_str());
}

TEST(FileBufferTest, BufferedReadRoundTrip)
{
    std::string p = FreshPath();
    std::string payload = "0123456789ABCDEFGHIJ";
    File::WriteAllText(Path(p), payload);

    File f(p);
    f.Open(true, false, false, File::DEFAULT_ATTRIBUTES, File::DEFAULT_PERMISSIONS, 4);
    char buf[32] = {0};
    EXPECT_EQ(f.Read(buf, payload.size()), payload.size());
    EXPECT_EQ(std::string(buf, payload.size()), payload);
    f.Close();
    std::remove(p.c_str());
}

TEST(FileBufferTest, BufferedWriteAcrossMultipleBuffers)
{
    std::string p = FreshPath();
    File f(p);
    f.Create(false, true, File::DEFAULT_ATTRIBUTES, File::DEFAULT_PERMISSIONS, 8);

    std::vector<uint8_t> data(64);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = (uint8_t)(i % 256);
    EXPECT_EQ(f.Write(data.data(), data.size()), data.size());
    f.Flush();
    EXPECT_EQ(f.size(), data.size());
    f.Close();

    auto readBack = File::ReadAllBytes(Path(p));
    EXPECT_EQ(readBack, data);
    std::remove(p.c_str());
}

TEST(FileBufferTest, ZeroBufferWriteReadDirectly)
{
    std::string p = FreshPath();
    File f(p);
    f.Create(true, true, File::DEFAULT_ATTRIBUTES, File::DEFAULT_PERMISSIONS, 0);

    const char data[] = "direct";
    EXPECT_EQ(f.Write(data, 6), 6u);
    f.Seek(0);
    char buf[8] = {0};
    EXPECT_EQ(f.Read(buf, 6), 6u);
    EXPECT_EQ(std::string(buf, 6), "direct");
    f.Close();
    std::remove(p.c_str());
}

TEST(FileBufferTest, ResizeClosedFileUsesTruncate)
{
    TempFile tf("seed");
    File f(tf.str());
    EXPECT_NO_THROW(f.Resize(500));
    EXPECT_EQ(File(Path(tf.str())).size(), 500u);
    EXPECT_NO_THROW(f.Resize(10));
    EXPECT_EQ(File(Path(tf.str())).size(), 10u);
}

TEST(FileBufferTest, OpenOrCreateTruncateExisting)
{
    TempFile tf("original-content");
    File f(tf.str());
    f.OpenOrCreate(true, true, true);
    EXPECT_TRUE(f.IsFileOpened());
    EXPECT_EQ(f.size(), 0u);
    f.Close();
    EXPECT_EQ(File(Path(tf.str())).size(), 0u);
}

TEST(FileBufferTest, OpenTruncateExisting)
{
    TempFile tf("abc");
    File f(tf.str());
    f.Open(true, true, true);
    EXPECT_EQ(f.size(), 0u);
    f.Close();
}

TEST(FileBufferTest, SeekPastEndAndOffsetReport)
{
    TempFile tf("hello");
    File f(tf.str());
    f.Open(true, true);
    f.Seek(100);
    EXPECT_EQ(f.offset(), 100u);
    f.Close();
}

TEST(FileBufferTest, FlushAfterWritePersists)
{
    std::string p = FreshPath();
    File f(p);
    f.Create(true, true);
    const char data[] = "abcdef";
    f.Write(data, 6);
    EXPECT_NO_THROW(f.Flush());
    f.Close();
    EXPECT_EQ(File::ReadAllText(Path(p)), "abcdef");
    std::remove(p.c_str());
}

TEST(FileBufferTest, ReadWriteEmptyBufferNoop)
{
    TempFile tf("x");
    File f(tf.str());
    f.Open(true, true);
    EXPECT_EQ(f.Write(nullptr, 0), 0u);
    EXPECT_EQ(f.Read(nullptr, 0), 0u);
    f.Close();
}

TEST(FileBufferTest, AssignmentAndSwapSemantics)
{
    std::string pa = FreshPath();
    std::string pb = FreshPath();
    File a(pa);
    a.Create(true, true);
    File b(pb);
    b.Create(true, true);

    File copyAssigned;
    copyAssigned = a;
    EXPECT_EQ(copyAssigned.string(), pa);
    EXPECT_FALSE(copyAssigned.IsFileOpened());

    File moveAssigned;
    moveAssigned = std::move(b);
    EXPECT_TRUE(moveAssigned.IsFileOpened());
    EXPECT_EQ(moveAssigned.string(), pb);

    a.Close();
    moveAssigned.Close();
    std::remove(pa.c_str());
    std::remove(pb.c_str());
}
