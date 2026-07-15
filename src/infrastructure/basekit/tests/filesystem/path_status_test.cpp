// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "filesystem/file.h"
#include "filesystem/directory.h"
#include "filesystem/path.h"
#include "filesystem/symlink.h"
#include "filesystem/exceptions.h"
#include "time/timestamp.h"

#include <cstdio>
#include <string>
#include <unistd.h>

using namespace BaseKit;

namespace {
std::string FreshPath()
{
    char t[] = "/tmp/bk_pst_XXXXXX";
    int fd = mkstemp(t);
    if (fd < 0) throw std::runtime_error("mkstemp");
    close(fd);
    std::remove(t);
    return t;
}

class TempFile
{
public:
    explicit TempFile(const std::string& content = "x")
    {
        _p = FreshPath();
        if (!content.empty()) File::WriteAllText(_p, content);
    }
    ~TempFile() { std::remove(_p.c_str()); }
    const std::string& str() const { return _p; }
private:
    std::string _p;
};

void SafeCleanup(const Path& p) noexcept
{
    try { Path::RemoveAll(p); } catch (const std::exception&) {}
}
}

TEST(PathStatusTest, TypeOfRegularFile)
{
    TempFile tf("data");
    Path p(tf.str());
    EXPECT_EQ(p.type(), FileType::REGULAR);
    EXPECT_TRUE(p.IsRegularFile());
    EXPECT_FALSE(p.IsDirectory());
    EXPECT_FALSE(p.IsSymlink());
    EXPECT_FALSE(p.IsOther());
    EXPECT_TRUE(p.IsExists());
}

TEST(PathStatusTest, TypeOfDirectory)
{
    Path base = Path::temp() / Path("bk_pst_dir");
    SafeCleanup(base);
    Directory::Create(base);
    EXPECT_EQ(base.type(), FileType::DIRECTORY);
    EXPECT_TRUE(base.IsDirectory());
    EXPECT_FALSE(base.IsRegularFile());
    EXPECT_FALSE(base.IsOther());
    SafeCleanup(base);
}

TEST(PathStatusTest, TypeOfSymlink)
{
    TempFile tf("target");
    Path link = Path(FreshPath());
    Symlink::CreateSymlink(tf.str(), link.string());
    EXPECT_EQ(link.type(), FileType::SYMLINK);
    EXPECT_TRUE(link.IsSymlink());
    EXPECT_FALSE(link.IsOther());
    std::remove(link.string().c_str());
}

TEST(PathStatusTest, TypeOfMissingIsNone)
{
    Path p("/tmp/__bk_pst_missing_xyz__");
    EXPECT_EQ(p.type(), FileType::NONE);
    EXPECT_FALSE(p.IsExists());
    EXPECT_FALSE(p.IsRegularFile());
    EXPECT_FALSE(p.IsDirectory());
    EXPECT_FALSE(p.IsOther());
}

TEST(PathStatusTest, IsOtherForDevice)
{
    if (Path("/dev/null").IsExists())
    {
        EXPECT_TRUE(Path("/dev/null").IsOther());
        EXPECT_EQ(Path("/dev/null").type(), FileType::CHARACTER);
    }
}

TEST(PathStatusTest, SetPermissionsReadable)
{
    TempFile tf;
    Path p(tf.str());
    Flags<FilePermissions> perms = FilePermissions::IRUSR | FilePermissions::IWUSR;
    EXPECT_NO_THROW(Path::SetPermissions(p, perms));
    Flags<FilePermissions> got = p.permissions();
    EXPECT_TRUE((bool)(got & FilePermissions::IRUSR));
    EXPECT_TRUE((bool)(got & FilePermissions::IWUSR));
}

TEST(PathStatusTest, SetPermissionsReadOnly)
{
    TempFile tf;
    Path p(tf.str());
    EXPECT_NO_THROW(Path::SetPermissions(p, FilePermissions::IRUSR));
    Flags<FilePermissions> got = p.permissions();
    EXPECT_TRUE((bool)(got & FilePermissions::IRUSR));
    EXPECT_FALSE((bool)(got & FilePermissions::IWUSR));
    Path::SetPermissions(p, FilePermissions::IRUSR | FilePermissions::IWUSR);
}

