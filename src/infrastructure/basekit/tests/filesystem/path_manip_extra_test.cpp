// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "filesystem/path.h"
#include "filesystem/file.h"
#include "filesystem/directory.h"
#include "filesystem/exceptions.h"

#include <cstdio>
#include <string>
#include <unistd.h>

using namespace BaseKit;

namespace {
std::string FreshPath()
{
    char t[] = "/tmp/bk_pmx_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    close(fd);
    std::remove(t);
    return t;
}

void SafeCleanup(const Path& p) noexcept
{
    try { Path::RemoveAll(p); } catch (const std::exception&) {}
}
}

TEST(PathManipExtraTest, ValidateReplacesDeprecatedChars)
{
    Path p("a?b*c:d");
    Path v = p.validate();
    EXPECT_EQ(v.string(), "a_b_c_d");
    Path v2 = p.validate('-');
    EXPECT_EQ(v2.string(), "a-b-c-d");
}

TEST(PathManipExtraTest, ValidateKeepsSeparators)
{
    Path p("/a/b?c");
    Path v = p.validate();
    EXPECT_EQ(v.string(), "/a/b_c");
}

TEST(PathManipExtraTest, MakePreferredConvertsBackslashes)
{
    Path p("a\\b\\c/d");
    p.MakePreferred();
    EXPECT_EQ(p.string(), "a/b/c/d");
}

TEST(PathManipExtraTest, RemoveTrailingSeparatorsMultiple)
{
    Path p("a/b///");
    p.RemoveTrailingSeparators();
    EXPECT_EQ(p.string(), "a/b");
}

TEST(PathManipExtraTest, RemoveTrailingSeparatorsSingle)
{
    Path p("dir/");
    p.RemoveTrailingSeparators();
    EXPECT_EQ(p.string(), "dir");
}

TEST(PathManipExtraTest, RemoveTrailingSeparatorsEmpty)
{
    Path p("");
    p.RemoveTrailingSeparators();
    EXPECT_TRUE(p.empty());
}

TEST(PathManipExtraTest, DeprecatedCharDetection)
{
    EXPECT_TRUE(Path::deprecated('?'));
    EXPECT_TRUE(Path::deprecated('*'));
    EXPECT_TRUE(Path::deprecated(':'));
    EXPECT_TRUE(Path::deprecated('"'));
    EXPECT_TRUE(Path::deprecated('\\'));
    EXPECT_TRUE(Path::deprecated('/'));
    EXPECT_FALSE(Path::deprecated('a'));
    EXPECT_FALSE(Path::deprecated('0'));
    EXPECT_FALSE(Path::deprecated('_'));
}

TEST(PathManipExtraTest, DeprecatedWideCharDetection)
{
    EXPECT_TRUE(Path::deprecated(L'?'));
    EXPECT_TRUE(Path::deprecated(L'*'));
    EXPECT_FALSE(Path::deprecated(L'a'));
}

TEST(PathManipExtraTest, DeprecatedStringContainsAll)
{
    std::string d = Path::deprecated();
    EXPECT_NE(d.find('\\'), std::string::npos);
    EXPECT_NE(d.find('?'), std::string::npos);
    EXPECT_NE(d.find('*'), std::string::npos);
    EXPECT_NE(d.find(':'), std::string::npos);
    EXPECT_NE(d.find('"'), std::string::npos);
    EXPECT_NE(d.find('<'), std::string::npos);
    EXPECT_NE(d.find('>'), std::string::npos);
}

TEST(PathManipExtraTest, SeparatorIsSlashOnUnix)
{
    EXPECT_EQ(Path::separator(), '/');
}

TEST(PathManipExtraTest, AttributesEmptyOnUnix)
{
    std::string f = FreshPath();
    File::WriteAllText(Path(f), "x");
    Path p(f);
    Flags<FileAttributes> attr = p.attributes();
    EXPECT_FALSE((bool)attr);
    std::remove(f.c_str());
}

TEST(PathManipExtraTest, UniquePathNonEmpty)
{
    Path u = Path::unique();
    EXPECT_FALSE(u.empty());
    Path u2 = Path::unique();
    EXPECT_NE(u.string(), u2.string());
}

TEST(PathManipExtraTest, InitialPathNonEmpty)
{
    Path i = Path::initial();
    EXPECT_FALSE(i.empty());
}

TEST(PathManipExtraTest, HomePathNonEmpty)
{
    Path h = Path::home();
    EXPECT_FALSE(h.empty());
}

TEST(PathManipExtraTest, CopyIfFileEmptyPattern)
{
    std::string src = FreshPath();
    File::WriteAllText(Path(src), "copyme");
    std::string dst = FreshPath();
    Path result = Path::CopyIf(Path(src), Path(dst), "", true);
    EXPECT_EQ(result.string(), dst);
    EXPECT_EQ(File::ReadAllText(Path(dst)), "copyme");
    std::remove(src.c_str());
    std::remove(dst.c_str());
}

TEST(PathManipExtraTest, CopyIfFileMatchingPattern)
{
    std::string src = FreshPath() + ".txt";
    File::WriteAllText(Path(src), "data");
    std::string dst = FreshPath() + ".txt";
    Path result = Path::CopyIf(Path(src), Path(dst), ".*\\.txt", true);
    EXPECT_FALSE(result.empty());
    std::remove(src.c_str());
    std::remove(dst.c_str());
}

TEST(PathManipExtraTest, CopyIfFileNonMatchingPatternReturnsEmpty)
{
    std::string src = FreshPath() + ".log";
    File::WriteAllText(Path(src), "data");
    std::string dst = FreshPath() + ".log";
    Path result = Path::CopyIf(Path(src), Path(dst), ".*\\.txt", true);
    EXPECT_TRUE(result.empty());
    std::remove(src.c_str());
    std::remove(dst.c_str());
}

TEST(PathManipExtraTest, CopyAllDirectoryTree)
{
    Path base = Path::temp() / "bk_pmx_tree";
    SafeCleanup(base);
    Directory::CreateTree(base / "sub");
    File::WriteAllText(base / "a.txt", "aaa");
    File::WriteAllText(base / "sub" / "b.txt", "bbb");

    Path dst = Path::temp() / "bk_pmx_dst";
    SafeCleanup(dst);
    Path::CopyAll(base, dst, false);
    EXPECT_TRUE((dst / "a.txt").IsExists());
    EXPECT_TRUE((dst / "sub" / "b.txt").IsExists());

    SafeCleanup(base);
    SafeCleanup(dst);
}

TEST(PathManipExtraTest, RemoveIfFileMatchingPattern)
{
    std::string f = FreshPath() + ".log";
    File::WriteAllText(Path(f), "x");
    EXPECT_TRUE(Path(f).IsExists());
    Path::RemoveIf(Path(f), ".*\\.log");
    EXPECT_FALSE(Path(f).IsExists());
}

TEST(PathManipExtraTest, RemoveIfDirectoryContents)
{
    Path base = Path::temp() / "bk_pmx_rmif";
    SafeCleanup(base);
    Directory::Create(base);
    File::WriteAllText(base / "keep.txt", "k");
    File::WriteAllText(base / "drop.tmp", "d");
    Path::RemoveIf(base, ".*\\.tmp");
    EXPECT_TRUE((base / "keep.txt").IsExists());
    EXPECT_FALSE((base / "drop.tmp").IsExists());
    SafeCleanup(base);
}
