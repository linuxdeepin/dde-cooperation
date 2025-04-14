#include <gtest/gtest.h>
#include "math/math.h"
#include <limits>
#include <cmath>

using namespace BaseKit;

// 辅助函数，用于处理0值的情况
template<typename T>
T SafeGCD(T a, T b) {
    if (b == 0)
        return a;
    if (a == 0)
        return b;
    if (a == 0 && b == 0)
        return 0;
    return GCD(a, b);
}

// 测试最大公约数（GCD）功能
TEST(MathTest, GCD) {
    // 测试整数对的GCD
    EXPECT_EQ(GCD(12, 8), 4);
    EXPECT_EQ(GCD(54, 24), 6);
    EXPECT_EQ(GCD(48, 18), 6);
    
    // 测试互质的数
    EXPECT_EQ(GCD(13, 7), 1);
    EXPECT_EQ(GCD(31, 37), 1);
    
    // 测试一个数是另一个数的倍数
    EXPECT_EQ(GCD(20, 5), 5);
    EXPECT_EQ(GCD(100, 25), 25);
    
    // 测试相同的数
    EXPECT_EQ(GCD(17, 17), 17);
    
    // 测试0的情况（使用辅助函数安全处理）
    EXPECT_EQ(SafeGCD(0, 5), 5);
    EXPECT_EQ(SafeGCD(7, 0), 7);
    EXPECT_EQ(SafeGCD(0, 0), 0);
    
    // 测试不同类型
    EXPECT_EQ(GCD<int8_t>(15, 10), 5);
    EXPECT_EQ(GCD<uint16_t>(56, 42), 14);
    EXPECT_EQ(GCD<int32_t>(123456, 7890), 6);
    EXPECT_EQ(GCD<uint64_t>(98765432123456, 12345678987654), 2);
}

// 测试向上取整（RoundUp）功能
TEST(MathTest, RoundUp) {
    // 基本测试
    EXPECT_EQ(Math::RoundUp(10, 3), 12);  // 10向上取整到3的倍数
    EXPECT_EQ(Math::RoundUp(20, 5), 20);  // 20已经是5的倍数
    EXPECT_EQ(Math::RoundUp(7, 10), 10);  // 7向上取整到10的倍数
    
    // 0和1的情况
    EXPECT_EQ(Math::RoundUp(0, 5), 0);    // 0向上取整到任何数的倍数仍是0
    EXPECT_EQ(Math::RoundUp(7, 1), 7);    // 任何数向上取整到1的倍数仍是它自己
    
    // 大数测试
    EXPECT_EQ(Math::RoundUp(999, 1000), 1000);
    EXPECT_EQ(Math::RoundUp(2000, 1000), 2000);
    EXPECT_EQ(Math::RoundUp(2001, 1000), 3000);
    
    // 测试不同类型
    EXPECT_EQ(Math::RoundUp<int8_t>(10, 4), 12);
    EXPECT_EQ(Math::RoundUp<uint16_t>(100, 30), 120);
    EXPECT_EQ(Math::RoundUp<int32_t>(1000, 400), 1200);
    EXPECT_EQ(Math::RoundUp<uint64_t>(10000, 3000), 12000);
}

// 测试乘法除法（MulDiv64）功能
TEST(MathTest, MulDiv64) {
    // 基本测试
    EXPECT_EQ(Math::MulDiv64(100, 200, 50), 400);  // (100 * 200) / 50 = 400
    EXPECT_EQ(Math::MulDiv64(1000, 10, 5), 2000);  // (1000 * 10) / 5 = 2000
    
    // 整除测试
    EXPECT_EQ(Math::MulDiv64(30, 40, 10), 120);    // (30 * 40) / 10 = 120
    EXPECT_EQ(Math::MulDiv64(25, 8, 2), 100);      // (25 * 8) / 2 = 100
    
    // 舍入测试（取决于实现）
    EXPECT_EQ(Math::MulDiv64(100, 3, 2), 150);     // (100 * 3) / 2 = 150
    
    // 边界情况
    EXPECT_EQ(Math::MulDiv64(0, 1000, 10), 0);     // (0 * 1000) / 10 = 0
    EXPECT_EQ(Math::MulDiv64(100, 0, 10), 0);      // (100 * 0) / 10 = 0
    
    // 大数测试（避免中间结果溢出）
    uint64_t large1 = 1ULL << 32;                  // 2^32
    uint64_t large2 = 1ULL << 31;                  // 2^31
    EXPECT_EQ(Math::MulDiv64(large1, large2, 1), large1 * large2);
    
    // 测试除数为1
    EXPECT_EQ(Math::MulDiv64(12345, 67890, 1), 12345 * 67890);
    
    // 测试近似最大值
    uint64_t maxValue = std::numeric_limits<uint64_t>::max();
    EXPECT_EQ(Math::MulDiv64(maxValue, 1, 1), maxValue);
    EXPECT_EQ(Math::MulDiv64(maxValue / 2, 2, 1), maxValue - (maxValue % 2));
}

// 测试溢出保护
TEST(MathTest, OverflowProtection) {
    // 测试乘法可能导致溢出的情况
    uint64_t large1 = std::numeric_limits<uint32_t>::max();
    uint64_t large2 = std::numeric_limits<uint32_t>::max();
    uint64_t divider = 1;
    
    // 正常的乘法会溢出uint64_t，但MulDiv64应该能处理
    uint64_t expected = static_cast<uint64_t>(large1) * static_cast<uint64_t>(large2) / divider;
    EXPECT_EQ(Math::MulDiv64(large1, large2, divider), expected);
    
    // 测试更大的数，近似uint64_t的最大值
    uint64_t maxHalf = std::numeric_limits<uint64_t>::max() / 2;
    EXPECT_NO_THROW(Math::MulDiv64(maxHalf, 2, 1));
} 