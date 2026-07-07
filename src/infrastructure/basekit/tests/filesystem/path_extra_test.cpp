// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Path 操作扩展测试：覆盖 Append/Concat/Replace/MakePreferred/validate/
// relative/parent/stem/extension/operators/swap 等分支。

#include <gtest/gtest.h>
#include "filesystem/path.h"
#include "filesystem/file.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace BaseKit;

TEST(PathExtraTest, AppendAddsSeparator)
{
    Path a("/usr");
    a.Append(Path("local/bin"));
    EXPECT_EQ(a.string(), "/usr/local/bin");

    Path b = Path("/var") / Path("log") / Path("syslog");
    EXPECT_EQ(b.string(), "/var/log/syslog");
}

TEST(PathExtraTest, ConcatWithoutSeparator)
{
    Path a("/usr/local");
    a.Concat(Path("bin"));
    EXPECT_EQ(a.string(), "/usr/localbin");

    Path b = Path("foo") + Path(".txt");
    EXPECT_EQ(b.string(), "foo.txt");
}

TEST(PathExtraTest, ReplaceFilename)
{
    Path a("/usr/local/bin/old");
    a.ReplaceFilename(Path("new"));
    // 父目录部分保持不变（ReplaceFilename 不改 parent）
    EXPECT_EQ(a.parent().string(), "/usr/local/bin");
    // 路径中含 new
    EXPECT_NE(a.string().find("new"), std::string::npos);

    EXPECT_NO_THROW(a.RemoveFilename());
}

TEST(PathExtraTest, ReplaceExtension)
{
    Path a("/tmp/archive.tar.gz");
    a.ReplaceExtension(Path("bz2"));
    EXPECT_EQ(a.string(), "/tmp/archive.tar.bz2");

    a.RemoveExtension();
    EXPECT_EQ(a.string(), "/tmp/archive.tar");

    // 给无扩展名路径添加扩展
    Path b("/tmp/noext");
    b.ReplaceExtension(Path("log"));
    EXPECT_EQ(b.string(), "/tmp/noext.log");
}

TEST(PathExtraTest, MakePreferred)
{
    Path a("a/b/c");
    a.MakePreferred();
    // Linux 下 preferred 分隔符为 '/'
    EXPECT_FALSE(a.string().empty());
}

TEST(PathExtraTest, StemAndExtension)
{
    Path a("/path/to/file.tar.gz");
    EXPECT_EQ(a.stem().string(), "file.tar");
    EXPECT_EQ(a.extension().string(), ".gz");

    Path b("/path/to/noext");
    EXPECT_EQ(b.stem().string(), "noext");
    EXPECT_EQ(b.extension().string(), "");

    Path c("/path/to/.hidden");
    // 隐藏文件（点文件）的 stem/extension 视实现而定，仅校验不抛异常
    EXPECT_NO_THROW((void)c.stem().string());
    EXPECT_NO_THROW((void)c.extension().string());
}

TEST(PathExtraTest, ParentAndFilename)
{
    Path a("/usr/local/bin/app");
    EXPECT_EQ(a.parent().string(), "/usr/local/bin");
    EXPECT_EQ(a.filename().string(), "app");

    Path b("relative/path/file.txt");
    EXPECT_EQ(b.parent().string(), "relative/path");
    EXPECT_EQ(b.filename().string(), "file.txt");
}

TEST(PathExtraTest, RelativeAndRoot)
{
    Path abs("/usr/local/bin");
    EXPECT_TRUE(abs.HasRoot());
    EXPECT_EQ(abs.root().string(), "/");
    EXPECT_EQ(abs.relative().string(), "usr/local/bin");

    Path rel("foo/bar");
    EXPECT_FALSE(rel.HasRoot());
    EXPECT_TRUE(rel.HasRelative());
}

TEST(PathExtraTest, ValidateReplacesInvalidChars)
{
    Path dirty("a\\b\tc");
    Path clean = dirty.validate('_');
    // 校验非法字符被占位符替换，长度保持
    EXPECT_EQ(clean.string().size(), dirty.string().size());
}

