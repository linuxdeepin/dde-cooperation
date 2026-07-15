// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "system/stack_trace.h"
#include "system/stack_trace_manager.h"

#include <sstream>
#include <string>

using namespace BaseKit;

namespace {
StackTrace MakeTraceDeep(int skip)
{
    return StackTrace(skip);
}
}

TEST(StackTraceTest, DefaultCaptureHasFrames)
{
    StackTrace st;
    EXPECT_FALSE(st.frames().empty());
}

TEST(StackTraceTest, CaptureWithSkip)
{
    StackTrace st0;
    StackTrace st2(2);
    EXPECT_FALSE(st0.frames().empty());
    EXPECT_FALSE(st2.frames().empty());
    EXPECT_LE(st2.frames().size(), st0.frames().size());
}

TEST(StackTraceTest, FrameAddressAndString)
{
    StackTrace st;
    ASSERT_FALSE(st.frames().empty());
    const auto& frame = st.frames().front();
    EXPECT_NE(frame.address, nullptr);
    std::string s = frame.string();
    EXPECT_FALSE(s.empty());
    EXPECT_NE(s.find("0x"), std::string::npos);
}

TEST(StackTraceTest, FrameStringForEmptyFields)
{
    StackTrace::Frame frame;
    frame.address = nullptr;
    frame.module = "";
    frame.function = "";
    frame.filename = "";
    frame.line = 0;
    std::string s = frame.string();
    EXPECT_NE(s.find("<unknown>"), std::string::npos);
    EXPECT_NE(s.find("??"), std::string::npos);
}

TEST(StackTraceTest, FrameStringWithLine)
{
    StackTrace::Frame frame;
    frame.address = nullptr;
    frame.module = "mymod";
    frame.function = "myfunc";
    frame.filename = "file.cpp";
    frame.line = 42;
    std::string s = frame.string();
    EXPECT_NE(s.find("mymod"), std::string::npos);
    EXPECT_NE(s.find("myfunc"), std::string::npos);
    EXPECT_NE(s.find("file.cpp"), std::string::npos);
    EXPECT_NE(s.find("(42)"), std::string::npos);
}

TEST(StackTraceTest, StackTraceStringNonEmpty)
{
    StackTrace st;
    std::string s = st.string();
    EXPECT_FALSE(s.empty());
}

TEST(StackTraceTest, OutputStreamOperator)
{
    StackTrace st;
    std::ostringstream oss;
    oss << st;
    EXPECT_FALSE(oss.str().empty());
}

TEST(StackTraceTest, OutputStreamFrameOperator)
{
    StackTrace st;
    ASSERT_FALSE(st.frames().empty());
    std::ostringstream oss;
    oss << st.frames().front();
    EXPECT_FALSE(oss.str().empty());
}

TEST(StackTraceTest, StackTraceMacro)
{
    StackTrace st = __STACK__;
    EXPECT_FALSE(st.frames().empty());
}

TEST(StackTraceTest, CopyAndMoveSemantics)
{
    StackTrace st;
    size_t n = st.frames().size();

    StackTrace copied(st);
    EXPECT_EQ(copied.frames().size(), n);

    StackTrace moved(std::move(copied));
    EXPECT_EQ(moved.frames().size(), n);

    StackTrace assigned;
    assigned = st;
    EXPECT_EQ(assigned.frames().size(), n);

    StackTrace moveAssigned;
    moveAssigned = std::move(assigned);
    EXPECT_EQ(moveAssigned.frames().size(), n);
}

TEST(StackTraceTest, CaptureFromNestedFunction)
{
    StackTrace st = MakeTraceDeep(0);
    EXPECT_FALSE(st.frames().empty());
}

TEST(StackTraceManagerTest, InitializeAndCleanupAreIdempotent)
{
    EXPECT_NO_THROW(StackTraceManager::Initialize());
    EXPECT_NO_THROW(StackTraceManager::Initialize());
    EXPECT_NO_THROW(StackTraceManager::Cleanup());
    EXPECT_NO_THROW(StackTraceManager::Cleanup());
}

TEST(StackTraceManagerTest, InitializeThenCaptureThenCleanup)
{
    StackTraceManager::Initialize();
    StackTrace st;
    EXPECT_FALSE(st.frames().empty());
    StackTraceManager::Cleanup();
}
