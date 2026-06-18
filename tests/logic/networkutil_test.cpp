// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// networkutil.cpp 本身几乎全部是依赖网络/IPC/会话管理器的编排逻辑：
//   - NetworkUtil::instance() 构造时会调用 sessionListen 绑定 socket（被禁）
//   - 几乎每个 public 方法都会发起 connect / 发送 rpc / 调 IPC
// 因此无法在不触发真实网络的前提下实例化 NetworkUtil。
//
// 本测试改为覆盖 networkutil.cpp 实际依赖的"纯逻辑"层 —— 即它构造/解析报文时
// 使用的常量与消息结构体（定义于 cooconstrants.h / sessionproto.h / global_defines.h）。
// 这些结构体的 JSON 序列化/反序列化与常量取值，直接决定了 networkutil.cpp 中
// 各分支（agree ? REPLY_ACCEPT : REPLY_REJECT、nick == "BUSY_COOPERATING" 等）的行为。

#include <gtest/gtest.h>

#include "lib/cooperation/core/net/cooconstrants.h"
#include "lib/cooperation/core/global_defines.h"
#include "lib/common/manager/sessionproto.h"

#include <picojson/picojson.h>

#include <string>
#include <vector>

namespace {

// 用 picojson 把字符串解析成 value，失败返回默认 value 并由调用方自行校验。
picojson::value parseJson(const std::string &s)
{
    picojson::value v;
    picojson::parse(v, s);
    return v;
}

}   // namespace

// ---------------------------------------------------------------------------
// cooconstrants.h : 端口 / PIN / 申请类型常量
//   networkutil.cpp 中 NetworkUtilPrivate 构造时使用 COO_SESSION_PORT、COO_HARD_PIN，
//   doNextCombiRequest / msg_cb 中按 APPLY_INFO / APPLY_TRANS / APPLY_SHARE 分发。
// ---------------------------------------------------------------------------
TEST(NetworkUtilTest, SessionPortConstant)
{
    EXPECT_EQ(COO_SESSION_PORT, 51598);
}

TEST(NetworkUtilTest, HardPinConstant)
{
    EXPECT_STREQ(COO_HARD_PIN, "515616");
}

TEST(NetworkUtilTest, ApplyTypeDistinctValues)
{
    // msg_cb switch 分发依赖这些值互不相同
    EXPECT_EQ(APPLY_LOGIN, 1000);
    EXPECT_EQ(APPLY_INFO, 100);
    EXPECT_EQ(APPLY_TRANS, 101);
    EXPECT_EQ(APPLY_TRANS_RESULT, 102);
    EXPECT_EQ(APPLY_SHARE, 111);
    EXPECT_EQ(APPLY_SHARE_RESULT, 112);
    EXPECT_EQ(APPLY_SHARE_STOP, 113);
    EXPECT_EQ(APPLY_CANCELED, 120);
    EXPECT_EQ(APPLY_SCAN_CONNECT, 200);
    EXPECT_EQ(APPLY_PROJECTION, 201);
    EXPECT_EQ(APPLY_PROJECTION_RESULT, 202);
    EXPECT_EQ(APPLY_PROJECTION_STOP, 203);
}

TEST(NetworkUtilTest, ShareConnectReplyCodes)
{
    // networkutil.cpp handleCompatConnectResult / handleAsyncRpcResult 依据 result<=0 判定失败
    EXPECT_EQ(SHARE_CONNECT_UNABLE, -1);
    EXPECT_EQ(SHARE_CONNECT_REFUSE, 0);
    EXPECT_EQ(SHARE_CONNECT_COMFIRM, 1);
    EXPECT_EQ(SHARE_CONNECT_ERR_CONNECTED, 2);

    // result <= 0 视为失败（UNABLE / REFUSE），> 0 视为成功（CONFIRM）
    EXPECT_LE(SHARE_CONNECT_UNABLE, 0);
    EXPECT_LE(SHARE_CONNECT_REFUSE, 0);
    EXPECT_GT(SHARE_CONNECT_COMFIRM, 0);
}

TEST(NetworkUtilTest, ExceptionTypeValues)
{
    // handleConnectStatus: result == EX_NETWORK_PINGOUT 走 pingout 分支
    EXPECT_EQ(EX_NETWORK_PINGOUT, -3);
    EXPECT_EQ(EX_SPACE_NOTENOUGH, -2);
    EXPECT_EQ(EX_FS_RWERROR, -1);
    EXPECT_EQ(EX_OTHER, 0);
}

