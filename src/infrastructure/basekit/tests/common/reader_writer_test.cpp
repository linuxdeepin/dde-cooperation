// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "common/reader.h"
#include "common/writer.h"
#include "filesystem/file.h"
#include "filesystem/path.h"

#include <string>
#include <vector>

using namespace BaseKit;

namespace {
void SafeCleanup(const Path& p) noexcept
{
    try { Path::RemoveAll(p); } catch (const std::exception&) {}
}
} // namespace

// 测试 Writer 的字符串/多行写入重载，以及 Reader 的整体读取接口
TEST(ReaderWriterTest, FileWriteAndReadAll)
{
    Path p = Path::temp() / "basekit_rw_test.txt";
    SafeCleanup(p);

    // 写入阶段：File 同时是 Writer
    {
        File f(p);
        f.OpenOrCreate(true, true); // 读+写

        // Writer::Write(const std::string&)
        EXPECT_EQ(f.Write(std::string("line1\n")), 6u);

        // Writer::Write(const std::vector<std::string>&) —— 逐行 + 行结束符
        std::vector<std::string> lines = {"line2", "line3"};
        f.Write(lines);
        f.Close();
    }

    // 读取阶段：File 同时是 Reader。
    // 注意：每次 ReadAll* 都会读到 EOF，因此每次读取前需重新打开文件。
    {
        File f(p);
        f.OpenOrCreate(true, false); // 只读
        std::string text = f.ReadAllText();
        EXPECT_NE(text.find("line1"), std::string::npos);
        EXPECT_NE(text.find("line2"), std::string::npos);
        EXPECT_NE(text.find("line3"), std::string::npos);
        f.Close();
    }
    {
        File f(p);
        f.OpenOrCreate(true, false);
        auto allLines = f.ReadAllLines(); // 覆盖按 \n 切分的逻辑
        EXPECT_GE(allLines.size(), 3u);
        bool hasLine3 = false;
        for (const auto& l : allLines)
            if (l == "line3")
                hasLine3 = true;
        EXPECT_TRUE(hasLine3);
        f.Close();
    }
    {
        File f(p);
        f.OpenOrCreate(true, false);
        auto bytes = f.ReadAllBytes();
        EXPECT_FALSE(bytes.empty());
        f.Close();
    }

    SafeCleanup(p);
}

// 测试 Reader::ReadAllLines 对 \r 回车换行的处理
TEST(ReaderWriterTest, ReadAllLinesCarriageReturn)
{
    Path p = Path::temp() / "basekit_rw_crlf.txt";
    SafeCleanup(p);

    {
        File f(p);
        f.OpenOrCreate(true, true);
        // 写入包含 \r 与 \n 的内容，覆盖 ReadAllLines 的两个分支
        std::string data = "a\rb\nc";
        f.Write(data.data(), data.size());
        f.Close();
    }

    {
        File f(p);
        f.OpenOrCreate(true, false);
        auto lines = f.ReadAllLines();
        // "a" 和 "b" 应作为独立行被切出
        bool hasA = false, hasB = false;
        for (const auto& l : lines) {
            if (l == "a") hasA = true;
            if (l == "b") hasB = true;
        }
        EXPECT_TRUE(hasA);
        EXPECT_TRUE(hasB);
        f.Close();
    }

    SafeCleanup(p);
}
