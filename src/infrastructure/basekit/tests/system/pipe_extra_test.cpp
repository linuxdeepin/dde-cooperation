// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "errors/exceptions.h"
#include "system/pipe.h"

#include <string>
#include <vector>

using namespace BaseKit;

TEST(PipeExtraTest, FreshlyCreatedPipeIsOpened)
{
    Pipe pipe;
    EXPECT_TRUE(pipe.IsPipeOpened());
    EXPECT_TRUE(pipe.IsPipeReadOpened());
    EXPECT_TRUE(pipe.IsPipeWriteOpened());
}

TEST(PipeExtraTest, WriteStringOverloadAndReadBack)
{
    Pipe pipe;
    size_t w = pipe.Write("abcde");
    EXPECT_EQ(w, 5u);

    std::vector<uint8_t> buf(8, 0);
    size_t r = pipe.Read(buf.data(), buf.size());
    EXPECT_EQ(r, 5u);
    EXPECT_EQ(std::string((char*)buf.data(), r), "abcde");
}

TEST(PipeExtraTest, WriteLinesOverload)
{
    Pipe pipe;
    std::vector<std::string> lines = {"one", "two"};
    size_t w = pipe.Write(lines);
    EXPECT_GT(w, 0u);
}

TEST(PipeExtraTest, CloseReadDisablesReadEnd)
{
    Pipe pipe;
    EXPECT_TRUE(pipe.IsPipeReadOpened());
    pipe.CloseRead();
    EXPECT_FALSE(pipe.IsPipeReadOpened());
    EXPECT_TRUE(pipe.IsPipeWriteOpened());
}

TEST(PipeExtraTest, CloseWriteDisablesWriteEnd)
{
    Pipe pipe;
    EXPECT_TRUE(pipe.IsPipeWriteOpened());
    pipe.CloseWrite();
    EXPECT_FALSE(pipe.IsPipeWriteOpened());
    EXPECT_TRUE(pipe.IsPipeReadOpened());
}

TEST(PipeExtraTest, CloseAllEndpoints)
{
    Pipe pipe;
    pipe.Close();
    EXPECT_FALSE(pipe.IsPipeOpened());
    EXPECT_FALSE(pipe.IsPipeReadOpened());
    EXPECT_FALSE(pipe.IsPipeWriteOpened());
}

TEST(PipeExtraTest, ReaderAndWriterHandlesNonNull)
{
    Pipe pipe;
    EXPECT_NE(pipe.reader(), nullptr);
    EXPECT_NE(pipe.writer(), nullptr);
}
