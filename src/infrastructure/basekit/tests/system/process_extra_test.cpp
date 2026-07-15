// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "errors/exceptions.h"
#include "system/process.h"
#include "system/pipe.h"
#include "time/timestamp.h"

#include <cstring>
#include <string>
#include <vector>

using namespace BaseKit;

TEST(ProcessExtraTest, ParentProcessObjectHasPid)
{
    Process parent = Process::ParentProcess();
    EXPECT_GT(parent.pid(), 0u);
}

TEST(ProcessExtraTest, WaitUntilOnFastEchoReturnsExitCode)
{
    std::vector<std::string> args = {"done"};
    Process p = Process::Execute("/bin/echo", &args);
    UtcTimestamp future(Timestamp::seconds(UtcTimestamp().seconds() + 30));
    int code = p.WaitUntil(future);
    EXPECT_EQ(code, 0);
}

TEST(ProcessExtraTest, ExecuteCapturesStderrViaErrorPipe)
{
    Pipe error;
    std::vector<std::string> args = {"-c", "printf '%s' 'stderr-msg-here' >&2"};
    Process p = Process::Execute("/bin/sh", &args, nullptr, nullptr, nullptr, nullptr, &error);
    EXPECT_EQ(p.Wait(), 0);

    std::vector<uint8_t> buf(64, 0);
    size_t n = error.Read(buf.data(), buf.size());
    EXPECT_GT(n, 0u);
    std::string captured((char*)buf.data(), n);
    EXPECT_NE(captured.find("stderr-msg-here"), std::string::npos);
}

TEST(ProcessExtraTest, ExecuteWithCustomEnvars)
{
    std::map<std::string, std::string> envs = {{"BK_TEST_ENV", "hello-env"}};
    std::vector<std::string> args = {"-c", "printf '%s' \"$BK_TEST_ENV\""};
    Pipe output;
    Process p = Process::Execute("/bin/sh", &args, &envs, nullptr, nullptr, &output, nullptr);
    EXPECT_EQ(p.Wait(), 0);

    std::vector<uint8_t> buf(64, 0);
    size_t n = output.Read(buf.data(), buf.size());
    std::string captured((char*)buf.data(), n);
    EXPECT_EQ(captured, "hello-env");
}

TEST(ProcessExtraTest, ExecuteWithWorkingDirectory)
{
    std::vector<std::string> args;
    std::string dir = "/tmp";
    Pipe output;
    Process p = Process::Execute("/bin/pwd", &args, nullptr, &dir, nullptr, &output, nullptr);
    EXPECT_EQ(p.Wait(), 0);

    std::vector<uint8_t> buf(256, 0);
    size_t n = output.Read(buf.data(), buf.size());
    std::string captured((char*)buf.data(), n);
    EXPECT_NE(captured.find("tmp"), std::string::npos);
}

TEST(ProcessExtraTest, ExecuteWithInputPipe)
{
    Pipe input;
    std::vector<std::string> args;
    Pipe output;
    Process p = Process::Execute("/bin/cat", &args, nullptr, nullptr, &input, &output, nullptr);

    const char msg[] = "piped-input-data";
    input.Write(msg, std::strlen(msg));
    input.CloseWrite();

    EXPECT_EQ(p.Wait(), 0);

    std::vector<uint8_t> buf(64, 0);
    size_t n = output.Read(buf.data(), buf.size());
    std::string captured((char*)buf.data(), n);
    EXPECT_EQ(captured.substr(0, std::strlen(msg)), std::string(msg));
}

TEST(ProcessExtraTest, KillAlreadyExitedProcessReportsNotRunning)
{
    std::vector<std::string> args = {"q"};
    Process p = Process::Execute("/bin/echo", &args);
    EXPECT_EQ(p.Wait(), 0);
    EXPECT_FALSE(p.IsRunning());
}
