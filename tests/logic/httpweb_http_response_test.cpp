// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Regression tests for the HTTP response parser used by the DTP2 stream.
// They cover the header/body split and the open ended stream mode, both of
// which used to corrupt or truncate downloads.

#include <gtest/gtest.h>

#include "http/http_response.h"

#include <string>

using NetUtil::HTTP::HTTPResponse;

namespace {

const std::string kOkHeader = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\n";
const std::string kStreamHeader = "HTTP/1.1 200 OK\r\nX-DTP-Stream: 1\r\n\r\n";

} // namespace

TEST(HttpWebHttpResponseTest, BodyArrivingWithHeaderIsKept)
{
    const std::string raw = kOkHeader + "hello";

    HTTPResponse response;
    EXPECT_TRUE(response.IsPendingHeader());
    ASSERT_TRUE(response.ReceiveHeader(raw.data(), raw.size()));
    EXPECT_EQ(response.status(), 200);
    EXPECT_EQ(response.body_length(), 5u);
    // The body bytes that arrived in the same read are still visible.
    EXPECT_EQ(std::string(response.body()), "hello");

    response.ConsumeHeader();
    EXPECT_EQ(response.cache(), "hello");
    EXPECT_FALSE(response.IsPendingHeader());
    EXPECT_TRUE(response.IsPendingBody());

    // This is exactly what HTTPSClient::onReceived does after the header:
    // it calls ReceiveBody with an empty buffer.
    EXPECT_TRUE(response.ReceiveBody("", 0));

    response.ClearCache();
    EXPECT_TRUE(response.cache().empty());
}

TEST(HttpWebHttpResponseTest, BodySplitAcrossReads)
{
    HTTPResponse response;
    const std::string part = kOkHeader + "he";
    ASSERT_TRUE(response.ReceiveHeader(part.data(), part.size()));
    response.ConsumeHeader();
    EXPECT_EQ(response.cache(), "he");
    response.ClearCache();

    EXPECT_FALSE(response.ReceiveBody("ll", 2));
    EXPECT_EQ(response.cache(), "ll");
    response.ClearCache();

    EXPECT_TRUE(response.ReceiveBody("o", 1));
    EXPECT_EQ(response.cache(), "o");
}

TEST(HttpWebHttpResponseTest, StreamFlagIsDetectedFromHeader)
{
    const std::string raw = kStreamHeader + "payload";

    HTTPResponse response;
    ASSERT_TRUE(response.ReceiveHeader(raw.data(), raw.size()));
    EXPECT_FALSE(response.stream_mode());
    EXPECT_TRUE(response.DetectStreamMode());
    EXPECT_TRUE(response.stream_mode());

    response.ConsumeHeader();
    EXPECT_EQ(response.cache(), "payload");
}

TEST(HttpWebHttpResponseTest, StreamModeIgnoresCrlfTerminator)
{
    HTTPResponse response;
    ASSERT_TRUE(response.ReceiveHeader(kStreamHeader.data(), kStreamHeader.size()));
    ASSERT_TRUE(response.DetectStreamMode());
    response.ConsumeHeader();
    response.ClearCache();

    // A binary archive chunk may legitimately contain CRLFCRLF. The response
    // must not be considered complete because of it.
    // Body ends with CRLFCRLF: without stream mode the heuristic would
    // complete the response. Stream mode must keep it incomplete.
    const char body[] = {'a', 'b', '\r', '\n', '\r', '\n'};
    EXPECT_FALSE(response.ReceiveBody(body, sizeof(body)));
    EXPECT_FALSE(response.IsPendingHeader());
    EXPECT_TRUE(response.IsPendingBody());
}

TEST(HttpWebHttpResponseTest, NonStreamResponseStillUsesCrlfTerminator)
{
    HTTPResponse response;
    const std::string header = "HTTP/1.1 200 OK\r\n\r\n";
    ASSERT_TRUE(response.ReceiveHeader(header.data(), header.size()));
    EXPECT_FALSE(response.stream_mode());
    response.ConsumeHeader();

    const char body[] = {'a', 'b', '\r', '\n', '\r', '\n'};
    EXPECT_TRUE(response.ReceiveBody(body, sizeof(body)));
}

TEST(HttpWebHttpResponseTest, ClearResetsStreamMode)
{
    HTTPResponse response;
    ASSERT_TRUE(response.ReceiveHeader(kStreamHeader.data(), kStreamHeader.size()));
    ASSERT_TRUE(response.DetectStreamMode());
    EXPECT_TRUE(response.stream_mode());

    response.Clear();
    EXPECT_FALSE(response.stream_mode());
    EXPECT_TRUE(response.IsPendingHeader());
    EXPECT_FALSE(response.IsPendingBody());
}

TEST(HttpWebHttpResponseTest, OpenEndedBodyHasNoContentLength)
{
    HTTPResponse response;
    response.SetBegin(200);
    response.SetHeader("X-DTP-Stream", "1");
    response.SetHeader("Connection", "close");
    response.SetOpenEndedBody();

    // This is exactly what goes on the wire before the first frame byte.
    EXPECT_EQ(response.cache(),
              "HTTP/1.1 200 OK\r\nX-DTP-Stream: 1\r\nConnection: close\r\n\r\n");
    EXPECT_EQ(response.body_length(), 0u);
    EXPECT_TRUE(response.DetectStreamMode());
    EXPECT_TRUE(response.stream_mode());
}