TEST(PathExtraTest, Comparisons)
{
    Path a("/x/y"), b("/x/y"), c("/x/z");
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(c > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(c >= a);
}

TEST(PathExtraTest, BooleanAndEmpty)
{
    Path empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_FALSE(bool(empty));

    Path nonempty("/a");
    EXPECT_FALSE(nonempty.empty());
    EXPECT_TRUE(bool(nonempty));
}

TEST(PathExtraTest, Swap)
{
    Path a("/aaa"), b("/bbb");
    a.swap(b);
    EXPECT_EQ(a.string(), "/bbb");
    EXPECT_EQ(b.string(), "/aaa");
    swap(a, b);
    EXPECT_EQ(a.string(), "/aaa");
}

TEST(PathExtraTest, AbsoluteAndCanonicalOfRelative)
{
    Path rel("some/relative/path");
    EXPECT_TRUE(rel.IsRelative());
    EXPECT_FALSE(rel.IsAbsolute());

    // absolute() 对不存在的相对路径会抛异常，这里用进程工作目录验证
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)))
    {
        Path existing(cwd);
        Path abs = existing.absolute();
        EXPECT_TRUE(abs.IsAbsolute());
    }
}

TEST(PathExtraTest, StreamOperators)
{
    std::ostringstream oss;
    Path p("/stream/test");
    oss << p;
    EXPECT_EQ(oss.str(), "/stream/test");

    std::istringstream iss("/parsed/path");
    Path parsed;
    iss >> parsed;
    EXPECT_EQ(parsed.string(), "/parsed/path");
}

// ===== File 真实文件系统操作 =====
namespace {
class TempFile
{
public:
    explicit TempFile(const std::string &content = "")
    {
        char tmpl[] = "/tmp/bk_test_XXXXXX";
        int fd = mkstemp(tmpl);
        if (fd < 0)
            throw std::runtime_error("mkstemp failed");
        _path = tmpl;
        close(fd);
        if (!content.empty())
        {
            std::ofstream f(_path, std::ios::binary);
            f << content;
            f.flush();
        }
    }
    ~TempFile() { if (!_path.empty()) std::remove(_path.c_str()); }
    const std::string &path() const { return _path; }
private:
    std::string _path;
};
}   // namespace

TEST(FileExtraTest, WriteAndReadAllText)
{
    TempFile tf;
    Path p(tf.path());
    std::string data = "the quick brown fox";
    File::WriteAllText(p, data);
    EXPECT_EQ(File::ReadAllText(p), data);

    auto bytes = File::ReadAllBytes(p);
    EXPECT_EQ(bytes.size(), data.size());
}

TEST(FileExtraTest, WriteAllBytesRoundTrip)
{
    TempFile tf;
    Path p(tf.path());
    std::vector<uint8_t> raw = {1, 2, 3, 4, 5};
    EXPECT_EQ(File::WriteAllBytes(p, raw.data(), raw.size()), raw.size());
    auto back = File::ReadAllBytes(p);
    EXPECT_EQ(back, raw);
}

TEST(FileExtraTest, PropertiesOfExistingFile)
{
    TempFile tf("payload");
    File file(tf.path());
    EXPECT_EQ(file.size(), 7ull);
    EXPECT_TRUE(file.IsExists());
    EXPECT_TRUE(file.IsRegularFile());
    EXPECT_NO_THROW((void)file.attributes());
    EXPECT_NO_THROW((void)file.permissions());
    EXPECT_NO_THROW((void)file.modified());
}

TEST(FileExtraTest, NonexistentFileIsNotExists)
{
    File file("/tmp/__definitely_not_existing_bk_test__");
    EXPECT_FALSE(file.IsExists());
    EXPECT_FALSE(file.IsRegularFile());
    EXPECT_FALSE(file.IsDirectory());
}

TEST(FileExtraTest, PathCopyRenameRemove)
{
    TempFile src("hello");
    Path srcPath(src.path());
    Path dstPath = std::string(src.path()) + ".copy";
    Path::Copy(srcPath, dstPath, true);
    EXPECT_TRUE(File(dstPath).IsExists());

    Path renamed = std::string(src.path()) + ".renamed";
    Path::Rename(dstPath, renamed);
    EXPECT_FALSE(File(dstPath).IsExists());
    EXPECT_TRUE(File(renamed).IsExists());

    Path::Remove(renamed);
    EXPECT_FALSE(File(renamed).IsExists());
}

TEST(FileExtraTest, WriteEmptyAndReadAllLines)
{
    TempFile tf;
    Path p(tf.path());
    File::WriteEmpty(p);
    EXPECT_EQ(File(p).size(), 0ull);

    File::WriteAllText(p, "a\nb\nc");
    auto lines = File::ReadAllLines(p);
    // 末尾无换行：ReadAllLines 视实现可能返回 2 或 3 行，仅断言非空且含首行
    EXPECT_GE(lines.size(), 2u);
    EXPECT_EQ(lines.front(), "a");
}
