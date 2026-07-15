// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "filesystem/file.h"
#include "filesystem/path.h"
#include "filesystem/symlink.h"
#include "filesystem/exceptions.h"

#include <cstdio>
#include <string>
#include <unistd.h>

using namespace BaseKit;

namespace {
std::string FreshTempPath()
{
    char t[] = "/tmp/bk_sym_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    close(fd);
    std::remove(t);
    return t;
}

std::string MakeTempFile(const std::string& content = "data")
{
    char t[] = "/tmp/bk_symf_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    write(fd, content.data(), content.size());
    close(fd);
    return t;
}

void SafeRemove(const std::string& p)
{
    if (!p.empty()) std::remove(p.c_str());
}
}

TEST(SymlinkTest, CreateSymlinkAndReadTarget)
{
    std::string src = MakeTempFile("payload");
    std::string dst = FreshTempPath();

    Symlink link = Symlink::CreateSymlink(src, dst);
    EXPECT_TRUE(link.IsSymlinkExists());
    EXPECT_TRUE(Symlink(dst).IsSymlinkExists());
    EXPECT_EQ(link.target().string(), src);
    EXPECT_TRUE(link.IsTargetExists());
    EXPECT_TRUE(bool(link));

    SafeRemove(src);
    SafeRemove(dst);
}

TEST(SymlinkTest, IsSymlinkExistsFalseForRegularFile)
{
    std::string f = MakeTempFile();
    Symlink sym(f);
    EXPECT_FALSE(sym.IsSymlinkExists());
    SafeRemove(f);
}

TEST(SymlinkTest, IsSymlinkExistsFalseForMissing)
{
    Symlink sym("/tmp/__bk_no_such_symlink_xyz__");
    EXPECT_FALSE(sym.IsSymlinkExists());
    EXPECT_FALSE(bool(sym));
}

TEST(SymlinkTest, TargetOfRegularFileThrows)
{
    std::string f = MakeTempFile("hello");
    Symlink sym(f);
    EXPECT_THROW((void)sym.target(), FileSystemException);
    SafeRemove(f);
}

TEST(SymlinkTest, CreateHardlinkIncrementsLinkCount)
{
    std::string src = MakeTempFile();
    std::string dst = FreshTempPath();

    Symlink::CreateHardlink(src, dst);
    EXPECT_TRUE(File(dst).IsFileExists());
    EXPECT_EQ(Path(src).hardlinks(), 2u);
    EXPECT_EQ(Path(dst).hardlinks(), 2u);

    SafeRemove(src);
    SafeRemove(dst);
}

TEST(SymlinkTest, CopySymlinkFromExistingSymlink)
{
    std::string src = MakeTempFile("base");
    std::string linkPath = FreshTempPath();
    std::string copyPath = FreshTempPath();

    Symlink::CreateSymlink(src, linkPath);
    Symlink::CopySymlink(linkPath, copyPath);

    EXPECT_TRUE(Symlink(copyPath).IsSymlinkExists());
    EXPECT_EQ(Symlink(copyPath).target().string(), src);

    SafeRemove(src);
    SafeRemove(linkPath);
    SafeRemove(copyPath);
}

TEST(SymlinkTest, CopySymlinkFromRegularFile)
{
    std::string src = MakeTempFile("plain");
    std::string copyPath = FreshTempPath();

    Symlink::CopySymlink(src, copyPath);
    EXPECT_TRUE(Symlink(copyPath).IsSymlinkExists());
    EXPECT_EQ(Symlink(copyPath).target().string(), src);

    SafeRemove(src);
    SafeRemove(copyPath);
}

TEST(SymlinkTest, AssignAndBoolConversion)
{
    std::string src = MakeTempFile();
    std::string dst = FreshTempPath();
    Symlink::CreateSymlink(src, dst);

    Symlink sym;
    sym = Path(dst);
    EXPECT_EQ(sym.string(), dst);
    EXPECT_TRUE(bool(sym));

    SafeRemove(src);
    SafeRemove(dst);
}

TEST(SymlinkTest, CreateSymlinkOverExistingThrows)
{
    std::string src = MakeTempFile();
    std::string dst = MakeTempFile("existing");
    EXPECT_THROW(Symlink::CreateSymlink(src, dst), FileSystemException);
    SafeRemove(src);
    SafeRemove(dst);
}

TEST(SymlinkTest, HardlinkOverExistingThrows)
{
    std::string src = MakeTempFile();
    std::string dst = MakeTempFile("existing");
    EXPECT_THROW(Symlink::CreateHardlink(src, dst), FileSystemException);
    SafeRemove(src);
    SafeRemove(dst);
}

TEST(SymlinkTest, MemberAndFreeSwap)
{
    std::string a = "/tmp/bk_sym_a";
    std::string b = "/tmp/bk_sym_b";
    Symlink sa(a);
    Symlink sb(b);
    sa.swap(sb);
    EXPECT_EQ(sa.string(), b);
    EXPECT_EQ(sb.string(), a);
    swap(sa, sb);
    EXPECT_EQ(sa.string(), a);
    EXPECT_EQ(sb.string(), b);
}

TEST(SymlinkTest, CopyAndMoveSemantics)
{
    std::string src = MakeTempFile();
    std::string dst = FreshTempPath();
    Symlink::CreateSymlink(src, dst);

    Symlink orig(dst);
    Symlink copied(orig);
    EXPECT_EQ(copied.string(), dst);

    Symlink moved(std::move(copied));
    EXPECT_EQ(moved.string(), dst);

    Symlink assigned;
    assigned = orig;
    EXPECT_EQ(assigned.string(), dst);

    Symlink moveAssigned;
    moveAssigned = std::move(assigned);
    EXPECT_EQ(moveAssigned.string(), dst);

    SafeRemove(src);
    SafeRemove(dst);
}
