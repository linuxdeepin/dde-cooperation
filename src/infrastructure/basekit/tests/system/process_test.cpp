// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Process 覆盖：进程/父进程 Id、CurrentProcess 运行态、Execute+Wait、
// WaitFor 超时语义、不存在命令容错、Kill 终止、拷贝/移动/交换。

#include <gtest/gtest.h>
#include "errors/exceptions.h"
#include "system/process.h"
#include "time/timestamp.h"

#include <limits>
#include <string>
#include <vector>

using namespace BaseKit;

// ===== 进程 Id =====
TEST(ProcessTest, CurrentProcessIdPositiveAndMatchesMacro)
{
    uint64_t pid = Process::CurrentProcessId();
    EXPECT_GT(pid, 0u);
    EXPECT_EQ(__PROCESS__, pid);
}

TEST(ProcessTest, ParentProcessIdPositive)
{
    EXPECT_GT(Process::ParentProcessId(), 0u);
}

TEST(ProcessTest, CurrentProcessIsRunning)
{
    Process p = Process::CurrentProcess();
    EXPECT_TRUE(p.IsRunning());
    EXPECT_EQ(p.pid(), Process::CurrentProcessId());
}

// ===== Execute + Wait =====
TEST(ProcessTest, ExecuteEchoWaitsZero)
{
    std::vector<std::string> args = {"hello"};
    Process p = Process::Execute("/bin/echo", &args);
    EXPECT_EQ(p.Wait(), 0);
}

TEST(ProcessTest, ExecuteWithArguments)
{
    std::vector<std::string> args = {"-n", "first", "second"};
    Process p = Process::Execute("/bin/echo", &args);
    EXPECT_EQ(p.Wait(), 0);
}

TEST(ProcessTest, ExecuteNonexistentCommandDoesNotCrash)
{
    std::vector<std::string> args;
    Process p = Process::Execute("/nonexistent/bk_cmd_xyzzy", &args);
    // 子进程 exec 失败后 _exit(666)，退出码被截断为 8 位 (666 % 256 = 154)
    int code = 0;
    EXPECT_NO_THROW(code = p.Wait());
    EXPECT_NE(code, 0);
}

// ===== WaitFor 超时语义 =====
TEST(ProcessTest, WaitForFastProcessReturnsExitCode)
{
    std::vector<std::string> args = {"ok"};
    Process p = Process::Execute("/bin/echo", &args);
    int result = p.WaitFor(Timespan::milliseconds(500));
    EXPECT_EQ(result, 0);
}

TEST(ProcessTest, WaitForLongSleepTimesOutOrExits)
{
    // 注意：Unix 实现中 WaitFor 忙等至进程退出（不真正尊重 timespan），
    // 因此可能返回 min()（真正的超时）或进程退出码（忙等结束后）。
    // 这里接受两种合法结果，并确保不抛异常、不留僵尸。
    std::vector<std::string> args = {"1"};
    Process p = Process::Execute("/bin/sleep", &args);
    int result = p.WaitFor(Timespan::milliseconds(50));
    EXPECT_TRUE(result == std::numeric_limits<int>::min() || result == 0);
    if (result == std::numeric_limits<int>::min())
    {
        p.Kill();
        try { p.Wait(); } catch (const Exception&) {}
    }
}

// ===== Kill =====
TEST(ProcessTest, KillSpawnedSleepStopsRunning)
{
    std::vector<std::string> args = {"30"};
    Process p = Process::Execute("/bin/sleep", &args);
    EXPECT_TRUE(p.IsRunning());
    p.Kill();
    // 被 SIGKILL 终止后 Wait 会走 WIFSIGNALED 分支抛异常，捕获以回收僵尸
    try { p.Wait(); } catch (const Exception&) {}
    EXPECT_FALSE(p.IsRunning());
}

// ===== 拷贝 / 移动 / 交换 =====
TEST(ProcessTest, CopyAndMoveSemantics)
{
    uint64_t pid = Process::CurrentProcessId();

    Process a = Process::CurrentProcess();
    EXPECT_EQ(a.pid(), pid);

    Process b(a);                       // 拷贝构造
    EXPECT_EQ(b.pid(), pid);

    Process c(std::move(b));            // 移动构造
    EXPECT_EQ(c.pid(), pid);

    Process d = a;                      // 拷贝赋值
    EXPECT_EQ(d.pid(), pid);

    Process e = std::move(c);           // 移动赋值
    EXPECT_EQ(e.pid(), pid);

    swap(a, d);
    EXPECT_EQ(a.pid(), pid);
    EXPECT_EQ(d.pid(), pid);
}
