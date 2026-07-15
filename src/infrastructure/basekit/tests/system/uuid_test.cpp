// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "system/uuid.h"

#include <array>
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>

using namespace BaseKit;

TEST(UUIDTest, DefaultConstructorIsNil)
{
    UUID uuid;
    EXPECT_EQ(uuid.data().size(), 16u);
    for (uint8_t b : uuid.data())
        EXPECT_EQ(b, 0);
    EXPECT_FALSE(bool(uuid));
    EXPECT_EQ(uuid.string(), "00000000-0000-0000-0000-000000000000");
}

TEST(UUIDTest, NilFactoryEqualsDefault)
{
    UUID nil = UUID::Nil();
    EXPECT_FALSE(bool(nil));
    EXPECT_EQ(nil, UUID());
}

TEST(UUIDTest, StringConstructorParsesCanonical)
{
    UUID uuid("123e4567-e89b-12d3-a456-426655440000");
    EXPECT_TRUE(bool(uuid));
    EXPECT_EQ(uuid.string(), "123e4567-e89b-12d3-a456-426655440000");
    EXPECT_EQ(uuid.data()[0], 0x12);
    EXPECT_EQ(uuid.data()[1], 0x3e);
    EXPECT_EQ(uuid.data()[15], 0x00);
}

TEST(UUIDTest, StringConstructorAcceptsBracesAndUppercase)
{
    UUID braced(std::string("{ABCD1234-ABCD-ABCD-ABCD-AABBCCDDEEFF}"));
    EXPECT_EQ(braced.string(), "abcd1234-abcd-abcd-abcd-aabbccddeeff");

    UUID upper("ABCDEF12-3456-7890-ABCD-EF1234567890");
    EXPECT_EQ(upper.string(), "abcdef12-3456-7890-abcd-ef1234567890");
}

TEST(UUIDTest, StringLiteralConstructor)
{
    UUID lit("11223344-5566-7788-99aa-bbccddeeff00");
    EXPECT_EQ(lit.data()[0], 0x11);
    EXPECT_EQ(lit.data()[15], 0x00);
}

TEST(UUIDTest, UserDefinedLiteralSuffix)
{
    auto uuid = "00112233-4455-6677-8899-aabbccddeeff"_uuid;
    EXPECT_TRUE(bool(uuid));
    EXPECT_EQ(uuid.data()[0], 0x00);
    EXPECT_EQ(uuid.data()[2], 0x22);
}

TEST(UUIDTest, DataBufferAccessorIsMutable)
{
    UUID uuid;
    auto& buf = uuid.data();
    buf[0] = 0xFF;
    EXPECT_EQ(uuid.data()[0], 0xFF);
}

TEST(UUIDTest, ArrayConstructor)
{
    std::array<uint8_t, 16> raw = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    UUID uuid(raw);
    EXPECT_EQ(uuid.data(), raw);
}

TEST(UUIDTest, StringFormatHasCorrectLengthAndDashes)
{
    UUID uuid = UUID::Random();
    std::string s = uuid.string();
    ASSERT_EQ(s.size(), 36u);
    EXPECT_EQ(s[8], '-');
    EXPECT_EQ(s[13], '-');
    EXPECT_EQ(s[18], '-');
    EXPECT_EQ(s[23], '-');
}

TEST(UUIDTest, SequentialFactoryIsUniqueAndNonNil)
{
    UUID a = UUID::Sequential();
    UUID b = UUID::Sequential();
    EXPECT_TRUE(bool(a));
    EXPECT_TRUE(bool(b));
    EXPECT_NE(a, b);
}

TEST(UUIDTest, RandomFactoryIsUniqueAndNonNil)
{
    UUID a = UUID::Random();
    UUID b = UUID::Random();
    EXPECT_TRUE(bool(a));
    EXPECT_NE(a, b);
}

TEST(UUIDTest, SecureFactoryProducesNonNil)
{
    UUID a = UUID::Secure();
    UUID b = UUID::Secure();
    EXPECT_TRUE(bool(a));
    EXPECT_TRUE(bool(b));
    EXPECT_NE(a, b);
}

TEST(UUIDTest, ComparisonOperators)
{
    UUID smaller("00000000-0000-0000-0000-000000000001");
    UUID same("00000000-0000-0000-0000-000000000001");
    UUID larger("00000000-0000-0000-0000-000000000002");

    EXPECT_TRUE(smaller == same);
    EXPECT_TRUE(smaller != larger);
    EXPECT_TRUE(smaller < larger);
    EXPECT_TRUE(larger > smaller);
    EXPECT_TRUE(smaller <= same);
    EXPECT_TRUE(same >= smaller);
}

TEST(UUIDTest, AssignmentFromStdString)
{
    UUID uuid;
    uuid = std::string("123e4567-e89b-12d3-a456-426655440000");
    EXPECT_EQ(uuid.string(), "123e4567-e89b-12d3-a456-426655440000");
}

TEST(UUIDTest, AssignmentFromArray)
{
    UUID uuid("123e4567-e89b-12d3-a456-426655440000");
    std::array<uint8_t, 16> raw = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    uuid = raw;
    EXPECT_EQ(uuid.data(), raw);
    EXPECT_FALSE(bool(uuid));
}

TEST(UUIDTest, OutputStreamOperator)
{
    UUID uuid("123e4567-e89b-12d3-a456-426655440000");
    std::ostringstream oss;
    oss << uuid;
    EXPECT_EQ(oss.str(), "123e4567-e89b-12d3-a456-426655440000");
}

TEST(UUIDTest, MemberAndFreeSwap)
{
    UUID a("123e4567-e89b-12d3-a456-426655440000");
    UUID b("00112233-4455-6677-8899-aabbccddeeff");
    UUID ca = a, cb = b;

    a.swap(b);
    EXPECT_EQ(a, cb);
    EXPECT_EQ(b, ca);

    swap(a, b);
    EXPECT_EQ(a, ca);
    EXPECT_EQ(b, cb);
}

TEST(UUIDTest, CopyAndMoveSemantics)
{
    UUID orig("123e4567-e89b-12d3-a456-426655440000");

    UUID copied(orig);
    EXPECT_EQ(copied, orig);

    UUID moved(std::move(copied));
    EXPECT_EQ(moved, orig);

    UUID assigned;
    assigned = orig;
    EXPECT_EQ(assigned, orig);

    UUID moveAssigned;
    moveAssigned = std::move(assigned);
    EXPECT_EQ(moveAssigned, orig);
}

TEST(UUIDTest, StdHashIsUsable)
{
    UUID a("123e4567-e89b-12d3-a456-426655440000");
    UUID b("123e4567-e89b-12d3-a456-426655440000");
    UUID c("00112233-4455-6677-8899-aabbccddeeff");

    std::hash<UUID> hasher;
    EXPECT_EQ(hasher(a), hasher(b));
    EXPECT_NE(hasher(a), hasher(c));
}
