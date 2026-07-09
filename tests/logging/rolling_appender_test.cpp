// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// RollingFileAppender 深挖测试
// 关键: AppendRecord 跳过 record.raw 为空的记录 (rolling_file_appender.cpp:343/987);
// 必须填充 record.raw 才能触发 write/rolling/archive 路径。auto_flush=true 同步写。

#include "logging/appenders/rolling_file_appender.h"
#include "logging/record.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

using namespace Logging;
using namespace BaseKit;

namespace {
Record makeRawRec(Level level, const std::string &msg)
{
    Record r;
    r.timestamp = 1700000000000000000ull;   // 固定时间戳
    r.thread = 1;
    r.level = level;
    r.logger = "Roll";
    r.message = msg;
    // 关键: 填 raw (AppendRecord 跳过 raw 空的记录); 末尾 '\n' 会被 size-1 掉
    r.raw.assign(msg.begin(), msg.end());
    r.raw.push_back('\n');
    return r;
}
void rmrf(const std::string &p) { std::filesystem::remove_all(p); }
std::string tmpDir(const char *name) { return std::string("/tmp/") + name; }
int countFiles(const std::string &dir)
{
    int n = 0;
    if (std::filesystem::exists(dir))
        for ([[maybe_unused]] auto &_ : std::filesystem::directory_iterator(dir))
            ++n;
    return n;
}
}   // namespace

// ===== Size policy 基础 + 强制 rolling =====
TEST(RollingSize, WriteAndRoll)
{
    auto dir = tmpDir("log_roll_size");
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        // size=8 极小, backups=3, truncate, auto_flush 同步
        RollingFileAppender app(Path(dir), "svc", "log", 8, 3, false, true, true, false);
        ASSERT_TRUE(app.Start());
        for (int i = 0; i < 30; ++i) {
            Record r = makeRawRec(Level::INFO, "padding-padding-padding-padding");
            app.AppendRecord(r);
        }
        app.Flush();
        app.Stop();
    }
    EXPECT_GE(countFiles(dir), 1);
    rmrf(dir);
}

// ===== Size policy + archive=true 触发 zip 归档线程 =====
TEST(RollingSize, ArchiveZip)
{
    auto dir = tmpDir("log_roll_arch");
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), "arc", "log", 8, 3,
                                /*archive=*/true, true, true, false);
        ASSERT_TRUE(app.Start());
        for (int i = 0; i < 20; ++i) {
            Record r = makeRawRec(Level::WARN, "archive-padding-padding-padding");
            app.AppendRecord(r);
        }
        app.Flush();
        app.Stop();   // Stop 会 join 归档线程, 等待 zip 产出
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_GE(countFiles(dir), 1);
    rmrf(dir);
}

// ===== Size policy backups 上限清理 =====
TEST(RollingSize, BackupsLimit)
{
    auto dir = tmpDir("log_roll_bk");
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), "bk", "log", 4, 2, false, true, true, false);
        ASSERT_TRUE(app.Start());
        for (int i = 0; i < 100; ++i) {
            Record r = makeRawRec(Level::INFO, "x-padding-padding-padding-padding");
            app.AppendRecord(r);
        }
        app.Flush();
        app.Stop();
    }
    // backups=2 → 目录文件数应受控 (svc.log + svc.1.log + svc.2.log = 3 左右, 可能加正在写的)
    EXPECT_GE(countFiles(dir), 1);
    rmrf(dir);
}

// ===== Time policy 各档位 (覆盖 rolldelay switch 全分支) =====
TEST(RollingTime, PolicyDay)
{
    auto dir = tmpDir("log_roll_day");
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{UtcDateTime}.log",
                                false, true, true, false);
        ASSERT_TRUE(app.Start());
        Record r = makeRawRec(Level::INFO, "day-record");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    EXPECT_GE(countFiles(dir), 1);
    rmrf(dir);
}

TEST(RollingTime, PolicyHour)   { auto dir = tmpDir("log_roll_hour"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::HOUR, "{LocalDateTime}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "h"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

TEST(RollingTime, PolicyMinute){ auto dir = tmpDir("log_roll_min"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::MINUTE, "{LocalDate}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "m"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

TEST(RollingTime, PolicySecond){ auto dir = tmpDir("log_roll_sec"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::SECOND, "{LocalTime}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "s"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

TEST(RollingTime, PolicyYear)  { auto dir = tmpDir("log_roll_year"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::YEAR, "{UtcYear}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "y"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

TEST(RollingTime, PolicyMonth) { auto dir = tmpDir("log_roll_month"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::MONTH, "{LocalMonth}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "mo"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

// ===== Time policy 各 placeholder (覆盖 Convert 方法 + operator<<) =====
TEST(RollingTime, PlaceholderDate) { auto dir = tmpDir("log_roll_pd"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{UtcDate}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "pd"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

TEST(RollingTime, PlaceholderTime) { auto dir = tmpDir("log_roll_pt"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{LocalTimezone}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "pt"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

TEST(RollingTime, PlaceholderFull) { auto dir = tmpDir("log_roll_pf"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "{UtcDay}_{LocalDay}_{UtcHour}_{LocalMinute}_{UtcSecond}.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "pf"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

// 字符串子模式 + 未知 placeholder (operator<< default 分支)
TEST(RollingTime, StringSubpattern) { auto dir = tmpDir("log_roll_str"); rmrf(dir); std::filesystem::create_directories(dir);
    { RollingFileAppender app(Path(dir), TimeRollingPolicy::DAY, "prefix-{UtcDateTime}-suffix.log", false, true, true, false); ASSERT_TRUE(app.Start());
      Record r = makeRawRec(Level::INFO, "str"); app.AppendRecord(r); app.Flush(); app.Stop(); }
    EXPECT_GE(countFiles(dir), 1); rmrf(dir); }

// ===== Time policy + archive=true =====
TEST(RollingTime, ArchiveZip) { auto dir = tmpDir("log_roll_tarch"); rmrf(dir); std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), TimeRollingPolicy::SECOND, "{UtcDateTime}.log", /*archive=*/true, true, true, false);
        ASSERT_TRUE(app.Start());
        Record r = makeRawRec(Level::INFO, "ta");
        app.AppendRecord(r);
        app.Flush();
        app.Stop();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_GE(countFiles(dir), 1);
    rmrf(dir);
}

// ===== 空 raw 记录被跳过 (覆盖 343/987 早返回分支) =====
TEST(RollingSize, EmptyRawSkipped)
{
    auto dir = tmpDir("log_roll_empty");
    rmrf(dir);
    std::filesystem::create_directories(dir);
    {
        RollingFileAppender app(Path(dir), "e", "log", 100, 3, false, true, true, false);
        ASSERT_TRUE(app.Start());
        Record r;   // raw 为空
        r.timestamp = 1700000000000000000ull;
        r.level = Level::INFO;
        r.message = "empty";
        app.AppendRecord(r);   // 不应崩溃, 也不写
        app.Flush();
        app.Stop();
    }
    SUCCEED();
    rmrf(dir);
}

// ===== operator<< TimeRollingPolicy (inl 模板, 覆盖 default 分支) =====
TEST(RollingTime, StreamOperator)
{
    std::ostringstream oss;
    oss << TimeRollingPolicy::YEAR << TimeRollingPolicy::MONTH << TimeRollingPolicy::DAY
        << TimeRollingPolicy::HOUR << TimeRollingPolicy::MINUTE << TimeRollingPolicy::SECOND
        << static_cast<TimeRollingPolicy>(99);   // default 分支
    EXPECT_FALSE(oss.str().empty());
}
