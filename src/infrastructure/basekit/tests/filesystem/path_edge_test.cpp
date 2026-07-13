// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Path 纯逻辑边界用例：Windows 风格根、stem/extension 边界、空路径操作、
// 替换/移除语义、MakePreferred、validate、canonical 字符串归并、操作符与流。

#include <gtest/gtest.h>
#include "filesystem/path.h"

#include <sstream>
#include <string>

using namespace BaseKit;

// ===== Windows 风格根路径（Linux 下走 root() 字符串分支）=====
TEST(PathEdgeTest, UNCShareRootDecomposition)
{
    Path p("\\\\net\\share\\foo");
    EXPECT_TRUE(p.HasRoot());
    EXPECT_EQ(p.root().string(), "\\\\net\\");
    EXPECT_EQ(p.parent().string(), "\\\\net\\share");
    EXPECT_EQ(p.filename().string(), "foo");
}

TEST(PathEdgeTest, DriveLetterBackslashRoot)
{
    Path p("C:\\x\\y");
    EXPECT_TRUE(p.HasRoot());
    EXPECT_EQ(p.root().string(), "C:\\");
    EXPECT_EQ(p.parent().string(), "C:\\x");
    EXPECT_EQ(p.filename().string(), "y");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_FALSE(p.IsRelative());
}

TEST(PathEdgeTest, DriveLetterSlashRoot)
{
    Path p("C:/x/y");
    EXPECT_TRUE(p.HasRoot());
    EXPECT_EQ(p.root().string(), "C:/");
    EXPECT_EQ(p.parent().string(), "C:/x");
    EXPECT_EQ(p.filename().string(), "y");
}

TEST(PathEdgeTest, TripleSlashRootCollapsesToOne)
{
    Path p("///foo");
    EXPECT_TRUE(p.HasRoot());
    EXPECT_EQ(p.root().string(), "/");
    EXPECT_EQ(p.parent().string(), "/");
    EXPECT_EQ(p.filename().string(), "foo");
}

// ===== stem / extension 边界 =====
TEST(PathEdgeTest, HiddenFileStemAndExtension)
{
    Path p("/path/to/.hidden");
    // 点文件：扩展名吞掉整个 ".hidden"，主干退化为 "."
    EXPECT_EQ(p.extension().string(), ".hidden");
    EXPECT_EQ(p.stem().string(), ".");
    EXPECT_TRUE(p.HasExtension());
}

TEST(PathEdgeTest, DoubleExtensionSplit)
{
    Path p("/d/file.tar.gz");
    EXPECT_EQ(p.stem().string(), "file.tar");
    EXPECT_EQ(p.extension().string(), ".gz");
}

TEST(PathEdgeTest, NoExtensionStemOnly)
{
    Path p("noext");
    EXPECT_EQ(p.stem().string(), "noext");
    EXPECT_EQ(p.extension().string(), "");
    EXPECT_FALSE(p.HasExtension());
}

TEST(PathEdgeTest, TrailingDotHasNoExtension)
{
    Path p("/dir/td.");
    EXPECT_EQ(p.extension().string(), "");
    EXPECT_EQ(p.stem().string(), "td.");
    EXPECT_FALSE(p.HasExtension());
}

// ===== Append / Concat / Assign 与空路径 =====
TEST(PathEdgeTest, AppendToEmptyTakesValue)
{
    Path p;
    p.Append(Path("x"));
    EXPECT_EQ(p.string(), "x");
}

TEST(PathEdgeTest, AppendEmptyAddsSeparator)
{
    Path p("/a");
    p.Append(Path(""));
    EXPECT_EQ(p.string(), "/a/");
}

TEST(PathEdgeTest, ConcatToEmpty)
{
    Path p;
    p.Concat(Path("ab"));
    EXPECT_EQ(p.string(), "ab");
}

TEST(PathEdgeTest, ConcatAppendsRaw)
{
    Path p("foo");
    p.Concat(Path("bar"));
    EXPECT_EQ(p.string(), "foobar");
}

TEST(PathEdgeTest, AssignReplacesContent)
{
    Path p("/old");
    p.Assign(Path("/new"));
    EXPECT_EQ(p.string(), "/new");
}

// ===== ReplaceExtension / ReplaceFilename =====
TEST(PathEdgeTest, ReplaceExtensionWithLeadingDot)
{
    Path p("a.txt");
    p.ReplaceExtension(".txt");
    EXPECT_EQ(p.string(), "a.txt");
}

TEST(PathEdgeTest, ReplaceExtensionWithoutLeadingDot)
{
    Path p("a.txt");
    p.ReplaceExtension("txt");
    EXPECT_EQ(p.string(), "a.txt");
}

TEST(PathEdgeTest, ReplaceExtensionAddsToBareName)
{
    Path p("noext");
    p.ReplaceExtension(".log");
    EXPECT_EQ(p.string(), "noext.log");
}

TEST(PathEdgeTest, ReplaceFilenameKeepsParent)
{
    Path p("/usr/local/bin/old");
    p.ReplaceFilename("new");
    EXPECT_EQ(p.string(), "/usr/local/bin/new");
    EXPECT_EQ(p.parent().string(), "/usr/local/bin");
}

