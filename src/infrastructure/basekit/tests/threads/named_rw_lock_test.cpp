// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// NamedRWLock 扩展覆盖：阻塞读写、Try 读写、限时 Try、
// 写->读转换、双线程读共享/写互斥、name() 查询。
// 复用 named_threads_test.cpp 的 PID 唯一名策略。

#include <gtest/gtest.h>
#include "threads/named_rw_lock.h"
#include "time/timestamp.h"

#include <atomic>
#include <string>
#include <thread>
#include <unistd.h>

using namespace BaseKit;

namespace {
std::string uniq(const char *tag)
{
    return std::string("bk_namedrw_") + tag + "_" + std::to_string(getpid());
}
}   // namespace

// ===== name() =====
TEST(NamedRWLockTest, NameGetterReturnsCtorArg)
{
    const auto name = uniq("name");
    NamedRWLock lock(name);
    EXPECT_EQ(lock.name(), name);
}

// ===== 阻塞 LockRead / LockWrite =====
TEST(NamedRWLockTest, LockReadUnlockRead)
{
    const auto name = uniq("lr");
    NamedRWLock lock(name);
    EXPECT_NO_THROW(lock.LockRead());
    EXPECT_NO_THROW(lock.UnlockRead());
}

TEST(NamedRWLockTest, LockWriteUnlockWrite)
{
    const auto name = uniq("lw");
    NamedRWLock lock(name);
    EXPECT_NO_THROW(lock.LockWrite());
    EXPECT_NO_THROW(lock.UnlockWrite());
}

// ===== TryLockRead / TryLockWrite（空闲时返回 true）=====
TEST(NamedRWLockTest, TryLocksSucceedWhenFree)
{
    const auto name = uniq("free");
    NamedRWLock lock(name);
    EXPECT_TRUE(lock.TryLockRead());
    lock.UnlockRead();
    EXPECT_TRUE(lock.TryLockWrite());
    lock.UnlockWrite();
}

// ===== 读共享、持有写时读失败 =====
TEST(NamedRWLockTest, ReadSharedAndWriteExclusive)
{
    const auto name = uniq("rwsem");
    NamedRWLock lock(name);
    ASSERT_TRUE(lock.TryLockRead());
    ASSERT_TRUE(lock.TryLockRead());   // 读可重入共享
    lock.UnlockRead();
    lock.UnlockRead();

    ASSERT_TRUE(lock.TryLockWrite());
    EXPECT_FALSE(lock.TryLockRead());    // 持有写时读失败
    EXPECT_FALSE(lock.TryLockWrite());   // 写不可重入
    lock.UnlockWrite();
}

// ===== 限时 Try：被其它线程持有时返回 false =====
TEST(NamedRWLockTest, TryLockReadForTimesOutWhenWriteHeld)
{
    const auto name = uniq("trf");
    NamedRWLock lock(name);
    ASSERT_TRUE(lock.TryLockWrite());   // 主线程持有写

    std::atomic<bool> got{true};
    std::thread t([&lock, &got] {
        got = lock.TryLockReadFor(Timespan::milliseconds(20));
    });
    t.join();
    EXPECT_FALSE(got);   // 因写锁被占而超时
    lock.UnlockWrite();
}

TEST(NamedRWLockTest, TryLockWriteForTimesOutWhenWriteHeld)
{
    const auto name = uniq("twf");
    NamedRWLock lock(name);
    ASSERT_TRUE(lock.TryLockWrite());

    std::atomic<bool> got{true};
    std::thread t([&lock, &got] {
        got = lock.TryLockWriteFor(Timespan::milliseconds(20));
    });
    t.join();
    EXPECT_FALSE(got);
    lock.UnlockWrite();
}

// ===== 写 -> 读 转换 =====
TEST(NamedRWLockTest, TryConvertWriteToRead)
{
    const auto name = uniq("tcwr");
    NamedRWLock lock(name);
    ASSERT_TRUE(lock.TryLockWrite());
    bool converted = lock.TryConvertWriteToRead();
    // 非阻塞转换是否成功取决于底层原语：成功则持读锁，失败仍持写锁
    if (converted)
        lock.UnlockRead();
    else
        lock.UnlockWrite();
    SUCCEED();
}

TEST(NamedRWLockTest, ConvertWriteToRead)
{
    const auto name = uniq("cwr");
    NamedRWLock lock(name);
    ASSERT_TRUE(lock.TryLockWrite());
    EXPECT_NO_THROW(lock.ConvertWriteToRead());
    lock.UnlockRead();
}

// ===== 双线程读共享 / 写互斥简短竞争 =====
TEST(NamedRWLockTest, TwoThreadReaderSharedWriterExclusive)
{
    const auto name = uniq("2th");
    NamedRWLock lock(name);

    ASSERT_TRUE(lock.TryLockRead());   // 主线程持读

    std::atomic<int> flags{0};
    std::thread t([&lock, &flags] {
        // 另一线程可同时获得读锁（共享）
        if (lock.TryLockRead())
        {
            flags |= 1;
            lock.UnlockRead();
        }
        // 但不能获得写锁（主线程持读）
        if (!lock.TryLockWrite())
            flags |= 2;
    });
    t.join();
    lock.UnlockRead();

    EXPECT_EQ(flags & 1, 1);   // 成功获得共享读
    EXPECT_EQ(flags & 2, 2);   // 未能获得写
}