// ---------------------------------------------------------------------------
// sessionproto.h : apply_flag_t 枚举
//   networkutil.cpp 各处：msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
//   res.flag = DO_DONE / DO_WAIT; msg.flag = ASK_NEEDCONFIRM / ASK_QUIET;
// ---------------------------------------------------------------------------
TEST(NetworkUtilTest, ApplyFlagValues)
{
    EXPECT_EQ(ASK_NEEDCONFIRM, 10);
    EXPECT_EQ(ASK_QUIET, 12);
    EXPECT_EQ(ASK_CANCELED, 14);
    EXPECT_EQ(DO_WAIT, 20);
    EXPECT_EQ(DO_DONE, 22);
    EXPECT_EQ(REPLY_ACCEPT, 30);
    EXPECT_EQ(REPLY_REJECT, 32);

    // 语义校验：accept 与 reject 不同，且都是回复类（>=30）
    EXPECT_NE(REPLY_ACCEPT, REPLY_REJECT);
    EXPECT_GE(REPLY_ACCEPT, 30);
    EXPECT_GE(REPLY_REJECT, 30);

    // 请求类与回复类区分（networkutil 据此判断 req.flag == REPLY_ACCEPT）
    EXPECT_LT(ASK_NEEDCONFIRM, REPLY_ACCEPT);
    EXPECT_LT(DO_DONE, REPLY_ACCEPT);
}

TEST(NetworkUtilTest, FlagChoiceForAgree)
{
    // 对应 networkutil.cpp 中: msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
    auto flagFor = [](bool agree) {
        return agree ? REPLY_ACCEPT : REPLY_REJECT;
    };
    EXPECT_EQ(flagFor(true), REPLY_ACCEPT);
    EXPECT_EQ(flagFor(false), REPLY_REJECT);
}

// ---------------------------------------------------------------------------
// sessionproto.h : LoginStatus / CallResult / ComType
//   handleAsyncRpcResult 中使用 LoginStatus 语义；common 类型用于 RPC 路由。
// ---------------------------------------------------------------------------
TEST(NetworkUtilTest, LoginStatusValues)
{
    EXPECT_EQ(LOGIN_SUCCESS, 666);
    EXPECT_EQ(LOGIN_DENIED, 444);
    EXPECT_GT(LOGIN_SUCCESS, LOGIN_DENIED);

    // handleAsyncRpcResult: bool logined = !login.auth.empty();
    LoginMessage login;
    EXPECT_TRUE(login.auth.empty());
    login.auth = "token";
    EXPECT_FALSE(login.auth.empty());
}

TEST(NetworkUtilTest, CallResultValues)
{
    EXPECT_EQ(DO_SUCCESS, 0);
    EXPECT_EQ(DO_FAILED, 1);
}

TEST(NetworkUtilTest, ComTypeValues)
{
    EXPECT_EQ(REQ_LOGIN, 1000);
    EXPECT_EQ(REQ_FREE_SPACE, 1001);
    EXPECT_EQ(REQ_TRANS_DATAS, 1002);
    EXPECT_EQ(REQ_TRANS_CANCLE, 1003);
    EXPECT_EQ(CAST_INFO, 1004);
    EXPECT_EQ(INFO_TRANS_COUNT, 1005);
}

TEST(NetworkUtilTest, TcpPortConstants)
{
    EXPECT_EQ(SESSION_TCP_PORT, 51616);
    EXPECT_EQ(WEB_TCP_PORT, SESSION_TCP_PORT + 2);
}

// ---------------------------------------------------------------------------
// sessionproto.h : ApplyMessage
//   networkutil.cpp 中最高频使用的报文：reqTargetInfo / sendTransApply /
//   sendShareApply / replyTransRequest / replyShareRequestBusy / cancelApply。
// ---------------------------------------------------------------------------
TEST(ApplyMessageTest, Defaults)
{
    ApplyMessage m;
    EXPECT_EQ(m.flag, 0);
    EXPECT_EQ(m.port, 0);
    EXPECT_TRUE(m.nick.empty());
    EXPECT_TRUE(m.host.empty());
    EXPECT_TRUE(m.fingerprint.empty());
}

