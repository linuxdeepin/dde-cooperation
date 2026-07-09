// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// compatstruct.h 全结构 from_json/as_json round-trip 覆盖
// 纯 POD + picojson 序列化, 无任何外部依赖。

#include "lib/cooperation/core/net/compatstruct.h"

#include <gtest/gtest.h>
#include <string>

using namespace ipc;

namespace {
// 辅助: 把结构体 as_json 后再 parse 回来, 模拟传输往返
template <typename T>
T roundtrip(const T &orig)
{
    std::string s = orig.as_json().serialize();
    picojson::value v;
    picojson::parse(v, s);
    T back;
    back.from_json(v);
    return back;
}
}   // namespace

TEST(CompatStruct, GenericResultRoundtrip)
{
    GenericResult o;
    o.id = 7; o.result = 1; o.msg = "ok"; o.isself = true;
    auto r = roundtrip(o);
    EXPECT_EQ(r.id, 7);
    EXPECT_EQ(r.result, 1);
    EXPECT_EQ(r.msg, "ok");
    EXPECT_TRUE(r.isself);
}

TEST(CompatStruct, NodePeerInfoRoundtrip)
{
    NodePeerInfo o;
    o.proto_version = "1.0"; o.uuid = "u1"; o.nickname = "n1";
    o.username = "usr"; o.hostname = "host"; o.ipv4 = "10.0.0.1";
    o.share_connect_ip = "10.0.0.2"; o.port = 8080; o.os_type = 2; o.mode_type = 3;
    auto r = roundtrip(o);
    EXPECT_EQ(r.uuid, "u1");
    EXPECT_EQ(r.ipv4, "10.0.0.1");
    EXPECT_EQ(r.port, 8080);
    EXPECT_EQ(r.os_type, 2);
    EXPECT_EQ(r.mode_type, 3);
}

TEST(CompatStruct, AppPeerInfoRoundtrip)
{
    AppPeerInfo o;
    o.appname = "coop"; o.json = "{\"k\":1}";
    auto r = roundtrip(o);
    EXPECT_EQ(r.appname, "coop");
    EXPECT_EQ(r.json, "{\"k\":1}");
}

TEST(CompatStruct, NodeInfoWithAppsRoundtrip)
{
    NodeInfo o;
    o.os.uuid = "os-uuid";
    o.os.port = 5;
    AppPeerInfo a; a.appname = "app1"; a.json = "j1";
    AppPeerInfo b; b.appname = "app2"; b.json = "j2";
    o.apps.push_back(a);
    o.apps.push_back(b);
    auto r = roundtrip(o);
    EXPECT_EQ(r.os.uuid, "os-uuid");
    EXPECT_EQ(r.apps.size(), 2u);
    EXPECT_EQ(r.apps[1].appname, "app2");
}

// from_json: apps 字段非 array 时跳过 (覆盖 is<picojson::array> false 分支)
TEST(CompatStruct, NodeInfoFromJsonAppsNotArray)
{
    picojson::value v;
    picojson::parse(v, R"({"os":{"proto_version":"v","uuid":"u","nickname":"n","username":"un","hostname":"h","ipv4":"1.1.1.1","share_connect_ip":"2.2.2.2","port":1,"os_type":1,"mode_type":1},"apps":"not-an-array"})");
    NodeInfo n;
    n.from_json(v);   // apps 非 array → 不填, 不崩
    EXPECT_TRUE(n.apps.empty());
    EXPECT_EQ(n.os.uuid, "u");
}

TEST(CompatStruct, NodeListRoundtrip)
{
    NodeList o;
    o.code = 42;
    NodeInfo n; n.os.uuid = "p1";
    o.peers.push_back(n);
    auto r = roundtrip(o);
    EXPECT_EQ(r.code, 42);
    EXPECT_EQ(r.peers.size(), 1u);
    EXPECT_EQ(r.peers[0].os.uuid, "p1");
}

TEST(CompatStruct, MiscJsonCallRoundtrip)
{
    MiscJsonCall o; o.app = "a"; o.json = "j";
    auto r = roundtrip(o);
    EXPECT_EQ(r.app, "a");
    EXPECT_EQ(r.json, "j");
}

