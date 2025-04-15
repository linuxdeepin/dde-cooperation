#include <gtest/gtest.h>
#include "time/timestamp.h"
#include "time/timespan.h"
#include <thread>
#include <chrono>

using namespace BaseKit;

// 测试时间戳创建
TEST(TimestampTest, Creation) {
    // 测试默认构造
    Timestamp ts1;
    EXPECT_EQ(ts1.total(), Timestamp::epoch());
    
    // 测试显式构造
    Timestamp ts2(1000000000); // 1秒
    EXPECT_EQ(ts2.total(), 1000000000);
    
    // 测试从std::chrono时间点构造
    auto now = std::chrono::system_clock::now();
    Timestamp ts3(now);
    EXPECT_GT(ts3.total(), 0);
    
    // 测试静态工厂方法
    Timestamp ts4 = Timestamp::seconds(1);
    EXPECT_EQ(ts4.total(), 1000000000);
    
    Timestamp ts5 = Timestamp::milliseconds(1000);
    EXPECT_EQ(ts5.total(), 1000000000);
    
    Timestamp ts6 = Timestamp::microseconds(1000000);
    EXPECT_EQ(ts6.total(), 1000000000);
    
    Timestamp ts7 = Timestamp::nanoseconds(1000000000);
    EXPECT_EQ(ts7.total(), 1000000000);
    
    Timestamp ts8 = Timestamp::minutes(1);
    EXPECT_EQ(ts8.total(), 60 * 1000000000ull);
    
    Timestamp ts9 = Timestamp::hours(1);
    EXPECT_EQ(ts9.total(), 60 * 60 * 1000000000ull);
    
    Timestamp ts10 = Timestamp::days(1);
    EXPECT_EQ(ts10.total(), 24 * 60 * 60 * 1000000000ull);
}

// 测试时间戳操作
TEST(TimestampTest, Operations) {
    Timestamp ts1 = Timestamp::seconds(1);
    Timestamp ts2 = Timestamp::seconds(2);
    
    // 测试加法
    Timestamp ts3 = ts1 + 1000000000; // 加1秒
    EXPECT_EQ(ts3.total(), 2000000000);
    
    Timestamp ts4 = ts1 + Timespan::seconds(1);
    EXPECT_EQ(ts4.total(), 2000000000);
    
    // 测试减法
    Timestamp ts5 = ts2 - 1000000000; // 减1秒
    EXPECT_EQ(ts5.total(), 1000000000);
    
    Timestamp ts6 = ts2 - Timespan::seconds(1);
    EXPECT_EQ(ts6.total(), 1000000000);
    
    // 测试两个时间戳之间的时间跨度
    Timespan span = ts2 - ts1;
    EXPECT_EQ(span.total(), 1000000000);
    
    // 测试复合赋值操作
    Timestamp ts7 = Timestamp::seconds(1);
    ts7 += 1000000000;
    EXPECT_EQ(ts7.total(), 2000000000);
    
    Timestamp ts8 = Timestamp::seconds(1);
    ts8 += Timespan::seconds(1);
    EXPECT_EQ(ts8.total(), 2000000000);
    
    Timestamp ts9 = Timestamp::seconds(2);
    ts9 -= 1000000000;
    EXPECT_EQ(ts9.total(), 1000000000);
    
    Timestamp ts10 = Timestamp::seconds(2);
    ts10 -= Timespan::seconds(1);
    EXPECT_EQ(ts10.total(), 1000000000);
}

