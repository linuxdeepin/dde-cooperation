// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "filesystem/directory.h"
#include "filesystem/file.h"
#include "filesystem/path.h"
#include "filesystem/symlink.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace BaseKit;

namespace {
// 安全清理：Path::RemoveAll 对不存在的路径会抛异常，这里吞掉该异常以保证幂等。
void SafeCleanup(const Path& p) noexcept
{
    try { Path::RemoveAll(p); } catch (const std::exception&) {}
}

void WriteFile(const Path& path, const std::string& content)
{
    File f(path);
    f.OpenOrCreate(true, true);
    f.Write(content.data(), content.size());
    f.Close();
}

// Directory/File/Symlink 均派生自 Path，用模板统一处理。
template <typename T>
bool HasName(const std::vector<T>& entries, const std::string& name)
{
    for (const auto& p : entries)
        if (p.filename().string() == name)
            return true;
    return false;
}

// 创建唯一的临时目录树:
//   <base>/a.txt
//   <base>/sub/b.txt
//   <base>/sub/sub2/c.txt
//   <base>/link_a.txt -> a.txt (符号链接)
Path MakeTestTree()
{
    Path base = Path::temp() / "basekit_dir_test";
    SafeCleanup(base);

    Directory::CreateTree(base / "sub" / "sub2");
    WriteFile(base / "a.txt", "aaa");
    WriteFile(base / "sub" / "b.txt", "bbb");
    WriteFile(base / "sub" / "sub2" / "c.txt", "ccc");
    Symlink::CreateSymlink(base / "a.txt", base / "link_a.txt");

    return base;
}
} // namespace

// 测试目录创建与存在性判断
TEST(DirectoryTest, CreateAndExists)
{
    Path base = Path::temp() / "basekit_dir_create";
    SafeCleanup(base);

    Directory dir(base);
    EXPECT_FALSE(dir.IsDirectoryExists());
    EXPECT_FALSE(bool(dir));

    Directory created = Directory::Create(base);
    EXPECT_TRUE(created.IsDirectoryExists());
    EXPECT_TRUE(bool(created));

    // Create 是幂等的：对已存在目录再次调用不会抛异常
    Directory again = Directory::Create(base);
    EXPECT_TRUE(again.IsDirectoryExists());

    // 对普通文件路径调用 IsDirectoryExists 应返回 false (ENOTDIR 分支)
    Path filePath = base / "not_a_dir.txt";
    WriteFile(filePath, "x");
    EXPECT_FALSE(Directory(filePath).IsDirectoryExists());

    SafeCleanup(base);
}

// 测试递归创建目录树
TEST(DirectoryTest, CreateTree)
{
    Path base = Path::temp() / "basekit_tree_test";
    SafeCleanup(base);

    Path deep = base / "a" / "b" / "c";
    Directory tree = Directory::CreateTree(deep);
    EXPECT_TRUE(tree.IsDirectoryExists());
    EXPECT_TRUE((base / "a").IsDirectory());
    EXPECT_TRUE((base / "a" / "b").IsDirectory());

    // 对已存在的完整树再次 CreateTree 应直接返回
    Directory tree2 = Directory::CreateTree(deep);
    EXPECT_TRUE(tree2.IsDirectoryExists());

    SafeCleanup(base);
}

// 测试目录是否为空
TEST(DirectoryTest, IsDirectoryEmpty)
{
    Path base = Path::temp() / "basekit_empty_test";
    SafeCleanup(base);
    Directory::Create(base);

    EXPECT_TRUE(Directory(base).IsDirectoryEmpty());

    WriteFile(base / "x.txt", "x");
    EXPECT_FALSE(Directory(base).IsDirectoryEmpty());

    SafeCleanup(base);
}

// 测试目录非递归迭代 (begin/end)
TEST(DirectoryTest, IterateBeginEnd)
{
    Path base = MakeTestTree();
    Directory dir(base);

    std::vector<std::string> names;
    for (auto it = dir.begin(); it != dir.end(); ++it)
        names.push_back(it->filename().string());

    EXPECT_NE(std::find(names.begin(), names.end(), "a.txt"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "sub"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "link_a.txt"), names.end());

    SafeCleanup(base);
}

