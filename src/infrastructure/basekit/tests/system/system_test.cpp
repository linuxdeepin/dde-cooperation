// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// basekit system 子模块测试：CPU / Process / Pipe / DLL + threads/FileLock。

#include <gtest/gtest.h>
#include "system/cpu.h"
#include "system/process.h"
#include "system/pipe.h"
#include "system/dll.h"
#include "threads/file_lock.h"
#include "filesystem/path.h"

#include <cstdio>
#include <string>
#include <thread>

using namespace BaseKit;

// ===== CPU =====
TEST(CPUTest, ArchitectureNonEmpty)
{
    auto arch = CPU::Architecture();
    EXPECT_FALSE(arch.empty());
}

TEST(CPUTest, AffinityPositive)
{
    int aff = CPU::Affinity();
    EXPECT_GT(aff, 0);
}

TEST(CPUTest, CoresCounts)
{
    EXPECT_GT(CPU::LogicalCores(), 0);
    EXPECT_GT(CPU::PhysicalCores(), 0);
    auto total = CPU::TotalCores();
    EXPECT_GE(total.first, 0);
    EXPECT_GE(total.second, 0);
}

TEST(CPUTest, ClockSpeedAndHyperThreading)
{
    int64_t hz = CPU::ClockSpeed();
    EXPECT_GE(hz, 0);   // 部分环境可能返回 0
    EXPECT_NO_THROW((void)CPU::HyperThreading());
}

// ===== Process =====
TEST(ProcessTest, CurrentProcessHasPid)
{
    EXPECT_GT(Process::CurrentProcessId(), 0u);
    EXPECT_GT(Process::ParentProcessId(), 0u);
    Process me = Process::CurrentProcess();
    EXPECT_GT(me.pid(), 0u);
    EXPECT_TRUE(me.IsRunning());
    Process parent = Process::ParentProcess();
    EXPECT_GT(parent.pid(), 0u);
}

TEST(ProcessTest, ExecuteEchoAndWait)
{
    Pipe output;
    std::vector<std::string> args = {"hi"};
    Process p = Process::Execute("/bin/echo", &args, nullptr, nullptr, nullptr, &output, nullptr);
    int code = p.Wait();
    EXPECT_EQ(code, 0);
}

TEST(ProcessTest, ExecuteInvalidDoesNotThrow)
{
    // 无效路径不抛异常，返回无效/已退出进程
    EXPECT_NO_THROW(Process::Execute("/no/such/program/xyz", nullptr));
}

// ===== Pipe =====
TEST(PipeTest, WriteAndReadBack)
{
    Pipe pipe;
    const std::string msg = "pipe-data";
    size_t w = pipe.Write(msg.data(), msg.size());
    EXPECT_EQ(w, msg.size());

    std::vector<uint8_t> buf(msg.size());
    size_t r = pipe.Read(buf.data(), buf.size());
    EXPECT_EQ(r, msg.size());
    EXPECT_EQ(std::string((char *)buf.data(), r), msg);
}

TEST(PipeTest, ReaderWriterHandles)
{
    Pipe pipe;
    EXPECT_NE(pipe.reader(), nullptr);
    EXPECT_NE(pipe.writer(), nullptr);
}

// ===== DLL =====
TEST(DLLTest, LoadLibzAndResolve)
{
    Path libz("/usr/lib/x86_64-linux-gnu/libz.so");
    DLL dll(libz, true);
    EXPECT_TRUE((bool)dll);
    auto fn = dll.Resolve<const char*()>("zlibVersion");
    ASSERT_NE(fn, nullptr);
    EXPECT_NE(fn(), nullptr);
}

TEST(DLLTest, ResolveMissingSymbolIsNull)
{
    DLL dll(Path("/usr/lib/x86_64-linux-gnu/libz.so"), true);
    ASSERT_TRUE((bool)dll);
    auto fn = dll.Resolve<const char*()>("nonexistent_symbol_xyz");
    EXPECT_EQ(fn, nullptr);
}

TEST(DLLTest, InvalidPathIsInvalid)
{
    DLL dll(Path("/no/such/lib.so"), true);
    EXPECT_FALSE((bool)dll);
}

TEST(DLLTest, ManualLoadUnload)
{
    DLL dll;
    EXPECT_FALSE((bool)dll);
    dll.Load(Path("/usr/lib/x86_64-linux-gnu/libz.so"));
    EXPECT_TRUE((bool)dll);
    dll.Unload();
    EXPECT_FALSE((bool)dll);
}

TEST(DLLTest, IsResolveQuery)
{
    DLL dll(Path("/usr/lib/x86_64-linux-gnu/libz.so"), true);
    ASSERT_TRUE((bool)dll);
    EXPECT_TRUE(dll.IsResolve("zlibVersion"));
    EXPECT_FALSE(dll.IsResolve("no_such_symbol"));
}

// ===== FileLock =====
namespace {
class TempPath
{
public:
    TempPath()
    {
        char tmpl[] = "/tmp/bk_lock_XXXXXX";
        int fd = mkstemp(tmpl);
        if (fd < 0) throw std::runtime_error("mkstemp failed");
        close(fd);
        _p = tmpl;
    }
    ~TempPath() { if (!_p.empty()) std::remove(_p.c_str()); }
    const std::string &str() const { return _p; }
private:
    std::string _p;
};
}   // namespace

TEST(FileLockTest, AcquireReadAndRelease)
{
    TempPath tp;
    FileLock lock(Path(tp.str()));
    EXPECT_EQ(lock.path().string(), tp.str());
    EXPECT_TRUE(lock.TryLockRead());
    lock.UnlockRead();
}

TEST(FileLockTest, AcquireWriteAndRelease)
{
    TempPath tp;
    FileLock lock(Path(tp.str()));
    EXPECT_TRUE(lock.TryLockWrite());
    lock.UnlockWrite();
}

TEST(FileLockTest, AssignmentFromPath)
{
    TempPath tp;
    FileLock lock;
    lock = Path(tp.str());
    EXPECT_EQ(lock.path().string(), tp.str());
    EXPECT_TRUE(lock.TryLockRead());
    lock.UnlockRead();
}

// ===== StdStream =====
#include "system/stream.h"

TEST(StreamTest, StdOutputWriteAndFlush)
{
    StdOutput out;
    EXPECT_TRUE(out.IsValid());
    const char *msg = "stream-test";
    EXPECT_EQ(out.Write(msg, std::strlen(msg)), std::strlen(msg));
    EXPECT_NO_THROW(out.Flush());
}

TEST(StreamTest, StdErrorWrite)
{
    StdError err;
    EXPECT_TRUE(err.IsValid());
    const char *msg = "err\n";
    EXPECT_EQ(err.Write(msg, std::strlen(msg)), std::strlen(msg));
}

TEST(StreamTest, StdInputValid)
{
    StdInput in;
    EXPECT_TRUE(in.IsValid());
    EXPECT_NE(in.stream(), nullptr);
}
