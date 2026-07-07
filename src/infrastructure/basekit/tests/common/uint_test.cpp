// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "common/uint128.h"
#include "common/uint256.h"

#include <string>

using namespace BaseKit;

// ===== uint128 构造与基本访问 =====
TEST(UInt128Test, DefaultCtorIsZero)
{
    uint128_t v;
    EXPECT_EQ(v.upper(), 0u);
    EXPECT_EQ(v.lower(), 0u);
    EXPECT_FALSE((bool)v);
}

TEST(UInt128Test, ConstructFromIntegerTypes)
{
    EXPECT_EQ(uint128_t((uint8_t)0xAB).lower(), 0xABull);
    EXPECT_EQ(uint128_t((uint16_t)0xABCD).lower(), 0xABCDull);
    EXPECT_EQ(uint128_t((uint32_t)0xDEADBEEFu).lower(), 0xDEADBEEFull);
    EXPECT_EQ(uint128_t((uint64_t)0x123456789ABCDEF0ull).lower(), 0x123456789ABCDEF0ull);
}

TEST(UInt128Test, ConstructFromSignedKeepsValue)
{
    // 有符号负数：符号扩展填充 lower 的 64 位（upper 保持 0）
    EXPECT_EQ(uint128_t((int8_t)-1).lower(), 0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(uint128_t((int8_t)-1).upper(), 0u);
    EXPECT_EQ(uint128_t((int32_t)-1).lower(), 0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(uint128_t((int32_t)-1).upper(), 0u);
    EXPECT_EQ(uint128_t((int64_t)-1).lower(), 0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(uint128_t((int64_t)-1).upper(), 0u);
}

TEST(UInt128Test, UpperLowerConstruction)
{
    uint128_t v(0x0001020304050607ull, 0x08090A0B0C0D0E0Full);
    EXPECT_EQ(v.upper(), 0x0001020304050607ull);
    EXPECT_EQ(v.lower(), 0x08090A0B0C0D0E0Full);
    EXPECT_TRUE((bool)v);
}

// ===== 算术运算 =====
TEST(UInt128Test, Addition)
{
    uint128_t a((uint64_t)1ull << 40);
    uint128_t b((uint64_t)1ull << 40);
    EXPECT_EQ((a + b).lower(), (uint64_t)1ull << 41);

    // 进位：lower 溢出到 upper
    uint128_t max_low(0, 0xFFFFFFFFFFFFFFFFull);
    uint128_t one(1ull);
    uint128_t r = max_low + one;
    EXPECT_EQ(r.upper(), 1u);
    EXPECT_EQ(r.lower(), 0u);
}

TEST(UInt128Test, Subtraction)
{
    uint128_t a(10);
    uint128_t b(3);
    EXPECT_EQ((a - b).lower(), 7u);

    // 借位
    uint128_t c(0, 0);
    uint128_t d(0, 1);
    uint128_t r = c - d;
    EXPECT_EQ(r.upper(), 0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(r.lower(), 0xFFFFFFFFFFFFFFFFull);
}

TEST(UInt128Test, MultiplicationSmall)
{
    EXPECT_EQ((uint128_t(6) * uint128_t(7)).lower(), 42u);
    EXPECT_EQ((uint128_t(0) * uint128_t(12345)).lower(), 0u);
}

TEST(UInt128Test, MultiplicationLargeProducesUpper)
{
    uint128_t big((uint64_t)1ull << 40);
    uint128_t r = big * big;
    // 2^80 → upper 含 (1<<16)
    EXPECT_EQ(r.upper(), (uint64_t)1ull << 16);
    EXPECT_EQ(r.lower(), 0u);
}

TEST(UInt128Test, DivisionAndModulo)
{
    uint128_t a(1000);
    uint128_t b(7);
    EXPECT_EQ((a / b).lower(), 142u);
    EXPECT_EQ((a % b).lower(), 6u);
}

TEST(UInt128Test, DivisionByOneAndEqual)
{
    uint128_t a(42);
    EXPECT_EQ((a / uint128_t(1)).lower(), 42u);
    EXPECT_EQ((a % uint128_t(1)).lower(), 0u);
    EXPECT_EQ((a / a).lower(), 1u);
    EXPECT_EQ((a % a).lower(), 0u);
}

TEST(UInt128Test, DivisionByZeroThrows)
{
    uint128_t a(42);
    EXPECT_THROW(a / uint128_t(0), std::domain_error);
    EXPECT_THROW(a % uint128_t(0), std::domain_error);
}

TEST(UInt128Test, IncrementDecrement)
{
    uint128_t a(5);
    EXPECT_EQ((++a).lower(), 6u);
    EXPECT_EQ((a--).lower(), 6u);
    EXPECT_EQ(a.lower(), 5u);
    EXPECT_EQ((--a).lower(), 4u);
}

// ===== 位运算 =====
TEST(UInt128Test, ShiftLeft)
{
    uint128_t a(1);
    EXPECT_EQ((a << 64).upper(), 1u);
    EXPECT_EQ((a << 64).lower(), 0u);
    EXPECT_EQ((a << 128), uint128_t(0));
    EXPECT_EQ((a << 0), a);
}

TEST(UInt128Test, ShiftRight)
{
    uint128_t a(0, 0xFFFFFFFFFFFFFFFFull);
    auto r = a << 64;   // upper=0xFFFF..., lower=0
    EXPECT_EQ((r >> 64).lower(), r.upper());

    uint128_t b(0xFFFFFFFFFFFFFFFFull, 0);
    EXPECT_EQ((b >> 128), uint128_t(0));
}

TEST(UInt128Test, BitwiseOps)
{
    uint128_t a(0xF0F0F0F0F0F0F0F0ull);
    uint128_t b(0x0FF00FF00FF00FF0ull);
    EXPECT_EQ((a & b).lower(), 0x00F000F000F000F0ull);
    EXPECT_EQ((a | b).lower(), 0xFFF0FFF0FFF0FFF0ull);
    EXPECT_EQ((a ^ b).lower(), 0xFF00FF00FF00FF00ull);
    EXPECT_EQ((~uint128_t(0)).upper(), 0xFFFFFFFFFFFFFFFFull);
}

TEST(UInt128Test, BitsCount)
{
    EXPECT_EQ(uint128_t(0).bits(), 0u);
    EXPECT_EQ(uint128_t(1).bits(), 1u);
    EXPECT_EQ(uint128_t(255).bits(), 8u);
    EXPECT_EQ(uint128_t(0, (uint64_t)1ull << 63).bits(), 64u);
    EXPECT_EQ(uint128_t(1, 0).bits(), 65u);
}

// ===== 比较 =====
TEST(UInt128Test, Comparisons)
{
    uint128_t a(100), b(200), c(100);
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= c);
    EXPECT_TRUE(b >= a);
}

// ===== 字符串表示 =====
TEST(UInt128Test, StringDecimal)
{
    EXPECT_EQ(uint128_t(0).string(10), "0");
    EXPECT_EQ(uint128_t(42).string(10), "42");
    EXPECT_EQ(uint128_t(123456789).string(10), "123456789");
    // 大数
    uint128_t big((uint64_t)0xFFFFFFFFFFFFFFFFull);
    EXPECT_EQ(big.string(10), "18446744073709551615");
}

TEST(UInt128Test, StringHex)
{
    EXPECT_EQ(uint128_t(255).string(16), "ff");
    EXPECT_EQ(uint128_t(0xABCDEF).string(16), "abcdef");
    EXPECT_EQ(uint128_t(0).string(16), "0");
}

TEST(UInt128Test, StringPaddedLength)
{
    EXPECT_EQ(uint128_t(5).string(10, 4), "0005");
    EXPECT_EQ(uint128_t(0xF).string(16, 4), "000f");
}

TEST(UInt128Test, StringInvalidBaseThrows)
{
    EXPECT_THROW(uint128_t(5).string(1), std::invalid_argument);
    EXPECT_THROW(uint128_t(5).string(17), std::invalid_argument);
}

TEST(UInt128Test, WstringDecimal)
{
    EXPECT_EQ(uint128_t(42).wstring(10), std::wstring(L"42"));
    EXPECT_EQ(uint128_t(0).wstring(16), std::wstring(L"0"));
}

TEST(UInt128Test, DivmodLarge)
{
    // 2^100 / 2^50 = 2^50
    uint128_t power = uint128_t(1) << 100;
    uint128_t divisor = uint128_t(1) << 50;
    auto qr = uint128_t::divmod(power, divisor);
    EXPECT_EQ(qr.first, divisor);
    EXPECT_EQ(qr.second, uint128_t(0));
}

TEST(UInt128Test, BooleanConversionAndAssign)
{
    uint128_t v;
    EXPECT_FALSE((bool)v);
    v = 7;
    EXPECT_TRUE((bool)v);
    EXPECT_EQ(v.lower(), 7u);

    uint128_t a(3);
    a += uint128_t(4);
    EXPECT_EQ(a.lower(), 7u);
    a -= uint128_t(2);
    EXPECT_EQ(a.lower(), 5u);
    a *= uint128_t(3);
    EXPECT_EQ(a.lower(), 15u);
    a /= uint128_t(5);
    EXPECT_EQ(a.lower(), 3u);
}

TEST(UInt128Test, UnaryOperators)
{
    uint128_t a(5);
    EXPECT_EQ((+a).lower(), 5u);
    // -a == ~a + 1
    uint128_t neg = -a;
    EXPECT_EQ(neg, ~a + uint128_t(1));
}

TEST(UInt128Test, Swap)
{
    uint128_t a(11), b(22);
    a.swap(b);
    EXPECT_EQ(a.lower(), 22u);
    EXPECT_EQ(b.lower(), 11u);
    swap(a, b);
    EXPECT_EQ(a.lower(), 11u);
    EXPECT_EQ(b.lower(), 22u);
}

// ===== uint256 烟雾测试（同类 API）=====
TEST(UInt256Test, DefaultAndConstruct)
{
    uint256_t v;
    EXPECT_FALSE((bool)v);

    uint256_t from64((uint64_t)0x1122334455667788ull);
    EXPECT_TRUE((bool)from64);
}

TEST(UInt256Test, BasicArithmetic)
{
    uint256_t a((uint64_t)1000), b((uint64_t)7);
    auto sum = a + b;
    auto diff = a - b;
    auto prod = a * b;
    auto quo = a / b;
    auto mod = a % b;

    EXPECT_TRUE((bool)sum);
    EXPECT_TRUE((bool)diff);
    EXPECT_TRUE((bool)prod);
    EXPECT_TRUE((bool)quo);
    EXPECT_TRUE((bool)mod);
}

TEST(UInt256Test, StringDecimalSmall)
{
    uint256_t v((uint64_t)42);
    EXPECT_EQ(v.string(10), "42");
}

TEST(UInt256Test, DivisionByZeroThrows)
{
    uint256_t a(42);
    EXPECT_THROW(a / uint256_t(0), std::domain_error);
}
