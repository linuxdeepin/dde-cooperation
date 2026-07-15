// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "system/shared_memory.h"

#include <cstring>
#include <string>
#include <unistd.h>

using namespace BaseKit;

namespace {
std::string UniqueName()
{
    static int counter = 0;
    ++counter;
    return "bk_shm_" + std::to_string((long)getpid()) + "_" + std::to_string(counter);
}
}

TEST(SharedMemoryTest, CreateAsOwner)
{
    std::string name = UniqueName();
    SharedMemory shm(name, 4096);
    EXPECT_TRUE(bool(shm));
    EXPECT_EQ(shm.name(), name);
    EXPECT_EQ(shm.size(), 4096u);
    EXPECT_TRUE(shm.owner());
    EXPECT_NE(shm.ptr(), nullptr);
}

TEST(SharedMemoryTest, ConstPtrAccessible)
{
    std::string name = UniqueName();
    const SharedMemory shm(name, 128);
    EXPECT_NE(shm.ptr(), nullptr);
}

TEST(SharedMemoryTest, WriteAndReadThroughMapping)
{
    std::string name = UniqueName();
    SharedMemory shm(name, 256);
    ASSERT_NE(shm.ptr(), nullptr);

    char* p = static_cast<char*>(shm.ptr());
    std::memcpy(p, "hello shared", 12);

    SharedMemory reader(name, 256);
    EXPECT_FALSE(reader.owner());
    const char* cp = static_cast<const char*>(reader.ptr());
    EXPECT_EQ(std::string(cp, 12), "hello shared");
}

TEST(SharedMemoryTest, SecondOpenerIsNotOwner)
{
    std::string name = UniqueName();
    SharedMemory owner(name, 1024);
    EXPECT_TRUE(owner.owner());

    {
        SharedMemory opener(name, 1024);
        EXPECT_TRUE(bool(opener));
        EXPECT_FALSE(opener.owner());
        EXPECT_EQ(opener.size(), 1024u);
    }

    EXPECT_TRUE(owner.owner());
}

TEST(SharedMemoryTest, SizeReportedConsistently)
{
    std::string name = UniqueName();
    SharedMemory a(name, 333);
    EXPECT_EQ(a.size(), 333u);

    SharedMemory b(name, 333);
    EXPECT_EQ(b.size(), 333u);
}
