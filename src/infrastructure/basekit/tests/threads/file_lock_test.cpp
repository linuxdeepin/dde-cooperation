// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// FileLock 覆盖测试：Assign/path/LockWrite/LockRead/TryLock*/TryLock*For/Reset。
// 使用 PID 生成唯一临时文件路径，避免与既有进程冲突。

#include <gtest/gtest.h>
#include "filesystem/path.h"
#include "threads/file_lock.h"
#include "time/timespan.h"

#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>

using namespace BaseKit;

namespace {
std::string tmpPath(const char *tag)
{
    return std::string("/tmp/bk_filelock_") + tag + "_" + std::to_string(getpid());
}
}   // namespace

// 默认构造 + Assign 赋值 + LockWrite/UnlockWrite
TEST(FileLockTest, AssignAndLockWriteUnlockWrite)
{
    FileLock fl;
    fl.Assign(Path(tmpPath("assign")));
    fl.LockWrite();
    fl.UnlockWrite();
    fl.Reset();
}

// 直接以 Path 构造，TryLockRead/TryLockWrite 在空闲文件上成功
TEST(FileLockTest, TryLockReadAndWriteOnFreshFile)
{
    FileLock fl(Path(tmpPath("try")));
    EXPECT_TRUE(fl.TryLockWrite());
    fl.UnlockWrite();
    EXPECT_TRUE(fl.TryLockRead());
    fl.UnlockRead();
    fl.Reset();
}

// path() 取值
TEST(FileLockTest, PathGetter)
{
    const auto p = tmpPath("path");
    FileLock fl{Path(p)};
    EXPECT_EQ(fl.path().string(), p);
    fl.Reset();
}

// 阻塞式 LockRead/UnlockRead
TEST(FileLockTest, LockReadUnlockRead)
{
    FileLock fl(Path(tmpPath("read")));
    fl.LockRead();
    fl.UnlockRead();
    fl.Reset();
}

// TryLockReadFor 在空闲文件上立即成功
TEST(FileLockTest, TryLockReadForShortTimeout)
{
    FileLock fl(Path(tmpPath("rlfor")));
    EXPECT_TRUE(fl.TryLockReadFor(Timespan::milliseconds(50)));
    fl.UnlockRead();
    fl.Reset();
}

// TryLockWriteFor 在空闲文件上立即成功
TEST(FileLockTest, TryLockWriteForShortTimeout)
{
    FileLock fl(Path(tmpPath("wlfor")));
    EXPECT_TRUE(fl.TryLockWriteFor(Timespan::milliseconds(50)));
    fl.UnlockWrite();
    fl.Reset();
}

// 两个独立 FileLock(不同 fd) 在同一文件上争用：写锁持有时，读/写 TryLockFor 超时
TEST(FileLockTest, TryLockWriteForTimesOutOnContention)
{
    const auto p = tmpPath("contend");
    FileLock holder{Path(p)};
    ASSERT_TRUE(holder.TryLockWrite());

    FileLock contender{Path(p)};
    // 写锁被持有时，竞争者的读/写请求应超时失败
    EXPECT_FALSE(contender.TryLockReadFor(Timespan::milliseconds(50)));
    EXPECT_FALSE(contender.TryLockWriteFor(Timespan::milliseconds(50)));

    holder.UnlockWrite();
    // 释放后竞争者可以获取
    EXPECT_TRUE(contender.TryLockWriteFor(Timespan::milliseconds(200)));
    contender.UnlockWrite();
}

// operator= 以 Path 赋值，行为与 Assign 一致
TEST(FileLockTest, AssignViaOperator)
{
    FileLock fl;
    fl = Path(tmpPath("opassign"));
    EXPECT_TRUE(fl.TryLockWrite());
    fl.UnlockWrite();
    fl.Reset();
}
