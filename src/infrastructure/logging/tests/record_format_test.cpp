// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Record 的 StoreCustomFormat / StoreListFormat / RestoreFormat 全类型覆盖。

#include <catch2/catch_all.hpp>
#include <logging/record.h>

#include <cstdint>
#include <string>
#include <vector>

using namespace Logging;

TEST_CASE("Record StoreCustomFormat int and restore", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("val={}", 42);
    REQUIRE_FALSE(r.buffer.empty());
    std::string out = r.RestoreFormat();
    REQUIRE(out.find("val=42") != std::string::npos);
}

TEST_CASE("Record StoreCustomFormat string", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("name={}", std::string("alice"));
    REQUIRE(r.RestoreFormat().find("alice") != std::string::npos);
}

TEST_CASE("Record StoreCustomFormat bool and char", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("b={} c={}", true, 'X');
    std::string out = r.RestoreFormat();
    REQUIRE_FALSE(out.empty());
}

TEST_CASE("Record StoreCustomFormat float/double", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("x={}", 3.14);
    REQUIRE_FALSE(r.buffer.empty());
    REQUIRE_FALSE(r.RestoreFormat().empty());
}

TEST_CASE("Record StoreCustomFormat mixed numeric types", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("u8={} u16={} u32={} u64={}",
                        (uint8_t)1, (uint16_t)2, (uint32_t)3, (uint64_t)4);
    std::string out = r.RestoreFormat();
    REQUIRE(out.find("u8=1") != std::string::npos);
}

TEST_CASE("Record StoreCustomFormat signed types", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("i8={} i16={} i32={} i64={}",
                        (int8_t)-1, (int16_t)-2, (int32_t)-3, (int64_t)-4);
    std::string out = r.RestoreFormat();
    REQUIRE(out.find("i32=-3") != std::string::npos);
}

TEST_CASE("Record StoreListFormat basic", "[record][fmt]")
{
    Record r;
    r.StoreListFormat("{} + {} = {}", 1, 2, 3);
    std::string out = r.RestoreFormat();
    REQUIRE_FALSE(out.empty());
}

TEST_CASE("Record RestoreFormat static with buffer", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("a={}", 99);
    std::string out = Record::RestoreFormat("a={}", r.buffer, 0, r.buffer.size());
    REQUIRE(out.find("a=99") != std::string::npos);
}

TEST_CASE("Record multiple StoreCustomFormat accumulates", "[record][fmt]")
{
    Record r;
    r.StoreCustomFormat("first={}", 10);
    r.StoreCustomFormat(" second={}", 20);
    REQUIRE_FALSE(r.buffer.empty());
}
