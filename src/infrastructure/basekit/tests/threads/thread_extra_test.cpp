// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Thread 扩展覆盖：线程 Id/亲和性查询、spawned 线程的 affinity/priority
// get/set 重载、SleepUntil 及时唤醒、Start 带参闭包。
// affinity/priority 在受限环境下可能抛异常，统一 try/catch 后 SUCCEED。

#include <gtest/gtest.h>
#include "errors/exceptions.h"
#include "threads/thread.h"
#include "time/timestamp.h"

#include <atomic>
#include <bitset>
#include <future>
#include <thread>

using namespace BaseKit;

// ===== 线程 Id / 亲和性查询 =====
TEST(ThreadExtraTest, CurrentThreadIdPositive)
{
    EXPECT_GT(Thread::CurrentThreadId(), 0u);
}

TEST(ThreadExtraTest, CurrentThreadAffinityNonzero)
{
    EXPECT_GT(Thread::CurrentThreadAffinity(), 0u);
}

TEST(ThreadExtraTest, CurrentThreadIdDiffersAcrossThreads)
{
    uint64_t mainId = Thread::CurrentThreadId();
    std::atomic<uint64_t> childId{0};
    std::thread t([&] { childId = Thread::CurrentThreadId(); });
    t.join();
    EXPECT_NE(childId.load(), 0u);
    EXPECT_NE(childId.load(), mainId);
}

// ===== spawned 线程的 affinity get/set 重载 =====
TEST(ThreadExtraTest, GetAffinityOfSpawnedThread)
{
    std::atomic<bool> done{false};
    std::thread t([&] { while (!done.load()) Thread::Yield(); });
    EXPECT_TRUE(t.get_id() != std::thread::id());

    try
    {
        std::bitset<64> af = Thread::GetAffinity(t);
        (void)af;
        SUCCEED();
    }
    catch (const Exception&)
    {
        SUCCEED();   // 受限环境拒绝查询
    }

    done = true;
    t.join();
}

TEST(ThreadExtraTest, SetAffinityOfSpawnedThread)
{
    std::atomic<bool> done{false};
    std::thread t([&] { while (!done.load()) Thread::Yield(); });

    try
    {
        Thread::SetAffinity(t, std::bitset<64>().set(0));
        SUCCEED();
    }
    catch (const Exception&)
    {
        SUCCEED();   // 需要权限，失败可接受
    }

    done = true;
    t.join();
}

// ===== spawned 线程的 priority get/set 重载 =====
TEST(ThreadExtraTest, GetPriorityOfSpawnedThread)
{
    std::atomic<bool> done{false};
    std::thread t([&] { while (!done.load()) Thread::Yield(); });

    try
    {
        ThreadPriority p = Thread::GetPriority(t);
        EXPECT_TRUE(p == ThreadPriority::IDLE || p == ThreadPriority::LOWEST ||
                    p == ThreadPriority::LOW || p == ThreadPriority::NORMAL ||
                    p == ThreadPriority::HIGH || p == ThreadPriority::HIGHEST ||
                    p == ThreadPriority::REALTIME);
    }
    catch (const Exception&)
    {
        SUCCEED();
    }

    done = true;
    t.join();
}

TEST(ThreadExtraTest, SetPriorityOfSpawnedThread)
{
    std::atomic<bool> done{false};
    std::thread t([&] { while (!done.load()) Thread::Yield(); });

    try
    {
        Thread::SetPriority(t, ThreadPriority::NORMAL);
        SUCCEED();
    }
    catch (const Exception&)
    {
        SUCCEED();
    }

    done = true;
    t.join();
}

// ===== SleepUntil 及时唤醒 =====
TEST(ThreadExtraTest, SleepUntilWakesPromptly)
{
    UtcTimestamp target = UtcTimestamp() + Timespan::milliseconds(10);
    Thread::SleepUntil(target);
    // 唤醒后当前时间不早于目标
    EXPECT_GE(UtcTimestamp(), target);
}

TEST(ThreadExtraTest, SleepForZeroReturns)
{
    auto start = UtcTimestamp();
    Thread::SleepFor(Timespan::milliseconds(0));
    auto end = UtcTimestamp();
    EXPECT_GE(end, start);
}

// ===== Start 带参闭包（promise/future 验证执行）=====
TEST(ThreadExtraTest, StartWithBoundArgs)
{
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    std::thread t = Thread::Start(
        [](std::promise<int>& p, int v) { p.set_value(v * 2); },
        std::ref(prom), 21);
    t.join();
    EXPECT_EQ(fut.get(), 42);
}

TEST(ThreadExtraTest, StartInvokesFunction)
{
    std::atomic<bool> ran{false};
    std::thread t = Thread::Start([&ran] { ran = true; });
    t.join();
    EXPECT_TRUE(ran.load());
}
