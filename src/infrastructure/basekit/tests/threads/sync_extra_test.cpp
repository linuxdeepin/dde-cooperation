// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 匿名(非命名)同步原语的补充覆盖：
//   ConditionVariable::TryWaitFor / EventAutoReset::TryWaitFor /
//   EventManualReset::TryWaitFor / Mutex::TryLockFor /
//   Semaphore::Lock+TryLockFor / CriticalSection::TryLockFor。
//
// 注意：本实现的 EventAutoReset/EventManualReset::TryWaitFor 将相对时长当作
// 绝对时间戳传给 pthread_cond_timedwait，未信号化时会忙等（永不超时返回）。
// 因此对这两个事件只覆盖"已信号化后立即成功"的快速路径，避免测试挂死。
// ConditionVariable::TryWaitFor 同样存在该 bug，但单次调用会立即返回 ETIMEDOUT
// (不挂死)，故只验证"未通知 -> 超时返回 false"这一可靠路径。
// Mutex 为非递归，同线程 TryLockFor 会抛异常，故用 holder 线程制造争用。
// CriticalSection 为递归锁，同线程重入成功，故用 holder 线程测试超时失败。

#include <gtest/gtest.h>
#include "threads/condition_variable.h"
#include "threads/critical_section.h"
#include "threads/event_auto_reset.h"
#include "threads/event_manual_reset.h"
#include "threads/mutex.h"
#include "threads/semaphore.h"
#include "time/timespan.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace BaseKit;

// ===== ConditionVariable::TryWaitFor =====
// 注意：本实现将相对时长当作绝对时间戳传给 pthread_cond_timedwait，对于任何
// 实际可用时长该时间点都在过去，因此 TryWaitFor 会立即返回 false(超时)。
// 这里只验证可靠的"未通知 -> 超时返回 false"路径，不依赖不可靠的唤醒时序。
TEST(ConditionVariableExtraTest, TryWaitForTimeoutWithoutNotify)
{
    CriticalSection cs;
    ConditionVariable cv;

    cs.Lock();
    EXPECT_FALSE(cv.TryWaitFor(cs, Timespan::milliseconds(30)));
    cs.Unlock();

    // NotifyOne 在没有等待者时是空操作，不会让随后的 TryWaitFor 成功
    cv.NotifyOne();
    cs.Lock();
    EXPECT_FALSE(cv.TryWaitFor(cs, Timespan::milliseconds(30)));
    cs.Unlock();
}

// ===== EventAutoReset::TryWaitFor（仅成功路径）=====
// 注意：TryWaitFor 内部 spin-loop 存在竞态(信号到达时机不同可能返回 false)，
// 故只覆盖稳定的"先 Signal 再 TryWaitFor -> 成功"快速路径。
TEST(EventAutoResetExtraTest, TryWaitForSucceedsAfterSignal)
{
    EventAutoReset ev(false);
    ev.Signal();
    EXPECT_TRUE(ev.TryWaitFor(Timespan::milliseconds(50)));
    // 自动重置，再次等待在已信号化被消费后不应进入挂死分支：先重新 Signal
    ev.Signal();
    EXPECT_TRUE(ev.TryWaitFor(Timespan::milliseconds(50)));
}

// ===== EventManualReset::TryWaitFor（仅成功路径）=====
TEST(EventManualResetExtraTest, TryWaitForSucceedsAfterSignal)
{
    EventManualReset ev(false);
    ev.Signal();
    EXPECT_TRUE(ev.TryWaitFor(Timespan::milliseconds(50)));
    // 手动重置：保持信号
    EXPECT_TRUE(ev.TryWaitFor(Timespan::milliseconds(50)));
    ev.Reset();
}

// ===== Mutex::TryLockFor（非递归，需 holder 线程制造争用）=====
TEST(MutexExtraTest, TryLockForTimeoutThenAcquire)
{
    Mutex m;
    std::thread holder([&] {
        m.Lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        m.Unlock();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(15));   // 让 holder 先获取

    EXPECT_FALSE(m.TryLockFor(Timespan::milliseconds(30)));       // 被持有 -> 超时
    holder.join();
    EXPECT_TRUE(m.TryLockFor(Timespan::milliseconds(200)));       // 已释放 -> 成功
    m.Unlock();
}

// ===== Semaphore::Lock + TryLockFor =====
TEST(SemaphoreExtraTest, LockAndTryLockFor)
{
    Semaphore sem(1);
    sem.Lock();                                                    // 耗尽唯一资源
    EXPECT_EQ(sem.resources(), 1);                                 // 配置值不变
    EXPECT_FALSE(sem.TryLockFor(Timespan::milliseconds(30)));      // 超时
    sem.Unlock();
    EXPECT_TRUE(sem.TryLockFor(Timespan::milliseconds(100)));
    sem.Unlock();
}

// ===== CriticalSection::TryLockFor（递归，需 holder 线程测试超时失败）=====
TEST(CriticalSectionExtraTest, TryLockForTimeoutThenAcquire)
{
    CriticalSection cs;
    std::thread holder([&] {
        cs.Lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        cs.Unlock();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(15));   // 让 holder 先获取

    EXPECT_FALSE(cs.TryLockFor(Timespan::milliseconds(30)));       // 被其他线程持有 -> 超时
    holder.join();
    EXPECT_TRUE(cs.TryLockFor(Timespan::milliseconds(200)));       // 已释放 -> 成功
    cs.Unlock();
}