TEST(CompatStruct, ApplyTransFilesRoundtrip)
{
    ApplyTransFiles o;
    o.machineName = "m"; o.appname = "a"; o.tarAppname = "t";
    o.type = 1; o.selfIp = "1.1.1.1"; o.selfPort = 9000;
    auto r = roundtrip(o);
    EXPECT_EQ(r.machineName, "m");
    EXPECT_EQ(r.type, 1);
    EXPECT_EQ(r.selfPort, 9000);
}

TEST(CompatStruct, ShareEventsRoundtrip)
{
    ShareEvents o; o.eventType = 109u; o.data = "payload";
    auto r = roundtrip(o);
    EXPECT_EQ(r.eventType, 109u);
    EXPECT_EQ(r.data, "payload");
}

TEST(CompatStruct, ShareConnectApplyRoundtrip)
{
    ShareConnectApply o;
    o.appName = "a"; o.tarAppname = "t"; o.ip = "1.1.1.1";
    o.tarIp = "2.2.2.2"; o.data = "d";
    auto r = roundtrip(o);
    EXPECT_EQ(r.ip, "1.1.1.1");
    EXPECT_EQ(r.tarIp, "2.2.2.2");
}

TEST(CompatStruct, ShareConnectReplyRoundtrip)
{
    ShareConnectReply o;
    o.appName = "a"; o.tarAppname = "t"; o.msg = "m";
    o.ip = "1.1.1.1"; o.reply = 1;
    auto r = roundtrip(o);
    EXPECT_EQ(r.reply, 1);
    EXPECT_EQ(r.msg, "m");
}

TEST(CompatStruct, ShareDisConnectRoundtrip)
{
    ShareDisConnect o; o.appName = "a"; o.tarAppname = "t"; o.msg = "bye";
    auto r = roundtrip(o);
    EXPECT_EQ(r.appName, "a");
    EXPECT_EQ(r.msg, "bye");
}

TEST(CompatStruct, ShareConnectDisApplyRoundtrip)
{
    ShareConnectDisApply o;
    o.appName = "a"; o.tarAppname = "t"; o.ip = "1.1.1.1"; o.msg = "x";
    auto r = roundtrip(o);
    EXPECT_EQ(r.ip, "1.1.1.1");
    EXPECT_EQ(r.msg, "x");
}

TEST(CompatStruct, SendStatusRoundtrip)
{
    SendStatus o;
    o.type = 2; o.status = 3; o.curstatus = 1; o.msg = "running";
    auto r = roundtrip(o);
    EXPECT_EQ(r.type, 2);
    EXPECT_EQ(r.status, 3);
    EXPECT_EQ(r.curstatus, 1);
    EXPECT_EQ(r.msg, "running");
}

TEST(CompatStruct, SendResultRoundtrip)
{
    SendResult o;
    o.protocolType = 100LL; o.errorType = -1LL; o.data = "err";
    auto r = roundtrip(o);
    EXPECT_EQ(r.protocolType, 100LL);
    EXPECT_EQ(r.errorType, -1LL);
    EXPECT_EQ(r.data, "err");
}

TEST(CompatStruct, FileStatusRoundtrip)
{
    FileStatus o;
    o.job_id = 1; o.file_id = 2; o.name = "f.txt";
    o.status = 4; o.total = 1000; o.current = 500; o.millisec = 12345;
    auto r = roundtrip(o);
    EXPECT_EQ(r.job_id, 1);
    EXPECT_EQ(r.name, "f.txt");
    EXPECT_EQ(r.total, 1000);
    EXPECT_EQ(r.current, 500);
}

TEST(CompatStruct, SearchDeviceRoundtrip)
{
    SearchDevice o; o.app = "coop"; o.ip = "1.1.1.1"; o.remove = true;
    auto r = roundtrip(o);
    EXPECT_EQ(r.app, "coop");
    EXPECT_EQ(r.ip, "1.1.1.1");
    EXPECT_TRUE(r.remove);
}

TEST(CompatStruct, SearchDeviceResultRoundtrip)
{
    SearchDeviceResult o; o.result = false; o.msg = "not found";
    auto r = roundtrip(o);
    EXPECT_FALSE(r.result);
    EXPECT_EQ(r.msg, "not found");
}
