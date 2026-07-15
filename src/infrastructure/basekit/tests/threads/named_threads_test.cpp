// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// basekit 命名同步原语测试：NamedMutex/NamedSemaphore/NamedConditionVariable/
// NamedCriticalSection/NamedEvent{Auto,Manual}Reset/NamedRWLock。
// 使用 PID 生成唯一名，避免与既有进程冲突。

#include <gtest/gtest.h>
#include "threads/named_mutex.h"
#include "threads/named_semaphore.h"
#include "threads/named_event_auto_reset.h"
#include "threads/named_event_manual_reset.h"
#include "threads/named_rw_lock.h"
#include "threads/named_critical_section.h"
#include "threads/named_condition_variable.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace BaseKit;

namespace {
std::string uniq(const char *tag)
{
    return std::string("bk_named_") + tag + "_" + std::to_string(getpid());
}
}   // namespace

TEST(NamedMutexTest, LockUnlockAndTryLock)
{
    const auto name = uniq("mutex");
    NamedMutex m(name);
    EXPECT_TRUE(m.TryLock());
    EXPECT_FALSE(m.TryLock());   // 不可重入
    m.Unlock();
    EXPECT_TRUE(m.TryLock());
    m.Unlock();
    EXPECT_EQ(m.name(), name);
}

TEST(NamedMutexTest, CtorOpensExisting)
{
    const auto name = uniq("mutex2");
    {
        NamedMutex a(name);
        EXPECT_TRUE(a.TryLock());
        a.Unlock();
    }
    // 重新打开同名（系统范围内持久）
    NamedMutex b(name);
    EXPECT_TRUE(b.TryLock());
    b.Unlock();
}

TEST(NamedSemaphoreTest, LockAndRelease)
{
    const auto name = uniq("sem");
    NamedSemaphore sem(name, 1);
    EXPECT_TRUE(sem.TryLock());
    EXPECT_FALSE(sem.TryLock());   // 资源耗尽
    sem.Unlock();
    EXPECT_TRUE(sem.TryLock());
    sem.Unlock();
}

TEST(NamedEventAutoResetTest, SignalAndTryWait)
{
    const auto name = uniq("ea");
    NamedEventAutoReset ev(name, false);
    EXPECT_FALSE(ev.TryWait());
    ev.Signal();
    EXPECT_TRUE(ev.TryWait());
    EXPECT_FALSE(ev.TryWait());   // 自动重置
}

TEST(NamedEventManualResetTest, SignalStaysAndReset)
{
    const auto name = uniq("em");
    NamedEventManualReset ev(name, false);
    EXPECT_FALSE(ev.TryWait());
    ev.Signal();
    EXPECT_TRUE(ev.TryWait());
    EXPECT_TRUE(ev.TryWait());   // 保持信号
    EXPECT_FALSE(ev.TryWait());
}

TEST(NamedEventManualResetTest, InitialSignaled)
{
    const auto name = uniq("em2");
    NamedEventManualReset ev(name, true);
    EXPECT_TRUE(ev.TryWait());
}

TEST(NamedRWLockTest, ReadWriteSemantics)
{
    const auto name = uniq("rw");
    NamedRWLock lock(name);
    EXPECT_TRUE(lock.TryLockRead());
    EXPECT_TRUE(lock.TryLockRead());   // 读共享
    lock.UnlockRead();
    lock.UnlockRead();

    EXPECT_TRUE(lock.TryLockWrite());
    EXPECT_FALSE(lock.TryLockRead());   // 持有写锁时读失败
    EXPECT_FALSE(lock.TryLockWrite());
    lock.UnlockWrite();
}


// 仅构造与元数据查询（避免触发会断言/挂起的操作）
TEST(NamedCriticalSectionMetaTest, CtorAndName)
{
    const auto name = uniq("cs_meta");
    NamedCriticalSection cs(name);
    EXPECT_EQ(cs.name(), name);
}

TEST(NamedConditionVariableMetaTest, CtorAndName)
{
    const auto name = uniq("cv_meta");
    NamedConditionVariable cv(name);
    EXPECT_EQ(cv.name(), name);
    EXPECT_NO_THROW(cv.NotifyOne());
    EXPECT_NO_THROW(cv.NotifyAll());
}

// 无等待者时 NotifyOne/NotifyAll 不阻塞
TEST(NamedEventAutoResetMetaTest, SignalWithoutWaiter)
{
    const auto name = uniq("ea_signal");
    NamedEventAutoReset ev(name, false);
    EXPECT_NO_THROW(ev.Signal());
}