TEST(ApplyMessageTest, SerializeRoundTrip)
{
    ApplyMessage src;
    src.flag = ASK_NEEDCONFIRM;
    src.nick = "MyPC";
    src.host = "192.168.1.10";
    src.port = 51616;
    src.fingerprint = "AB:CD:EF";

    std::string json = src.as_json().serialize();

    // 关键字段必须出现在序列化结果中
    EXPECT_NE(json.find("\"flag\""), std::string::npos);
    EXPECT_NE(json.find("\"nick\""), std::string::npos);
    EXPECT_NE(json.find("\"selfIp\""), std::string::npos);
    EXPECT_NE(json.find("\"selfPort\""), std::string::npos);
    EXPECT_NE(json.find("\"fingerprint\""), std::string::npos);
    EXPECT_NE(json.find("192.168.1.10"), std::string::npos);

    // 反序列化应当还原回等价对象
    ApplyMessage dst;
    dst.from_json(parseJson(json));

    EXPECT_EQ(dst.flag, src.flag);
    EXPECT_EQ(dst.nick, src.nick);
    EXPECT_EQ(dst.host, src.host);
    EXPECT_EQ(dst.port, src.port);
    EXPECT_EQ(dst.fingerprint, src.fingerprint);
}

TEST(ApplyMessageTest, FieldNameMappingHostToSelfIp)
{
    // 注意：结构体成员叫 host，但 JSON 字段是 "selfIp"
    ApplyMessage m;
    m.host = "10.0.0.1";
    std::string json = m.as_json().serialize();
    EXPECT_NE(json.find("\"selfIp\":\"10.0.0.1\""), std::string::npos);
    EXPECT_EQ(json.find("\"host\""), std::string::npos);
}

TEST(ApplyMessageTest, FromJsonWithoutFingerprintDefaultsEmpty)
{
    // networkutil 收到的报文可能不含 fingerprint（旧版本/特定消息）
    std::string json = R"({"flag":30,"nick":"x","selfIp":"1.2.3.4","selfPort":51616})";
    ApplyMessage m;
    m.from_json(parseJson(json));
    EXPECT_EQ(m.flag, 30);
    EXPECT_EQ(m.nick, "x");
    EXPECT_EQ(m.host, "1.2.3.4");
    EXPECT_EQ(m.port, 51616);
    EXPECT_TRUE(m.fingerprint.empty());
}

TEST(ApplyMessageTest, BusyCooperatingMarker)
{
    // networkutil.cpp replyShareRequestBusy 构造的特殊 nick，对端据此走
    // SHARE_CONNECT_ERR_CONNECTED 分支。
    ApplyMessage m;
    m.flag = REPLY_REJECT;
    m.nick = "BUSY_COOPERATING";
    m.host = "192.168.0.1";
    m.fingerprint = "";

    std::string json = m.as_json().serialize();
    EXPECT_NE(json.find("BUSY_COOPERATING"), std::string::npos);

    ApplyMessage back;
    back.from_json(parseJson(json));
    EXPECT_EQ(back.nick, "BUSY_COOPERATING");
    EXPECT_EQ(back.flag, REPLY_REJECT);
}

TEST(ApplyMessageTest, CanceledTypeValuesRoundTrip)
{
    // cancelApply: msg.nick = type ( "share" / "transfer" )
    for (const auto &type : { std::string("share"), std::string("transfer") }) {
        ApplyMessage m;
        m.nick = type;
        std::string json = m.as_json().serialize();
        ApplyMessage back;
        back.from_json(parseJson(json));
        EXPECT_EQ(back.nick, type);
    }
}

// ---------------------------------------------------------------------------
// sessionproto.h : LoginMessage
//   handleAsyncRpcResult/APPLY_LOGIN 用 name/auth；logined = !auth.empty()
// ---------------------------------------------------------------------------
TEST(LoginMessageTest, SerializeRoundTrip)
{
    LoginMessage src;
    src.name = "user@host";
    src.auth = "secret-token";

    std::string json = src.as_json().serialize();
    EXPECT_NE(json.find("\"name\""), std::string::npos);
    EXPECT_NE(json.find("\"auth\""), std::string::npos);
    EXPECT_NE(json.find("secret-token"), std::string::npos);

    LoginMessage dst;
    dst.from_json(parseJson(json));
    EXPECT_EQ(dst.name, src.name);
    EXPECT_EQ(dst.auth, src.auth);
}

TEST(LoginMessageTest, EmptyAuthMeansNotLogined)
{
    // 对应 networkutil.cpp: bool logined = !login.auth.empty();
    LoginMessage m;
    m.name = "nobody";
    m.auth = "";
    bool logined = !m.auth.empty();
    EXPECT_FALSE(logined);

    m.auth = "token";
    logined = !m.auth.empty();
    EXPECT_TRUE(logined);
}