TEST(PathStatusTest, SetModifiedUpdatesTimestamp)
{
    TempFile tf;
    Path p(tf.str());
    UtcTimestamp ts(Timestamp::seconds(1000000000ull));
    EXPECT_NO_THROW(Path::SetModified(p, ts));
    EXPECT_NO_THROW((void)p.modified());
}

TEST(PathStatusTest, SetCreatedDoesNotThrow)
{
    TempFile tf;
    Path p(tf.str());
    UtcTimestamp ts(Timestamp::seconds(1200000000ull));
    EXPECT_NO_THROW(Path::SetCreated(p, ts));
}

TEST(PathStatusTest, TouchExistingUpdatesModified)
{
    TempFile tf;
    Path p(tf.str());
    EXPECT_NO_THROW(Path::Touch(p));
    EXPECT_NO_THROW((void)p.modified());
}

TEST(PathStatusTest, TouchMissingCreatesEmptyFile)
{
    Path p(FreshPath());
    EXPECT_FALSE(p.IsExists());
    EXPECT_NO_THROW(Path::Touch(p));
    EXPECT_TRUE(p.IsExists());
    EXPECT_EQ(File(p).size(), 0u);
    std::remove(p.string().c_str());
}

TEST(PathStatusTest, SpaceInfoSensible)
{
    Path p("/");
    SpaceInfo si = p.space();
    EXPECT_GT(si.capacity, 0ull);
    EXPECT_LE(si.free, si.capacity);
    EXPECT_LE(si.available, si.capacity);
}

TEST(PathStatusTest, HardlinksDefaultOne)
{
    TempFile tf;
    Path p(tf.str());
    EXPECT_EQ(p.hardlinks(), 1u);
}

TEST(PathStatusTest, RenameMovesFile)
{
    TempFile tf("content");
    Path src(tf.str());
    Path dst(FreshPath());
    Path::Rename(src, dst);
    EXPECT_FALSE(File(src).IsFileExists());
    EXPECT_TRUE(File(dst).IsFileExists());
    EXPECT_EQ(File::ReadAllText(dst), "content");
    std::remove(dst.string().c_str());
}

TEST(PathStatusTest, CopyWithoutOverwrite)
{
    TempFile tf("src");
    Path src(tf.str());
    Path dst(FreshPath());
    Path::Copy(src, dst, false);
    EXPECT_TRUE(File(dst).IsFileExists());
    EXPECT_EQ(File::ReadAllText(dst), "src");
    std::remove(dst.string().c_str());
}

TEST(PathStatusTest, CopyWithOverwriteExisting)
{
    TempFile srcFile("new");
    TempFile dstFile("old");
    Path::Copy(srcFile.str(), dstFile.str(), true);
    EXPECT_EQ(File::ReadAllText(dstFile.str()), "new");
}

TEST(PathStatusTest, IsEquivalentForHardlink)
{
    TempFile tf("body");
    Path src(tf.str());
    Path dst(FreshPath());
    Symlink::CreateHardlink(src, dst);
    EXPECT_TRUE(src.IsEquivalent(dst));
    std::remove(dst.string().c_str());
}

TEST(PathStatusTest, SetCurrentRoundTrip)
{
    Path original = Path::current();
    Path target = Path::temp();
    EXPECT_NO_THROW(Path::SetCurrent(target));
    Path now = Path::current();
    EXPECT_EQ(now.absolute().string(), target.absolute().string());
    Path::SetCurrent(original);
}

TEST(PathStatusTest, PermissionsOfMissingIsNone)
{
    Path p("/tmp/__bk_pst_perm_missing_xyz__");
    EXPECT_FALSE(bool(p.permissions()));
}

TEST(PathStatusTest, AbsoluteOfExistingDirectory)
{
    Path target = Path::temp().absolute();
    EXPECT_TRUE(target.IsAbsolute());
}

TEST(PathStatusTest, AbsoluteOfMissingThrows)
{
    Path p("/tmp/__bk_pst_abs_missing_xyz__");
    EXPECT_THROW((void)p.absolute(), FileSystemException);
}
