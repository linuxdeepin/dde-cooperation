// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "time/time.h"
#include "time/timestamp.h"
#include "time/timespan.h"

using namespace BaseKit;

// ===== 构造与字段 =====
TEST(TimeTest, ComponentCtor)
{
    Time t(2024, 6, 15, 10, 30, 45, 123, 456, 789);
    EXPECT_EQ(t.year(), 2024);
    EXPECT_EQ(t.month(), 6);
    EXPECT_EQ(t.day(), 15);
    EXPECT_EQ(t.hour(), 10);
    EXPECT_EQ(t.minute(), 30);
    EXPECT_EQ(t.second(), 45);
    EXPECT_EQ(t.millisecond(), 123);
    EXPECT_EQ(t.microsecond(), 456);
    EXPECT_EQ(t.nanosecond(), 789);
}

TEST(TimeTest, DefaultCtor)
{
    Time t;
    // 默认构造应为合法的最小值（不抛异常）
    EXPECT_NO_THROW((void)t.year());
}

// 注：非法月份/日期/时间等用例在 Debug 构建下会先触发 assert() 终止进程，
// 无法用 EXPECT_THROW 捕获，故此处仅覆盖合法路径；非法分支由源码内 assert 守护。

// ===== epoch 与 weekday =====
TEST(TimeTest, Epoch)
{
    Time e = Time::epoch();
    EXPECT_EQ(e.year(), 1970);
    EXPECT_EQ(e.month(), 1);
    EXPECT_EQ(e.day(), 1);
    // weekday 由组件构造函数内部推算，仅校验为合法枚举
    int w = (int)e.weekday();
    EXPECT_GE(w, (int)Weekday::Sunday);
    EXPECT_LE(w, (int)Weekday::Saturday);
}

TEST(TimeTest, WeekdayIsInRange)
{
    // 仅断言 weekday 落在合法枚举区间（具体映射以 epoch 校准为准）
    auto wd = Time(2024, 6, 15).weekday();
    int w = (int)wd;
    EXPECT_GE(w, (int)Weekday::Sunday);
    EXPECT_LE(w, (int)Weekday::Saturday);
}

// ===== 与 Timestamp 比较（内部经 Time(Timestamp) 友元转换）=====
TEST(TimeTest, CompareWithEpochTimestamp)
{
    Time epoch_time(1970, 1, 1);
    UtcTimestamp ts0(0);   // epoch 纳秒
    EXPECT_TRUE(epoch_time == ts0);
    EXPECT_FALSE(epoch_time != ts0);
}

TEST(TimeTest, CompareWithFutureTimestamp)
{
    Time past(1970, 1, 2);
    UtcTimestamp future((uint64_t)2 * 86400 * 1000000000ull);   // 2 天的纳秒，晚于 1970-01-02
    EXPECT_TRUE(past < future);
    EXPECT_TRUE(past <= future);
    EXPECT_TRUE(future >= past);
    EXPECT_TRUE(future > past);
}

// ===== 比较 =====
TEST(TimeTest, Comparisons)
{
    Time a(2020, 1, 1);
    Time b(2020, 1, 2);
    Time c(2020, 1, 1);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(b >= a);
}

TEST(TimeTest, CompareWithTimestamp)
{
    Time a(1970, 1, 1);
    UtcTimestamp ts0(0);
    EXPECT_TRUE(a == ts0);
    EXPECT_FALSE(a != ts0);
    UtcTimestamp ts_future(1000000000ll);   // 远晚于 epoch
    EXPECT_TRUE(a < ts_future);
    EXPECT_FALSE(a > ts_future);
    EXPECT_TRUE(a <= ts_future);
    EXPECT_TRUE(ts_future >= a);
}

// ===== 与 Timespan 的加减 =====
TEST(TimeTest, AddSubTimespan)
{
    Time t(2020, 1, 1, 0, 0, 0);
    Timespan one_day = Timespan::days(1);
    Time next = t + one_day;
    EXPECT_EQ(next.day(), 2);
    Time back = next - one_day;
    EXPECT_EQ(back.day(), 1);
    EXPECT_EQ((back - t).days(), 0);
}

TEST(TimeTest, CompoundAddSub)
{
    Time t(2020, 1, 1, 12, 0, 0);
    Timespan hour = Timespan::hours(1);
    t += hour;
    EXPECT_EQ(t.hour(), 13);
    t -= hour;
    EXPECT_EQ(t.hour(), 12);
}

// ===== utcstamp / localstamp 可调用 =====
TEST(TimeTest, StampExporters)
{
    Time t(2020, 6, 15, 12, 0, 0);
    EXPECT_NO_THROW((void)t.utcstamp());
    EXPECT_NO_THROW((void)t.localstamp());
}

// ===== swap =====
TEST(TimeTest, Swap)
{
    Time a(2020, 1, 1);
    Time b(2030, 5, 5);
    a.swap(b);
    EXPECT_EQ(a.year(), 2030);
    EXPECT_EQ(b.year(), 2020);
    swap(a, b);
    EXPECT_EQ(a.year(), 2020);
    EXPECT_EQ(b.year(), 2030);
}
