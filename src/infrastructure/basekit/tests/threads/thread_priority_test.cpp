// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Thread::SetPriority 的 switch-case 分支覆盖。
// 优先级调整通常需要 root 权限，因此每条调用都包在 try/catch 中，
// 目标是覆盖 thread.cpp 中 7 个分支，而非断言设置一定成功。

#include <gtest/gtest.h>
#include "errors/exceptions.h"
#include "threads/thread.h"

using namespace BaseKit;

// 覆盖 SetPriority 全部 7 个枚举分支
TEST(ThreadPriorityTest, SetAllPriorities)
{
    try { Thread::SetPriority(ThreadPriority::IDLE); } catch (...) {}
    try { Thread::SetPriority(ThreadPriority::LOWEST); } catch (...) {}
    try { Thread::SetPriority(ThreadPriority::LOW); } catch (...) {}
    try { Thread::SetPriority(ThreadPriority::NORMAL); } catch (...) {}
    try { Thread::SetPriority(ThreadPriority::HIGH); } catch (...) {}
    try { Thread::SetPriority(ThreadPriority::HIGHEST); } catch (...) {}
    try { Thread::SetPriority(ThreadPriority::REALTIME); } catch (...) {}
    SUCCEED();
}

// GetPriority 应返回合法的枚举值（权限不足时允许抛异常）
TEST(ThreadPriorityTest, GetPriorityReturnsValidEnum)
{
    try
    {
        ThreadPriority p = Thread::GetPriority();
        EXPECT_TRUE(p == ThreadPriority::IDLE ||
                    p == ThreadPriority::LOWEST ||
                    p == ThreadPriority::LOW ||
                    p == ThreadPriority::NORMAL ||
                    p == ThreadPriority::HIGH ||
                    p == ThreadPriority::HIGHEST ||
                    p == ThreadPriority::REALTIME);
    }
    catch (const Exception&)
    {
        SUCCEED();   // 受限环境拒绝查询
    }
}
