// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Path 高级 API 覆盖：静态工厂、RemoveTrailingSeparators、Copy/RemoveAll、
// IsEquivalent、space、attributes、permissions 等。

#include <gtest/gtest.h>
#include "filesystem/path.h"
#include "filesystem/file.h"

#include <cstdio>
#include <string>
#include <unistd.h>

using namespace BaseKit;

// ===== 静态工厂 =====
TEST(PathAdvancedTest, StaticFactoriesReturnValidPaths)
{
    EXPECT_FALSE(Path::initial().string().empty());
    EXPECT_FALSE(Path::current().string().empty());
    EXPECT_FALSE(Path::executable().string().empty());
    EXPECT_FALSE(Path::home().string().empty());
    EXPECT_FALSE(Path::temp().string().empty());
}

TEST(PathAdvancedTest, UniqueGeneratesDistinctPaths)
{
    Path a = Path::unique();
    Path b = Path::unique();
    EXPECT_NE(a.string(), b.string());
}

// ===== RemoveTrailingSeparators =====
TEST(PathAdvancedTest, RemoveTrailingSeparators)
{
    Path p("/usr/local/bin///");
    p.RemoveTrailingSeparators();
    EXPECT_EQ(p.string(), "/usr/local/bin");
}

TEST(PathAdvancedTest, RemoveTrailingSeparatorsNoChange)
{
    Path p("/usr/local");
    p.RemoveTrailingSeparators();
    EXPECT_EQ(p.string(), "/usr/local");
}

// ===== IsEquivalent =====
TEST(PathAdvancedTest, IsEquivalentSameFile)
{
    char cwd[4096];
    ASSERT_TRUE(getcwd(cwd, sizeof(cwd)));
    Path here(cwd);
    EXPECT_TRUE(here.IsEquivalent(here));
}

TEST(PathAdvancedTest, IsEquivalentDifferentFiles)
{
    Path a("/usr");
    Path b("/tmp");
    EXPECT_FALSE(a.IsEquivalent(b));
}

// ===== 真实文件的属性 / 权限 / 时间戳 / 大小 =====
namespace {
class TempFile
{
public:
    explicit TempFile(const std::string &content = "")
    {
        char t[] = "/tmp/bk_adv_XXXXXX";
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
}   // namespace

TEST(PathAdvancedTest, AttributesAndPermissionsOfExistingFile)
{
    TempFile tf("x");
    Path p(tf.str());
    EXPECT_NO_THROW((void)p.attributes());
    EXPECT_NO_THROW((void)p.permissions());
    EXPECT_NO_THROW((void)p.modified());
    EXPECT_NO_THROW((void)p.created());
}

TEST(PathAdvancedTest, SpaceInfoAccessible)
{
    Path p("/");
    EXPECT_NO_THROW((void)p.space());
}

TEST(PathAdvancedTest, HardlinksForExistingFile)
{
    TempFile tf;
    Path p(tf.str());
    EXPECT_NO_THROW((void)p.hardlinks());
}

// ===== CopyAll / RemoveAll 对目录 =====
TEST(PathAdvancedTest, CopyAllDirectoryRoundTrip)
{
    Path src = Path::temp() / Path("bk_cpall_src");
    Path::RemoveAll(src);
    File::WriteAllText(src / Path("a.txt"), "a");
    File::WriteAllText(src / Path("b.txt"), "b");

    Path dst = Path::temp() / Path("bk_cpall_dst");
    Path::RemoveAll(dst);
    Path::CopyAll(src, dst, true);
    EXPECT_TRUE(File(dst / Path("a.txt")).IsExists());
    EXPECT_TRUE(File(dst / Path("b.txt")).IsExists());

    Path::RemoveAll(src);
    Path::RemoveAll(dst);
    EXPECT_FALSE(File(src).IsExists());
}

TEST(PathAdvancedTest, CopyIfWithPattern)
{
    Path src = Path::temp() / Path("bk_cpif_src");
    Path::RemoveAll(src);
    File::WriteAllText(src / Path("keep.txt"), "k");
    File::WriteAllText(src / Path("drop.log"), "d");

    Path dst = Path::temp() / Path("bk_cpif_dst");
    Path::RemoveAll(dst);
    Path::CopyIf(src, dst, "*.txt", true);
    EXPECT_TRUE(File(dst / Path("keep.txt")).IsExists());

    Path::RemoveAll(src);
    Path::RemoveAll(dst);
}

TEST(PathAdvancedTest, RemoveIfPattern)
{
    Path dir = Path::temp() / Path("bk_rif_dir");
    Path::RemoveAll(dir);
    File::WriteAllText(dir / Path("a.tmp"), "1");
    File::WriteAllText(dir / Path("b.keep"), "2");
    Path::RemoveIf(dir, "*.tmp");
    EXPECT_FALSE(File(dir / Path("a.tmp")).IsExists());
    EXPECT_TRUE(File(dir / Path("b.keep")).IsExists());
    Path::RemoveAll(dir);
}

// ===== 分解：root/parent/filename/stem/extension 组合 =====
TEST(PathAdvancedTest, FullDecomposition)
{
    Path p("/usr/local/bin/app.tar.gz");
    EXPECT_EQ(p.root().string(), "/");
    EXPECT_EQ(p.parent().string(), "/usr/local/bin");
    EXPECT_EQ(p.filename().string(), "app.tar.gz");
    EXPECT_EQ(p.stem().string(), "app.tar");
    EXPECT_EQ(p.extension().string(), ".gz");
    EXPECT_TRUE(p.HasRoot());
    EXPECT_TRUE(p.HasParent());
    EXPECT_TRUE(p.HasFilename());
    EXPECT_TRUE(p.HasStem());
    EXPECT_TRUE(p.HasExtension());
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_FALSE(p.IsRelative());
}
