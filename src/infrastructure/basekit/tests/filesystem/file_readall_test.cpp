// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "filesystem/file.h"
#include "filesystem/exceptions.h"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

using namespace BaseKit;

namespace {
std::string FreshPath()
{
    char t[] = "/tmp/bk_fra_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    close(fd);
    std::remove(t);
    return t;
}
}

TEST(FileReadAllTest, MemberReadAllTextAfterOpen)
{
    std::string p = FreshPath();
    File::WriteAllText(Path(p), "the text body");
    File f(p);
    f.Open(true, false);
    std::string text = f.ReadAllText();
    EXPECT_EQ(text, "the text body");
    f.Close();
    std::remove(p.c_str());
}

TEST(FileReadAllTest, MemberReadAllBytesAfterOpen)
{
    std::string p = FreshPath();
    File::WriteAllBytes(Path(p), "abcd", 4);
    File f(p);
    f.Open(true, false);
    auto bytes = f.ReadAllBytes();
    EXPECT_EQ(bytes.size(), 4u);
    EXPECT_EQ(bytes[0], 'a');
    f.Close();
    std::remove(p.c_str());
}

TEST(FileReadAllTest, MemberReadAllLinesAfterOpen)
{
    std::string p = FreshPath();
    std::vector<std::string> lines = {"alpha", "beta", "gamma"};
    File::WriteAllLines(Path(p), lines);
    File f(p);
    f.Open(true, false);
    auto read = f.ReadAllLines();
    EXPECT_GE(read.size(), 1u);
    EXPECT_EQ(read.front(), "alpha");
    f.Close();
    std::remove(p.c_str());
}

TEST(FileReadAllTest, MemberWriteStringOverload)
{
    std::string p = FreshPath();
    File f(p);
    f.Create(false, true);
    EXPECT_EQ(f.Write(std::string("written-text")), 12u);
    f.Close();
    EXPECT_EQ(File::ReadAllText(Path(p)), "written-text");
    std::remove(p.c_str());
}

TEST(FileReadAllTest, MemberWriteLinesOverload)
{
    std::string p = FreshPath();
    File f(p);
    f.Create(false, true);
    std::vector<std::string> lines = {"x", "y", "z"};
    size_t n = f.Write(lines);
    EXPECT_GT(n, 0u);
    f.Close();
    auto read = File::ReadAllLines(Path(p));
    EXPECT_FALSE(read.empty());
    EXPECT_EQ(read.front(), "x");
    std::remove(p.c_str());
}

TEST(FileReadAllTest, MemberReadAllTextEmptyFile)
{
    std::string p = FreshPath();
    File::WriteEmpty(Path(p));
    File f(p);
    f.Open(true, false);
    std::string text = f.ReadAllText();
    EXPECT_TRUE(text.empty());
    f.Close();
    std::remove(p.c_str());
}

TEST(FileReadAllTest, MemberReadAllBytesReadsFromCurrentOffset)
{
    std::string p = FreshPath();
    File::WriteAllText(Path(p), "0123456789");
    File f(p);
    f.Open(true, false);
    f.Seek(5);
    auto bytes = f.ReadAllBytes();
    EXPECT_EQ(bytes.size(), 5u);
    EXPECT_EQ(bytes[0], '5');
    f.Close();
    std::remove(p.c_str());
}
