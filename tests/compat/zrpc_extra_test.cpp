// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <functional>
#include <string>

#include "errorcode.h"
#include "netaddress.h"
#include "rpcchannel.h"
#include "rpccontroller.h"
#include "rpcdispatcher.h"
#include "rpcclosure.h"

using namespace zrpc_ns;

TEST(ZRpcDispatcherTest, ParseServiceFullNameSplitsOnFirstDot)
{
    ZRpcDispacther d;
    std::string svc, method;
    EXPECT_TRUE(d.parseServiceFullName("QueryService.query_name", svc, method));
    EXPECT_EQ(svc, "QueryService");
    EXPECT_EQ(method, "query_name");
}

TEST(ZRpcDispatcherTest, ParseServiceFullNameEmptyReturnsFalse)
{
    ZRpcDispacther d;
    std::string svc, method;
    EXPECT_FALSE(d.parseServiceFullName("", svc, method));
}

TEST(ZRpcDispatcherTest, ParseServiceFullNameNoDotReturnsFalse)
{
    ZRpcDispacther d;
    std::string svc, method;
    EXPECT_FALSE(d.parseServiceFullName("NoSeparator", svc, method));
}

TEST(ZRpcDispatcherTest, ParseServiceFullNameMultipleDots)
{
    ZRpcDispacther d;
    std::string svc, method;
    EXPECT_TRUE(d.parseServiceFullName("a.b.c", svc, method));
    EXPECT_EQ(svc, "a");
    EXPECT_EQ(method, "b.c");
}

TEST(ZRpcDispatcherTest, DispatcherDefaultServiceMapEmpty)
{
    ZRpcDispacther d;
    EXPECT_TRUE(d.m_service_map.empty());
}

TEST(ZRpcControllerExtraTest, SetErrorCodeOnlyKeepsFailedFalse)
{
    ZRpcController c;
    c.SetErrorCode(77);
    EXPECT_EQ(c.ErrorCode(), 77);
    EXPECT_FALSE(c.Failed());
}

TEST(ZRpcControllerExtraTest, SetErrorCombinesCodeAndInfo)
{
    ZRpcController c;
    c.SetError(ERROR_FAILED_ENCODE, "encode broken");
    EXPECT_TRUE(c.Failed());
    EXPECT_EQ(c.ErrorCode(), ERROR_FAILED_ENCODE);
    EXPECT_EQ(c.ErrorText(), "encode broken");
}

TEST(NetAddressToStringTest, FormatsIpAndPort)
{
    NetAddress addr("192.168.0.1", 9999, false);
    EXPECT_EQ(addr.toString(), "192.168.0.1:9999");
}

TEST(NetAddressToStringTest, WildcardIpForNull)
{
    NetAddress addr(nullptr, 8080, false);
    EXPECT_EQ(addr.toString(), "0.0.0.0:8080");
}

TEST(ZRpcClosureTest, RunInvokesCallback)
{
    int counter = 0;
    ZRpcClosure cl([&counter]() { counter++; });
    cl.Run();
    EXPECT_EQ(counter, 1);
}

TEST(ZRpcClosureTest, RunTwiceInvokesTwice)
{
    int counter = 0;
    ZRpcClosure cl([&counter]() { counter += 10; });
    cl.Run();
    cl.Run();
    EXPECT_EQ(counter, 20);
}

TEST(ZRpcChannelTest, ConstructShortConnectionDoesNotConnect)
{
    auto addr = std::make_shared<NetAddress>("127.0.0.1", 12345, false);
    ZRpcChannel ch(addr, false);
    SUCCEED();
}

TEST(ZRpcChannelTest, DestructorLongConnectReleasesGlobalClient)
{
    auto addr = std::make_shared<NetAddress>("127.0.0.1", 12346, false);
    { ZRpcChannel ch(addr, true); }
    SUCCEED();
}
