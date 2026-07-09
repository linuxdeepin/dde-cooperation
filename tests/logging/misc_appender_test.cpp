// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 其余 appender 覆盖 (file/console/debug/error/ostream/memory/syslog)
// 全部填 record.raw (AppendRecord 跳过空 raw)。

#include "logging/appenders/file_appender.h"
#include "logging/appenders/console_appender.h"
#include "logging/appenders/debug_appender.h"
#include "logging/appenders/error_appender.h"
#include "logging/appenders/ostream_appender.h"
#include "logging/appenders/memory_appender.h"
#include "logging/appenders/syslog_appender.h"
#include "logging/record.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <sstream>
#include <string>

using namespace Logging;
using namespace BaseKit;

namespace {
Record makeRawRec(Level level, const std::string &msg)
{
    Record r;
    r.timestamp = 1700000000000000000ull;
    r.thread = 1;
    r.level = level;
    r.logger = "Misc";
    r.message = msg;
    r.raw.assign(msg.begin(), msg.end());
    r.raw.push_back('\n');
    return r;
}
void rmrf(const std::string &p) { std::filesystem::remove_all(p); }
}   // namespace

TEST(FileAppender, WriteAndRead)
{
    const std::string fname = "/tmp/log_misc_file.log";
    rmrf(fname);
    {
        FileAppender app(Path(fname), true, true, /*auto_start=*/false);
        ASSERT_TRUE(app.Start());
        Record r = makeRawRec(Level::INFO, "file-msg");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    EXPECT_TRUE(std::filesystem::exists(fname));
    rmrf(fname);
}

TEST(FileAppender, EmptyRawSkipped)
{
    const std::string fname = "/tmp/log_misc_file2.log";
    rmrf(fname);
    {
        FileAppender app(Path(fname), true, true, false);
        ASSERT_TRUE(app.Start());
        Record r;   // raw 为空 → 被跳过
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    SUCCEED();
    rmrf(fname);
}

// auto_start=true: 构造即 Start; 不手动 Stop, 析构走 IsStarted→Stop (覆盖构造 14 行 + 析构 21 行)
TEST(FileAppender, AutoStartAndDtorStop)
{
    const std::string fname = "/tmp/log_misc_autostart.log";
    rmrf(fname);
    {
        FileAppender app(Path(fname), true, true, /*auto_start=*/true);   // 构造触发 Start
        Record r = makeRawRec(Level::INFO, "auto-start-msg");
        app.AppendRecord(r);
        // 不调 app.Stop() —— 析构里 IsStarted()=true → Stop() → CloseFile()
    }
    EXPECT_TRUE(std::filesystem::exists(fname));
    rmrf(fname);
}

// 重复 Start/Stop 边界: 已 started 再 Start 返回 false; 已 stopped 再 Stop 返回 false
TEST(FileAppender, DoubleStartStop)
{
    const std::string fname = "/tmp/log_misc_ds.log";
    rmrf(fname);
    FileAppender app(Path(fname), true, true, false);
    EXPECT_TRUE(app.Start());
    EXPECT_FALSE(app.Start());   // 已 started → false
    EXPECT_TRUE(app.Stop());
    EXPECT_FALSE(app.Stop());    // 已 stopped → false
    rmrf(fname);
}

// 不可创建的路径: OpenOrCreate 抛 → PrepareFile catch (111/114/115/116) + 100ms 重试窗 (95/96)
// 注: 不触发 Write/Flush 的 IO 异常 —— 那些路径在 basekit 析构链里走 fatality→abort, 无法安全测试。
TEST(FileAppender, PrepareFileFailureAndRetryWindow)
{
    const std::string bad = "/nonexistent_log_dir_xyz/file.log";   // 父目录不存在且不可创建
    FileAppender app(Path(bad), true, true, /*auto_start=*/false);
    EXPECT_TRUE(app.Start());   // Start 调 PrepareFile 失败但仍设 _started=true
    Record r = makeRawRec(Level::INFO, "bad-path");
    app.AppendRecord(r);   // PrepareFile 失败 → catch 设 _retry; if(PrepareFile())=false 不写
    app.AppendRecord(r);   // 100ms 内 → retry window 直接 return false (95/96)
    app.Flush();           // 同样走 retry window 或失败 catch
    app.Stop();
    SUCCEED();
}

TEST(OstreamAppender, WriteToStream)
{
    std::ostringstream oss;
    OstreamAppender app(oss);
    ASSERT_TRUE(app.Start());
    Record r = makeRawRec(Level::INFO, "ostream-msg");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
    EXPECT_GT(oss.str().size(), 0u);
}

TEST(MemoryAppender, LogAndDrain)
{
    MemoryAppender app(0);
    ASSERT_TRUE(app.Start());
    Record r = makeRawRec(Level::ERROR, "mem");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
    SUCCEED();
}

// 遍历所有 level, 覆盖 ConsoleAppender::AppendRecord switch 全分支
TEST(ConsoleAppender, AllLevels)
{
    const Level levels[] = {Level::NONE, Level::FATAL, Level::ERROR,
                            Level::WARN,  Level::INFO,  Level::DEBUG, Level::ALL};
    ConsoleAppender app;
    ASSERT_TRUE(app.Start());
    for (Level lv : levels)
    {
        Record r = makeRawRec(lv, "console-level");
        app.AppendRecord(r);
    }
    app.Flush();
    app.Stop();
}

TEST(ConsoleAppender, EmptyRawSkipped)
{
    ConsoleAppender app;
    ASSERT_TRUE(app.Start());
    Record r;   // raw 为空 → 被跳过
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

// ConsoleAppender 用 Element 默认 Start/Stop (恒 true), 覆盖默认实现路径
TEST(ConsoleAppender, DefaultStartStop)
{
    ConsoleAppender app;
    EXPECT_TRUE(app.IsStarted());
    EXPECT_TRUE(app.Start());
    EXPECT_TRUE(app.Stop());
}

TEST(DebugAppender, WriteNoThrow)
{
    DebugAppender app;
    ASSERT_TRUE(app.Start());
    Record r = makeRawRec(Level::DEBUG, "debug");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

// 空 raw 被跳过 (覆盖 DebugAppender::AppendRecord 早返回)
TEST(DebugAppender, EmptyRawSkipped)
{
    DebugAppender app;
    ASSERT_TRUE(app.Start());
    Record r;
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

TEST(ErrorAppender, WriteNoThrow)
{
    ErrorAppender app;
    ASSERT_TRUE(app.Start());
    Record r = makeRawRec(Level::ERROR, "error");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

// 空 raw 被跳过 (覆盖 ErrorAppender::AppendRecord 早返回)
TEST(ErrorAppender, EmptyRawSkipped)
{
    ErrorAppender app;
    ASSERT_TRUE(app.Start());
    Record r;
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

// 遍历所有 level, 覆盖 SyslogAppender::AppendRecord switch 全分支 + default
TEST(SyslogAppender, AllLevels)
{
    const Level levels[] = {Level::NONE,  Level::FATAL, Level::ERROR,
                            Level::WARN,   Level::INFO,  Level::DEBUG, Level::ALL};
    SyslogAppender app;
    ASSERT_TRUE(app.Start());
    for (Level lv : levels)
    {
        Record r = makeRawRec(lv, "syslog-level");
        app.AppendRecord(r);
    }
    app.Flush();
    app.Stop();
}

TEST(SyslogAppender, EmptyRawSkipped)
{
    SyslogAppender app;
    ASSERT_TRUE(app.Start());
    Record r;
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}