// 测试目录递归迭代 (rbegin/rend)
TEST(DirectoryTest, IterateRecursive)
{
    Path base = MakeTestTree();
    Directory dir(base);

    std::vector<std::string> names;
    for (auto it = dir.rbegin(); it != dir.rend(); ++it)
        names.push_back(it->filename().string());

    EXPECT_NE(std::find(names.begin(), names.end(), "c.txt"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "b.txt"), names.end());

    SafeCleanup(base);
}

// 测试 DirectoryIterator 的拷贝/移动/后置自增/operator-> 等操作
TEST(DirectoryTest, IteratorOperations)
{
    Path base = MakeTestTree();
    Directory dir(base);

    // 后置 ++ 与 operator->
    int count = 0;
    for (auto it = dir.begin(); it != dir.end(); it++)
    {
        (void)it->filename().string(); // operator->
        ++count;
    }
    EXPECT_GT(count, 0);

    // 拷贝构造 / 移动构造
    auto it2 = dir.begin();
    DirectoryIterator it3(it2);            // 拷贝构造
    DirectoryIterator it4(std::move(it3)); // 移动构造
    EXPECT_EQ(it2, it4);

    // 拷贝赋值 / 移动赋值
    auto it5 = dir.begin();
    DirectoryIterator it6;
    it6 = it5;                             // 拷贝赋值
    DirectoryIterator it7;
    it7 = std::move(it6);                  // 移动赋值
    EXPECT_EQ(it5, it7);

    // 默认构造的迭代器互相比相等（均为 end）
    DirectoryIterator a, b;
    EXPECT_EQ(a, b);
    EXPECT_FALSE(a != b);

    SafeCleanup(base);
}

// 测试 GetEntries / GetEntriesRecursive
TEST(DirectoryTest, GetEntries)
{
    Path base = MakeTestTree();
    Directory dir(base);

    auto entries = dir.GetEntries();
    EXPECT_FALSE(entries.empty());

    auto txtEntries = dir.GetEntries("a.txt");
    EXPECT_EQ(txtEntries.size(), 1u);

    auto recEntries = dir.GetEntriesRecursive();
    EXPECT_TRUE(HasName(recEntries, "c.txt"));

    SafeCleanup(base);
}

// 测试 GetDirectories / GetDirectoriesRecursive
TEST(DirectoryTest, GetDirectories)
{
    Path base = MakeTestTree();
    Directory dir(base);

    auto dirs = dir.GetDirectories();
    EXPECT_TRUE(HasName(dirs, "sub"));

    auto dirsRec = dir.GetDirectoriesRecursive();
    EXPECT_TRUE(HasName(dirsRec, "sub2"));

    SafeCleanup(base);
}

// 测试 GetFiles / GetFilesRecursive
TEST(DirectoryTest, GetFiles)
{
    Path base = MakeTestTree();
    Directory dir(base);

    auto files = dir.GetFiles();
    EXPECT_TRUE(HasName(files, "a.txt"));

    auto filesRec = dir.GetFilesRecursive();
    EXPECT_TRUE(HasName(filesRec, "c.txt"));

    SafeCleanup(base);
}

// 测试 GetSymlinks / GetSymlinksRecursive
TEST(DirectoryTest, GetSymlinks)
{
    Path base = MakeTestTree();
    Directory dir(base);

    auto links = dir.GetSymlinks();
    EXPECT_TRUE(HasName(links, "link_a.txt"));

    auto linksRec = dir.GetSymlinksRecursive();
    EXPECT_FALSE(linksRec.empty());

    SafeCleanup(base);
}

// 测试 swap
TEST(DirectoryTest, Swap)
{
    Path baseA = Path::temp() / "basekit_swap_a";
    Path baseB = Path::temp() / "basekit_swap_b";
    SafeCleanup(baseA);
    SafeCleanup(baseB);
    Directory::Create(baseA);
    Directory::Create(baseB);

    Directory da(baseA);
    Directory db(baseB);
    da.swap(db);
    EXPECT_EQ(da.string(), baseB.string());
    EXPECT_EQ(db.string(), baseA.string());

    swap(da, db);
    EXPECT_EQ(da.string(), baseA.string());
    EXPECT_EQ(db.string(), baseB.string());

    SafeCleanup(baseA);
    SafeCleanup(baseB);
}