// ---------------------------------------------------------------------------
// sessionproto.h : FreeSpaceMessage
// ---------------------------------------------------------------------------
TEST(FreeSpaceMessageTest, SerializeRoundTrip)
{
    FreeSpaceMessage src;
    src.total = 1000000;
    src.free = 500000;

    std::string json = src.as_json().serialize();
    EXPECT_NE(json.find("\"total\""), std::string::npos);
    EXPECT_NE(json.find("\"free\""), std::string::npos);

    FreeSpaceMessage dst;
    dst.from_json(parseJson(json));
    EXPECT_EQ(dst.total, src.total);
    EXPECT_EQ(dst.free, src.free);
}

// ---------------------------------------------------------------------------
// sessionproto.h : TransDataMessage
//   注意：成员 endpoint <-> JSON "token"；names <-> JSON array
// ---------------------------------------------------------------------------
TEST(TransDataMessageTest, SerializeRoundTrip)
{
    TransDataMessage src;
    src.id = "file-svc-1";
    src.names = { "a.txt", "dir/b.txt" };
    src.endpoint = "192.168.1.5:7000:tok";
    src.flag = true;
    src.size = 4096;

    std::string json = src.as_json().serialize();
    // endpoint 序列化为 "token" 字段
    EXPECT_NE(json.find("\"token\""), std::string::npos);
    EXPECT_NE(json.find("\"names\""), std::string::npos);
    EXPECT_EQ(json.find("\"endpoint\""), std::string::npos);

    TransDataMessage dst;
    dst.from_json(parseJson(json));
    EXPECT_EQ(dst.id, src.id);
    EXPECT_EQ(dst.endpoint, src.endpoint);
    EXPECT_EQ(dst.flag, src.flag);
    EXPECT_EQ(dst.size, src.size);
    ASSERT_EQ(dst.names.size(), src.names.size());
    EXPECT_EQ(dst.names[0], src.names[0]);
    EXPECT_EQ(dst.names[1], src.names[1]);
}

TEST(TransDataMessageTest, EmptyNamesRoundTrip)
{
    TransDataMessage src;
    src.id = "x";
    src.endpoint = "ep";
    src.flag = false;
    src.size = 0;

    std::string json = src.as_json().serialize();
    TransDataMessage dst;
    dst.from_json(parseJson(json));
    EXPECT_TRUE(dst.names.empty());
    EXPECT_EQ(dst.size, 0);
    EXPECT_FALSE(dst.flag);
}

// ---------------------------------------------------------------------------
// sessionproto.h : TransCancelMessage
// ---------------------------------------------------------------------------
TEST(TransCancelMessageTest, SerializeRoundTrip)
{
    TransCancelMessage src;
    src.id = "svc-9";
    src.name = "movie.mp4";
    src.reason = "user";

    std::string json = src.as_json().serialize();
    EXPECT_NE(json.find("\"id\""), std::string::npos);
    EXPECT_NE(json.find("\"name\""), std::string::npos);
    EXPECT_NE(json.find("\"reason\""), std::string::npos);

    TransCancelMessage dst;
    dst.from_json(parseJson(json));
    EXPECT_EQ(dst.id, src.id);
    EXPECT_EQ(dst.name, src.name);
    EXPECT_EQ(dst.reason, src.reason);
}

// ---------------------------------------------------------------------------
// sessionproto.h : MyInfoMessage
//   msg_cb APPLY_INFO 回复时用 res.nick = deviceInfoStr()；MyInfoMessage 描述对端设备。
// ---------------------------------------------------------------------------
TEST(MyInfoMessageTest, SerializeRoundTrip)
{
    MyInfoMessage src;
    src.nickname = "Mike";
    src.username = "mike";
    src.hostname = "mike-pc";
    src.ipv4 = "172.16.0.2";
    src.port = "51616";

    std::string json = src.as_json().serialize();
    EXPECT_NE(json.find("\"nickname\""), std::string::npos);
    EXPECT_NE(json.find("\"username\""), std::string::npos);
    EXPECT_NE(json.find("\"hostname\""), std::string::npos);
    EXPECT_NE(json.find("\"ipv4\""), std::string::npos);
    EXPECT_NE(json.find("\"port\""), std::string::npos);

    MyInfoMessage dst;
    dst.from_json(parseJson(json));
    EXPECT_EQ(dst.nickname, src.nickname);
    EXPECT_EQ(dst.username, src.username);
    EXPECT_EQ(dst.hostname, src.hostname);
    EXPECT_EQ(dst.ipv4, src.ipv4);
    EXPECT_EQ(dst.port, src.port);
}

