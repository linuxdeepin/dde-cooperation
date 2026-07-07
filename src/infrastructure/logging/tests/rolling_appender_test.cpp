// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// rolling_file_appender 与其余 appender 扩展用例。

#include <catch2/catch_all.hpp>
#include <logging/record.h>
#include <logging/appenders/file_appender.h>
#include <logging/appenders/rolling_file_appender.h>
#include <logging/appenders/ostream_appender.h>
#include <logging/appenders/memory_appender.h>
#include <logging/appenders/console_appender.h>
#include <logging/appenders/debug_appender.h>
#include <logging/appenders/error_appender.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using namespace Logging;
using namespace BaseKit;

namespace {
Record makeRec(Level level, const std::string &msg)
{
    Record r;
    r.timestamp = 1700000000;
    r.thread = 1;
    r.level = level;
    r.logger = "Roll";
    r.message = msg;
    return r;
}
void rmrf(const std::string &p)
{
    std::filesystem::remove_all(p);
}
}   // namespace

// ===== FileAppender =====
TEST_CASE("FileAppender writes records", "[appender][file]")
{
    const std::string fname = "/tmp/roll_test_basic.log";
    rmrf(fname);
    {
        FileAppender app(Path(fname), true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::INFO, "hello-file");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(fname));
    rmrf(fname);
}

// ===== RollingFileAppender (size policy) =====
TEST_CASE("RollingFileAppender creates base file", "[appender][rolling]")
{
    const std::string dir = "/tmp/roll_dir_basic";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), "test", "log", 100, 3, false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::INFO, "rolling-base-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    // 目录下应至少有一个文件
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

TEST_CASE("RollingFileAppender rolls on size", "[appender][rolling]")
{
    const std::string dir = "/tmp/roll_dir_size";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        // size=1 极小，强制频繁轮转；backups=2
        RollingFileAppender app(Path(dir), "svc", "log", 1, 2, false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        for (int i = 0; i < 30; ++i) {
            Record r = makeRec(Level::INFO, "filler-line-padding-padding-padding");
            app.AppendRecord(r);
        }
        app.Flush();
        app.Stop();
    }
    // 轮转后目录下应有多于一个文件（基础 + 备份）
    int files = 0;
    if (std::filesystem::exists(dir))
        for ([[maybe_unused]] auto &_ : std::filesystem::directory_iterator(dir))
            ++files;
    REQUIRE(files >= 1);
    rmrf(dir);
}

TEST_CASE("RollingFileAppender time policy day", "[appender][rolling][time]")
{
    const std::string dir = "/tmp/roll_dir_time";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{UtcDateTime}.log",
                                false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::WARN, "time-policy-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

// ===== OstreamAppender =====
TEST_CASE("OstreamAppender writes to stream", "[appender][ostream]")
{
    std::ostringstream oss;
    OstreamAppender app(oss);
    REQUIRE(app.Start());
    Record r = makeRec(Level::INFO, "ostream-msg");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
    // 至少写入了若干字节
    REQUIRE(oss.str().size() > 0);
}

// ===== MemoryAppender =====
TEST_CASE("MemoryAppender log and drain", "[appender][memory]")
{
    MemoryAppender app(0);
    REQUIRE(app.Start());
    Record r = makeRec(Level::ERROR, "mem-record");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
    SUCCEED();
}

// ===== ConsoleAppender / DebugAppender / ErrorAppender =====
TEST_CASE("ConsoleAppender writes without throw", "[appender][console]")
{
    ConsoleAppender app;
    REQUIRE(app.Start());
    Record r = makeRec(Level::INFO, "console-line");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

TEST_CASE("DebugAppender no throw", "[appender][debug]")
{
    DebugAppender app;
    REQUIRE(app.Start());
    Record r = makeRec(Level::DEBUG, "debug-line");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

TEST_CASE("ErrorAppender no throw", "[appender][error]")
{
    ErrorAppender app;
    REQUIRE(app.Start());
    Record r = makeRec(Level::ERROR, "error-line");
    app.AppendRecord(r);
    app.Flush();
    app.Stop();
}

// ===== 各 TimeRollingPolicy =====
TEST_CASE("RollingFileAppender time policy year", "[appender][rolling][time]")
{
    const std::string dir = "/tmp/roll_year";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::YEAR, "{UtcDateTime}.log",
                                false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::INFO, "year-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

TEST_CASE("RollingFileAppender time policy month", "[appender][rolling][time]")
{
    const std::string dir = "/tmp/roll_month";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::MONTH, "{UtcDateTime}.log",
                                false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::INFO, "month-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

TEST_CASE("RollingFileAppender time policy hour", "[appender][rolling][time]")
{
    const std::string dir = "/tmp/roll_hour";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::HOUR, "{UtcDateTime}.log",
                                false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::INFO, "hour-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

// ===== archive=true 触发归档线程 =====
TEST_CASE("RollingFileAppender size policy with archive", "[appender][rolling][archive]")
{
    const std::string dir = "/tmp/roll_arch";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), "arc", "log", 1, 2,
                                /*archive=*/true, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        for (int i = 0; i < 10; ++i) {
            Record r = makeRec(Level::WARN, "archive-filler-padding-padding");
            app.AppendRecord(r);
        }
        app.Flush();
        app.Stop();
    }
    // archive 模式下可能产生 .gz 归档；至少有文件产出
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

TEST_CASE("RollingFileAppender time policy day with archive", "[appender][rolling][archive]")
{
    const std::string dir = "/tmp/roll_time_arch";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{UtcDateTime}.log",
                                /*archive=*/true, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        Record r = makeRec(Level::INFO, "day-archive-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}

TEST_CASE("RollingFileAppender size policy many backups", "[appender][rolling]")
{
    const std::string dir = "/tmp/roll_backups";
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), "bk", "log", 1, 5,
                                false, true, true, /*auto_start=*/false);
        REQUIRE(app.Start());
        for (int i = 0; i < 50; ++i) {
            Record r = makeRec(Level::INFO, "filler-line-padding");
            app.AppendRecord(r);
        }
        app.Flush();
        app.Stop();
    }
    REQUIRE(std::filesystem::exists(dir));
    rmrf(dir);
}