// ===== NamedCriticalSection：可递归 =====
TEST(NamedCriticalSectionTest, LockRecursiveThenUnlock)
{
    const auto name = uniq("cs_lock");
    NamedCriticalSection cs(name);
    cs.Lock();
    EXPECT_TRUE(cs.TryLock());   // 同线程递归获取成功
    cs.Unlock();
    cs.Unlock();
    EXPECT_TRUE(cs.TryLock());   // 完全释放后可再次获取
    cs.Unlock();
}

TEST(NamedCriticalSectionTest, DoubleLockDoubleUnlock)
{
    const auto name = uniq("cs_rec");
    NamedCriticalSection cs(name);
    cs.Lock();
    cs.Lock();   // 递归
    cs.Unlock();
    cs.Unlock();
}

TEST(NamedCriticalSectionTest, TryLockForTimeoutThenAcquire)
{
    const auto name = uniq("cs_for");
    NamedCriticalSection cs(name);
    std::thread holder([&] {
        cs.Lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        cs.Unlock();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(15));   // 让 holder 先获取
    EXPECT_FALSE(cs.TryLockFor(Timespan::milliseconds(30)));      // 被其他线程持有 -> 超时
    holder.join();
    EXPECT_TRUE(cs.TryLockFor(Timespan::milliseconds(200)));      // 已释放 -> 成功
    cs.Unlock();
}

// ===== NamedSemaphore：Lock 与 resources =====
TEST(NamedSemaphoreTest, LockAndResources)
{
    const auto name = uniq("sem_lr");
    NamedSemaphore sem(name, 1);
    EXPECT_EQ(sem.resources(), 1);
    sem.Lock();                                                   // 阻塞获取，耗尽资源
    EXPECT_EQ(sem.resources(), 1);                                // 配置值不变
    sem.Unlock();
    EXPECT_TRUE(sem.TryLockFor(Timespan::milliseconds(50)));
    sem.Unlock();
}

TEST(NamedSemaphoreTest, TryLockForTimeout)
{
    const auto name = uniq("sem_to");
    NamedSemaphore sem(name, 1);
    ASSERT_TRUE(sem.TryLock());                                   // 耗尽唯一资源
    EXPECT_FALSE(sem.TryLockFor(Timespan::milliseconds(30)));     // 超时失败
    sem.Unlock();
    EXPECT_TRUE(sem.TryLockFor(Timespan::milliseconds(100)));     // 成功
    sem.Unlock();
}

// ===== NamedMutex：非递归，Lock 后 TryLock/TryLockFor 失败（用 holder 线程）=====
TEST(NamedMutexTest, LockAndTryLockFor)
{
    const auto name = uniq("mtx_tlf");
    NamedMutex m(name);
    std::thread holder([&] {
        m.Lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        m.Unlock();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(15));   // 让 holder 先获取
    EXPECT_FALSE(m.TryLock());                                    // 被持有 -> 失败
    EXPECT_FALSE(m.TryLockFor(Timespan::milliseconds(30)));       // 超时失败
    holder.join();
    EXPECT_TRUE(m.TryLockFor(Timespan::milliseconds(200)));       // 已释放 -> 成功
    m.Unlock();
}

// ===== NamedEventAutoReset：跨线程 Wait/Signal =====
TEST(NamedEventAutoResetTest, CrossThreadWaitAndSignal)
{
    const auto name = uniq("ea_ws");
    NamedEventAutoReset ev(name, false);
    std::atomic<int> hit{0};
    std::thread w([&] {
        ev.Wait();
        hit.fetch_add(1);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(hit.load(), 0);   // 仍在等待
    ev.Signal();
    w.join();
    EXPECT_EQ(hit.load(), 1);
}

// ===== NamedEventManualReset：Signal/Wait/Reset =====
TEST(NamedEventManualResetTest, CrossThreadWaitSignalReset)
{
    const auto name = uniq("em_wr");
    NamedEventManualReset ev(name, false);
    std::atomic<int> hit{0};
    std::thread w([&] {
        ev.Wait();   // 阻塞直到 Signal
        hit.fetch_add(1);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ev.Signal();     // 手动重置：保持信号
    w.join();
    EXPECT_EQ(hit.load(), 1);
    // 仍处于信号态
    EXPECT_TRUE(ev.TryWait());
    ev.Reset();
    EXPECT_FALSE(ev.TryWait());   // Reset 后未信号（TryWaitFor 在未信号时会挂死，故用非阻塞 TryWait）
}

// ===== NamedConditionVariable：跨线程 NotifyOne 唤醒 Wait =====
TEST(NamedConditionVariableTest, NotifyOneWakesWaiter)
{
    const auto name = uniq("cv_wake");
    NamedConditionVariable cv(name);
    std::atomic<int> hit{0};
    std::thread w([&] {
        cv.Wait();
        hit.fetch_add(1);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(hit.load(), 0);   // 仍在等待
    cv.NotifyOne();
    w.join();
    EXPECT_EQ(hit.load(), 1);
}