// ---------------------------------------------------------------------------
// global_defines.h : AppSettings 常量（networkutil.cpp 多处引用）
// ---------------------------------------------------------------------------
TEST(NetworkUtilTest, AppSettingsKeysAreStable)
{
    // sendTransApply / replyTransRequest 通过 AppSettings::DeviceNameKey 取设备名
    EXPECT_STREQ(AppSettings::DeviceNameKey, "DeviceName");
    EXPECT_STREQ(AppSettings::IPAddress, "IPAddress");
    EXPECT_STREQ(AppSettings::StoragePathKey, "StoragePath");
    EXPECT_STREQ(AppSettings::GenericGroup, "GenericAttribute");
    EXPECT_STREQ(AppSettings::CacheGroup, "Cache");
    EXPECT_STREQ(AppSettings::TransHistoryKey, "TransHistory");
    EXPECT_STREQ(AppSettings::ConnectHistoryKey, "ConnectHistory");
    EXPECT_STREQ(AppSettings::CloseOptionKey, "CloseOption");
    EXPECT_STREQ(AppSettings::CooperationEnabled, "CooperationEnabled");
}

TEST(NetworkUtilTest, DConfigKeysAreStable)
{
    EXPECT_STREQ(DConfigKey::DiscoveryModeKey, "cooperation.discovery.mode");
    EXPECT_STREQ(DConfigKey::TransferModeKey, "cooperation.transfer.mode");
}

TEST(NetworkUtilTest, MainAppNameConstant)
{
    EXPECT_STREQ(MainAppName, "dde-cooperation");
}

TEST(NetworkUtilTest, OperationKeysAreStable)
{
    EXPECT_STREQ(OperationKey::kID, "id");
    EXPECT_STREQ(OperationKey::kIconName, "icon-name");
    EXPECT_STREQ(OperationKey::kButtonStyle, "button-style");
    EXPECT_STREQ(OperationKey::kLocation, "location");
    EXPECT_STREQ(OperationKey::kDescription, "description");
}

TEST(NetworkUtilTest, ReportAttributeKeysAreStable)
{
    EXPECT_STREQ(ReportAttribute::CooperationStatus, "CooperationStatus");
    EXPECT_STREQ(ReportAttribute::FileDelivery, "FileDelivery");
    EXPECT_STREQ(ReportAttribute::ConnectionInfo, "ConnectionInfo");
}

TEST(NetworkUtilTest, MenuAndCooperationModeEnums)
{
    EXPECT_EQ(kPC, 0);
    EXPECT_EQ(kMobile, 1);
    EXPECT_NE(kSettings, kDownloadWindowClient);
    EXPECT_NE(kDownloadWindowClient, kDownloadMobileClient);
}

// ---------------------------------------------------------------------------
// networkutil.cpp replyShareRequestBusy 用空 fingerprint；校验序列化保持空串
// ---------------------------------------------------------------------------
TEST(ApplyMessageTest, EmptyFingerprintSerialized)
{
    ApplyMessage m;
    m.flag = REPLY_REJECT;
    m.nick = "BUSY_COOPERATING";
    m.host = "1.1.1.1";
    m.fingerprint = "";
    std::string json = m.as_json().serialize();
    EXPECT_NE(json.find("\"fingerprint\":\"\""), std::string::npos);
}

// ---------------------------------------------------------------------------
// networkutil.cpp doNextCombiRequest：根据 _nextCombi.first 分发；该值为 0 直接 return。
// 这里间接验证分发所用的常量值域正确。
// ---------------------------------------------------------------------------
TEST(NetworkUtilTest, NextCombiDispatchTargetsAreDistinct)
{
    // 三种合法下一跳类型必须互不相同
    EXPECT_NE(APPLY_INFO, APPLY_TRANS);
    EXPECT_NE(APPLY_TRANS, APPLY_SHARE);
    EXPECT_NE(APPLY_INFO, APPLY_SHARE);
    // 且都 > 0（type<=0 时不分发）
    EXPECT_GT(APPLY_INFO, 0);
    EXPECT_GT(APPLY_TRANS, 0);
    EXPECT_GT(APPLY_SHARE, 0);
}