// ===== RemoveFilename / RemoveExtension / RemoveTrailingSeparators =====
TEST(PathEdgeTest, RemoveFilenameLeavesParent)
{
    Path p("/a/b.txt");
    p.RemoveFilename();
    EXPECT_EQ(p.string(), "/a");
}

TEST(PathEdgeTest, RemoveExtensionLeavesStem)
{
    Path p("/a/b.txt");
    p.RemoveExtension();
    EXPECT_EQ(p.string(), "/a/b");
}

TEST(PathEdgeTest, RemoveTrailingSeparatorsOnDriveRootKeepsSeparator)
{
    // 分隔符紧跟冒号时被视为根的一部分，不被移除
    Path p("C:/");
    p.RemoveTrailingSeparators();
    EXPECT_EQ(p.string(), "C:/");
}

TEST(PathEdgeTest, RemoveTrailingSeparatorsStripsExtras)
{
    Path p("/a/b///");
    p.RemoveTrailingSeparators();
    EXPECT_EQ(p.string(), "/a/b");
}

// ===== MakePreferred / validate =====
TEST(PathEdgeTest, MakePreferredConvertsBackslashOnUnix)
{
    Path p("a\\b\\c");
    p.MakePreferred();
    EXPECT_EQ(p.string(), "a/b/c");
}

TEST(PathEdgeTest, ValidateReplacesDeprecatedChars)
{
    EXPECT_EQ(Path(":").validate('_').string(), "_");
    EXPECT_EQ(Path("*").validate('_').string(), "_");
    EXPECT_EQ(Path("?").validate('_').string(), "_");
    EXPECT_EQ(Path("a:b").validate('_').string(), "a_b");
}

TEST(PathEdgeTest, ValidateKeepsSeparators)
{
    // 分隔符（/ 和 \）不被替换
    EXPECT_EQ(Path("a/b").validate('_').string(), "a/b");
}

TEST(PathEdgeTest, DeprecatedCharDetection)
{
    EXPECT_TRUE(Path::deprecated('*'));
    EXPECT_TRUE(Path::deprecated(':'));
    EXPECT_FALSE(Path::deprecated('a'));
    EXPECT_EQ(Path::deprecated().size(), 10u);
}

// ===== canonical 字符串归并（仅处理 . 与 ..）=====
TEST(PathEdgeTest, CanonicalResolvesParentSegment)
{
    EXPECT_EQ(Path("/a/b/../c").canonical().string(), "/a/c");
}

TEST(PathEdgeTest, CanonicalResolvesCurrentSegment)
{
    EXPECT_EQ(Path("/a/./b").canonical().string(), "/a/b");
}

TEST(PathEdgeTest, CanonicalOfParentAboveRootYieldsEmpty)
{
    // parent() 越过根后结果为空，canonical 直接返回空路径
    EXPECT_EQ(Path("/x/../..").canonical().string(), "");
}

// ===== swap / 操作符 / 流 =====
TEST(PathEdgeTest, MemberAndFreeSwap)
{
    Path a("/aaa"), b("/bbb");
    a.swap(b);
    EXPECT_EQ(a.string(), "/bbb");
    EXPECT_EQ(b.string(), "/aaa");
    swap(a, b);
    EXPECT_EQ(a.string(), "/aaa");
    EXPECT_EQ(b.string(), "/bbb");
}

TEST(PathEdgeTest, CompoundAndBinaryOperators)
{
    Path a("/a");
    a /= Path("b");
    EXPECT_EQ(a.string(), "/a/b");

    Path b = Path("/x") / Path("y");
    EXPECT_EQ(b.string(), "/x/y");

    Path c("/a");
    c += Path("b");
    EXPECT_EQ(c.string(), "/ab");

    Path d = Path("foo") + Path(".txt");
    EXPECT_EQ(d.string(), "foo.txt");
}

TEST(PathEdgeTest, ComparisonOperators)
{
    Path a("/x/y"), b("/x/y"), c("/x/z");
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a != c);
    EXPECT_TRUE(a < c);
    EXPECT_TRUE(c > a);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a >= b);
    EXPECT_FALSE(a != b);
}

TEST(PathEdgeTest, StreamOutputAndInput)
{
    std::ostringstream oss;
    oss << Path("/stream/test");
    EXPECT_EQ(oss.str(), "/stream/test");

    std::istringstream iss("/parsed/path");
    Path parsed;
    iss >> parsed;
    EXPECT_EQ(parsed.string(), "/parsed/path");
}

// ===== empty / Clear / string / wstring =====
TEST(PathEdgeTest, EmptyAndClearAndAccessors)
{
    Path p;
    EXPECT_TRUE(p.empty());
    EXPECT_FALSE(bool(p));
    EXPECT_EQ(p.string(), "");

    Path q("/abc");
    EXPECT_FALSE(q.empty());
    EXPECT_TRUE(bool(q));
    EXPECT_EQ(q.string(), "/abc");
    EXPECT_EQ(q.wstring(), L"/abc");

    q.Clear();
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.string(), "");
}

TEST(PathEdgeTest, SeparatorIsSlashOnUnix)
{
    EXPECT_EQ(Path::separator(), '/');
}