// 测试时间戳比较
TEST(TimestampTest, Comparison) {
    Timestamp ts1 = Timestamp::seconds(1);
    Timestamp ts2 = Timestamp::seconds(2);
    Timestamp ts3 = Timestamp::seconds(1);
    
    // 测试相等
    EXPECT_TRUE(ts1 == ts3);
    EXPECT_TRUE(ts1 == 1000000000);
    EXPECT_TRUE(1000000000 == ts1);
    
    // 测试不等
    EXPECT_TRUE(ts1 != ts2);
    EXPECT_TRUE(ts1 != 2000000000);
    EXPECT_TRUE(2000000000 != ts1);
    
    // 测试大于
    EXPECT_TRUE(ts2 > ts1);
    EXPECT_TRUE(ts2 > 1000000000);
    EXPECT_TRUE(2000000000 > ts1);
    
    // 测试小于
    EXPECT_TRUE(ts1 < ts2);
    EXPECT_TRUE(ts1 < 2000000000);
    EXPECT_TRUE(1000000000 < ts2);
    
    // 测试大于等于
    EXPECT_TRUE(ts1 >= ts3);
    EXPECT_TRUE(ts2 >= ts1);
    EXPECT_TRUE(ts1 >= 1000000000);
    EXPECT_TRUE(2000000000 >= ts1);
    
    // 测试小于等于
    EXPECT_TRUE(ts1 <= ts3);
    EXPECT_TRUE(ts1 <= ts2);
    EXPECT_TRUE(ts1 <= 1000000000);
    EXPECT_TRUE(1000000000 <= ts2);
}

// 测试时间戳转换
TEST(TimestampTest, Conversion) {
    Timestamp ts = Timestamp::seconds(1);
    
    // 测试各种时间单位转换
    EXPECT_EQ(ts.days(), 0);
    EXPECT_EQ(ts.hours(), 0);
    EXPECT_EQ(ts.minutes(), 0);
    EXPECT_EQ(ts.seconds(), 1);
    EXPECT_EQ(ts.milliseconds(), 1000);
    EXPECT_EQ(ts.microseconds(), 1000000);
    EXPECT_EQ(ts.nanoseconds(), 1000000000);
    
    // 测试转换为std::chrono时间点
    auto chrono_time = ts.chrono();
    auto duration = chrono_time.time_since_epoch();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    EXPECT_EQ(nanos, 1000000000);
}

// 测试静态时间戳获取函数
TEST(TimestampTest, StaticFunctions) {
    // 测试UTC时间戳
    uint64_t utc = Timestamp::utc();
    EXPECT_GT(utc, 0);
    
    // 测试本地时间戳
    uint64_t local = Timestamp::local();
    EXPECT_GT(local, 0);
    
    // 测试高精度时间戳
    uint64_t nano1 = Timestamp::nano();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t nano2 = Timestamp::nano();
    EXPECT_GT(nano2, nano1);
    
    // 测试RDTS计数器
    uint64_t rdts1 = Timestamp::rdts();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t rdts2 = Timestamp::rdts();
    EXPECT_GT(rdts2, rdts1);
}

// 测试特殊时间戳类
TEST(TimestampTest, SpecialTimestamps) {
    // 测试Epoch时间戳
    EpochTimestamp epoch;
    EXPECT_EQ(epoch.total(), 0);
    
    // 测试UTC时间戳
    UtcTimestamp utc;
    EXPECT_GT(utc.total(), 0);
    
    // 测试本地时间戳
    LocalTimestamp local;
    EXPECT_GT(local.total(), 0);
    
    // 测试高精度时间戳
    NanoTimestamp nano;
    EXPECT_GT(nano.total(), 0);
    
    // 测试RDTS时间戳
    RdtsTimestamp rdts;
    EXPECT_GT(rdts.total(), 0);
}

// 测试时间戳交换
TEST(TimestampTest, Swap) {
    Timestamp ts1 = Timestamp::seconds(1);
    Timestamp ts2 = Timestamp::seconds(2);
    
    // 测试成员交换
    ts1.swap(ts2);
    EXPECT_EQ(ts1.total(), 2000000000);
    EXPECT_EQ(ts2.total(), 1000000000);
    
    // 测试全局交换
    swap(ts1, ts2);
    EXPECT_EQ(ts1.total(), 1000000000);
    EXPECT_EQ(ts2.total(), 2000000000);
} 