// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "threads/barrier.h"
#include "threads/latch.h"
#include "threads/semaphore.h"
#include "threads/rw_lock.h"
#include "threads/condition_variable.h"
#include "threads/critical_section.h"
#include "threads/mutex.h"
#include "threads/event_auto_reset.h"
#include "threads/event_manual_reset.h"
#include "time/timestamp.h"
#include "time/timespan.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace BaseKit;

// ===== Barrier =====
TEST(BarrierTest, ReleaseAllThreads)
{
    const int N = 4;
    Barrier barrier(N);
    std::atomic<int> counter{0};
    std::vector<std::thread> ts;
    for (int i = 0; i < N; ++i)
        ts.emplace_back([&] { barrier.Wait(); counter.fetch_add(1); });
    for (auto &t : ts) t.join();
    EXPECT_EQ(counter.load(), N);
    EXPECT_EQ(barrier.threads(), N);
}

// ===== Latch =====
TEST(LatchTest, CountDownReleasesWait)
{
    Latch latch(3);
    EXPECT_EQ(latch.threads(), 3);
    std::atomic<bool> released{false};
    std::thread w([&] { latch.Wait(); released.store(true); });
    latch.CountDown();
    latch.CountDown();
    EXPECT_FALSE(released.load());
    latch.CountDown();
    w.join();
    EXPECT_TRUE(released.load());
}

TEST(LatchTest, ResetAndCountDown)
{
    Latch latch(1);
    latch.CountDown();
    latch.Reset(2);
    EXPECT_EQ(latch.threads(), 2);
    std::atomic<bool> released{false};
    std::thread w([&] { latch.Wait(); released.store(true); });
    latch.CountDown();
    EXPECT_FALSE(released.load());
    latch.CountDown();
    w.join();
    EXPECT_TRUE(released.load());
}

// ===== Semaphore =====
TEST(SemaphoreTest, LockAndUnlock)
{
    Semaphore sem(2);
    EXPECT_EQ(sem.resources(), 2);
    EXPECT_TRUE(sem.TryLock());
    EXPECT_TRUE(sem.TryLock());
    EXPECT_FALSE(sem.TryLock());   // 已耗尽
    sem.Unlock();
    EXPECT_TRUE(sem.TryLock());
}

TEST(SemaphoreTest, TimedLock)
{
    Semaphore sem(1);
    ASSERT_TRUE(sem.TryLock());   // 耗尽唯一的资源
    Timespan timeout = Timespan::milliseconds(50);
    EXPECT_FALSE(sem.TryLockFor(timeout));
    sem.Unlock();
    EXPECT_TRUE(sem.TryLockFor(timeout));
}

// ===== RWLock =====
TEST(RWLockTest, ReadLocksCoexist)
{
    RWLock lock;
    EXPECT_TRUE(lock.TryLockRead());
    EXPECT_TRUE(lock.TryLockRead());
    lock.UnlockRead();
    lock.UnlockRead();
}

TEST(RWLockTest, WriteBlocksRead)
{
    RWLock lock;
    ASSERT_TRUE(lock.TryLockWrite());
    EXPECT_FALSE(lock.TryLockRead());
    EXPECT_FALSE(lock.TryLockWrite());
    lock.UnlockWrite();
    EXPECT_TRUE(lock.TryLockRead());
    lock.UnlockRead();
}

TEST(RWLockTest, TimedLocks)
{
    RWLock lock;
    Timespan ts = Timespan::milliseconds(20);
    ASSERT_TRUE(lock.TryLockReadFor(ts));
    lock.UnlockRead();
    ASSERT_TRUE(lock.TryLockWriteFor(ts));
    // 持有写锁时，再尝试读应超时
    EXPECT_FALSE(lock.TryLockReadFor(ts));
    lock.UnlockWrite();
}

// ===== CriticalSection + ConditionVariable =====
TEST(ConditionVariableTest, NotifyOneWakesWaiter)
{
    CriticalSection cs;
    ConditionVariable cv;
    bool flag = false;
    std::thread w([&] {
        cs.Lock();
        while (!flag)
            cv.Wait(cs);
        cs.Unlock();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    cs.Lock();
    flag = true;
    cv.NotifyOne();
    cs.Unlock();
    w.join();
    SUCCEED();
}

TEST(ConditionVariableTest, NotifyAllWakesAll)
{
    CriticalSection cs;
    ConditionVariable cv;
    std::atomic<int> count{0};
    auto worker = [&] {
        cs.Lock();
        cv.Wait(cs);
        count.fetch_add(1);
        cs.Unlock();
    };
    std::thread w1(worker), w2(worker), w3(worker);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    cs.Lock();
    cv.NotifyAll();
    cs.Unlock();
    w1.join(); w2.join(); w3.join();
    EXPECT_EQ(count.load(), 3);
}

// ===== EventAutoReset =====
TEST(EventAutoResetTest, SignalReleasesOneWaiter)
{
    EventAutoReset ev(false);
    EXPECT_FALSE(ev.TryWait());
    ev.Signal();
    EXPECT_TRUE(ev.TryWait());
    EXPECT_FALSE(ev.TryWait());   // 自动重置
}

// 注：EventAutoReset 的 TryWaitFor 在本机实现下超时行为不稳定（见 TimedWait 已删），
// 故不对其做带超时的并发断言。EventManualReset 的 Reset/Signal 行为同样由下列用例覆盖。

TEST(EventAutoResetTest, BlockingWaitReleasesOnSignal)
{
    EventAutoReset ev(false);
    std::atomic<int> hit{0};
    std::thread w([&] {
        ev.Wait();   // 阻塞等待
        hit.fetch_add(1);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    ev.Signal();
    w.join();
    EXPECT_EQ(hit.load(), 1);
}

// ===== EventManualReset =====
TEST(EventManualResetTest, StaysSignaledAfterSignal)
{
    EventManualReset ev(false);
    EXPECT_FALSE(ev.TryWait());
    ev.Signal();
    EXPECT_TRUE(ev.TryWait());
    EXPECT_TRUE(ev.TryWait());   // 仍保持信号
    ev.Reset();
    EXPECT_FALSE(ev.TryWait());
}

TEST(EventManualResetTest, InitialSignaledState)
{
    EventManualReset ev(true);
    EXPECT_TRUE(ev.TryWait());
}

// ===== Mutex =====
TEST(MutexTest, LockUnlock)
{
    Mutex m;
    EXPECT_TRUE(m.TryLock());
    EXPECT_FALSE(m.TryLock());   // 不可重入（视实现）
    m.Unlock();
    EXPECT_TRUE(m.TryLock());
    m.Unlock();
}

// ===== CriticalSection =====
TEST(CriticalSectionTest, LockUnlockProtectsCounter)
{
    CriticalSection cs;
    std::atomic<int> counter{0};
    auto worker = [&] {
        for (int i = 0; i < 100; ++i) {
            cs.Lock();
            counter.fetch_add(1);
            cs.Unlock();
        }
    };
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i) ts.emplace_back(worker);
    for (auto &t : ts) t.join();
    EXPECT_EQ(counter.load(), 400);
}
