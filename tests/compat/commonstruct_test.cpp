// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "co/json.h"
#include "commonstruct.h"

#include <string>

namespace {

template <typename T>
T roundtrip(const T& src)
{
    co::Json j = src.as_json();
    fastring dumped = j.str();
    co::Json parsed;
    parsed.parse_from(dumped);
    T dst;
    dst.from_json(parsed);
    return dst;
}

}

TEST(CommonStructTest, SendResultRoundtrip)
{
    SendResult src;
    src.protocolType = 7u;
    src.errorType = -3;
    src.data = "payload";
    auto dst = roundtrip(src);
    EXPECT_EQ(dst.protocolType, 7u);
    EXPECT_EQ(dst.errorType, -3);
    EXPECT_STREQ(dst.data.c_str(), "payload");
}

TEST(CommonStructTest, UserLoginInfoRoundtrip)
{
    UserLoginInfo src;
    src.name = "alice";
    src.auth = "tok";
    src.my_uid = "1001";
    src.my_name = "me";
    src.session_id = "sess";
    src.selfappName = "coop";
    src.appName = "dde";
    src.version = "1.0";
    src.ip = "10.0.0.1";
    auto dst = roundtrip(src);
    EXPECT_STREQ(dst.name.c_str(), "alice");
    EXPECT_STREQ(dst.auth.c_str(), "tok");
    EXPECT_STREQ(dst.ip.c_str(), "10.0.0.1");
    EXPECT_STREQ(dst.version.c_str(), "1.0");
}

TEST(CommonStructTest, PeerInfoRoundtrip)
{
    PeerInfo src;
    src.username = "bob";
    src.hostname = "host";
    src.platform = "linux";
    src.version = "2.0";
    src.privacy_mode = true;
    auto dst = roundtrip(src);
    EXPECT_STREQ(dst.username.c_str(), "bob");
    EXPECT_TRUE(dst.privacy_mode);
}

TEST(CommonStructTest, UserLoginResultInfoNestedRoundtrip)
{
    UserLoginResultInfo src;
    src.peer.username = "carol";
    src.peer.privacy_mode = false;
    src.token = "tkn";
    src.appName = "app";
    src.result = true;
    auto dst = roundtrip(src);
    EXPECT_STREQ(dst.peer.username.c_str(), "carol");
    EXPECT_FALSE(dst.peer.privacy_mode);
    EXPECT_STREQ(dst.token.c_str(), "tkn");
    EXPECT_TRUE(dst.result);
}

TEST(CommonStructTest, FileEntryRoundtrip)
{
    FileEntry src;
    src.filetype = 1;
    src.name = "a.txt";
    src.hidden = true;
    src.size = 9999;
    src.modified_time = 123456;
    src.appName = "x";
    src.rcvappName = "y";
    auto dst = roundtrip(src);
    EXPECT_EQ(src.filetype, dst.filetype);
    EXPECT_EQ(src.size, dst.size);
    EXPECT_TRUE(dst.hidden);
    EXPECT_STREQ(dst.name.c_str(), "a.txt");
}

TEST(CommonStructTest, FileTransBlockRoundtrip)
{
    FileTransBlock src;
    src.job_id = 5;
    src.file_id = 6;
    src.rootdir = "/r";
    src.filename = "f";
    src.blk_id = 11u;
    src.flags = 2;
    src.data_size = 4096;
    auto dst = roundtrip(src);
    EXPECT_EQ(dst.job_id, 5);
    EXPECT_EQ(dst.blk_id, 11u);
    EXPECT_EQ(dst.data_size, 4096);
    EXPECT_STREQ(dst.rootdir.c_str(), "/r");
}

TEST(CommonStructTest, ShareEventsRoundtrip)
{
    ShareEvents src;
    src.eventType = 42u;
    src.data = "blob";
    auto dst = roundtrip(src);
    EXPECT_EQ(dst.eventType, 42u);
    EXPECT_STREQ(dst.data.c_str(), "blob");
}

TEST(CommonStructTest, PingPongRoundtrip)
{
    PingPong src;
    src.appName = "a";
    src.tarAppname = "b";
    src.ip = "1.2.3.4";
    auto dst = roundtrip(src);
    EXPECT_STREQ(dst.appName.c_str(), "a");
    EXPECT_STREQ(dst.tarAppname.c_str(), "b");
    EXPECT_STREQ(dst.ip.c_str(), "1.2.3.4");
}

TEST(CommonStructTest, DiscoverInfoRoundtrip)
{
    DiscoverInfo src;
    src.ip = "9.9.9.9";
    src.msg = "hi";
    auto dst = roundtrip(src);
    EXPECT_STREQ(dst.ip.c_str(), "9.9.9.9");
    EXPECT_STREQ(dst.msg.c_str(), "hi");
}

TEST(CommonStructTest, MiscInfoRoundtrip)
{
    MiscInfo src;
    src.appName = "dde-cooperation";
    src.json = "{\"k\":1}";
    auto dst = roundtrip(src);
    EXPECT_STREQ(dst.appName.c_str(), "dde-cooperation");
    EXPECT_STREQ(dst.json.c_str(), "{\"k\":1}");
}

TEST(CommonStructTest, FileTransResponseDefaultsAndRoundtrip)
{
    FileTransResponse src;
    EXPECT_EQ(src.id, -1);
    EXPECT_EQ(src.result, -1);
    src.id = 99;
    src.name = "resp";
    src.result = 0;
    auto dst = roundtrip(src);
    EXPECT_EQ(dst.id, 99);
    EXPECT_EQ(dst.result, 0);
    EXPECT_STREQ(dst.name.c_str(), "resp");
}

TEST(CommonStructTest, FileTransCreateNestedEntryRoundtrip)
{
    FileTransCreate src;
    src.job_id = 3;
    src.file_id = 4;
    src.sub_dir = "sub";
    src.entry.name = "inner.txt";
    src.entry.size = 555;
    auto dst = roundtrip(src);
    EXPECT_EQ(dst.job_id, 3);
    EXPECT_EQ(dst.file_id, 4);
    EXPECT_STREQ(dst.sub_dir.c_str(), "sub");
    EXPECT_STREQ(dst.entry.name.c_str(), "inner.txt");
    EXPECT_EQ(dst.entry.size, 555);
}

TEST(CommonStructTest, AsJsonProducesValidJsonString)
{
    ShareConnectApply src;
    src.appName = "a";
    src.tarAppname = "b";
    src.ip = "1.1.1.1";
    src.tarIp = "2.2.2.2";
    src.data = "d";
    fastring s = src.as_json().str();
    EXPECT_NE(s.find("appName"), std::string::npos);
    EXPECT_NE(s.find("tarIp"), std::string::npos);
}
