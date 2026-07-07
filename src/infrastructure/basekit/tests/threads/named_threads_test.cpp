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
