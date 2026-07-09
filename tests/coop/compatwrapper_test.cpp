// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// CompatWrapper 单元测试: 单例身份 + ipcCompatSlot 全 switch 分支。
// ipcCompatSlot 内通过 invokeMethod(QueuedConnection) 投递的事件不主动处理,
// 测试全程不调用事件循环 -> 投递的事件在 QApplication 析构时被丢弃, 安全。

#include <gtest/gtest.h>
#include <QString>
#include <QCoreApplication>

#include "lib/cooperation/core/net/compatwrapper.h"
#include "lib/cooperation/core/net/compatwrapper_p.h"
#include "lib/cooperation/core/net/compatstruct.h"

using cooperation_core::CompatWrapper;
using cooperation_core::CompatWrapperPrivate;
namespace ipc = ::ipc;

// 工具: 把 picojson::value 序列化成 QString (供 ipcCompatSlot 入参)
static QString toJsonStr(const picojson::value &v)
{
    return QString::fromStdString(v.serialize());
}

// 构造一个带 app 的 NodeInfo JSON (FRONT_PEER_CB 用)
static QString makeNodeInfoJson(bool hasApp, bool find)
{
    ipc::NodeInfo ni;
    ni.os.ipv4 = "10.0.0.50";
    ni.os.share_connect_ip = "10.0.0.51";
    if (hasApp) {
        ipc::AppPeerInfo app;
        app.appname = ipc::CooperRegisterName;
        app.json = "host-display";
        ni.apps.push_back(app);
    }
    picojson::value nodeVal = ni.as_json();

    ipc::GenericResult gr;
    gr.result = find ? 1 : 0;
    gr.msg = nodeVal.serialize();
    return toJsonStr(gr.as_json());
}

// ===== 单例身份 =====

TEST(CompatWrapperTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(CompatWrapper::instance(), CompatWrapper::instance());
}

TEST(CompatWrapperTest, SessionIsEmptyBeforeBackendConnect)
{
    EXPECT_EQ(CompatWrapper::instance()->session().toStdString(), "");
}

TEST(CompatWrapperTest, IpcInterfaceReturnsNonNull)
{
    EXPECT_NE(CompatWrapper::instance()->ipcInterface(), nullptr);
}

TEST(CompatWrapperTest, IpcInterfaceReturnsSamePointer)
{
    auto *a = CompatWrapper::instance()->ipcInterface();
    auto *b = CompatWrapper::instance()->ipcInterface();
    EXPECT_EQ(a, b);
}

// ===== ipcCompatSlot: 无效 JSON 早返回 =====

TEST(CompatWrapperIpcCompatSlot, InvalidJsonEarlyReturn)
{
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_PEER_CB, QString("{not-json")));
}

// ===== IPC_PING (10) =====

TEST(CompatWrapperIpcCompatSlot, IpcPingNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::IPC_PING, QString("{}")));
}

// ===== FRONT_PEER_CB (100): find=true + 有 app -> addDeivces =====

TEST(CompatWrapperIpcCompatSlot, FrontPeerCbWithAppAndFind)
{
    QString msg = makeNodeInfoJson(true /*hasApp*/, true /*find*/);
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_PEER_CB, msg));
}

// ===== FRONT_PEER_CB (100): find=false + 无 app -> removeDeivce =====

TEST(CompatWrapperIpcCompatSlot, FrontPeerCbNoAppAndNotFind)
{
    QString msg = makeNodeInfoJson(false /*hasApp*/, false /*find*/);
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_PEER_CB, msg));
}

// ===== FRONT_PEER_CB (100): 内层 msg 解析失败 (GenericResult.msg 非合法 JSON) =====

TEST(CompatWrapperIpcCompatSlot, FrontPeerCbInnerMsgBadJson)
{
    ipc::GenericResult gr;
    gr.result = 1;
    gr.msg = "{not-json";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_PEER_CB, toJsonStr(gr.as_json())));
}

// ===== FRONT_CONNECT_CB (101) =====

TEST(CompatWrapperIpcCompatSlot, FrontConnectCb)
{
    ipc::GenericResult gr;
    gr.result = 1;
    gr.msg = "10.0.0.60 myhost";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_CONNECT_CB, toJsonStr(gr.as_json())));
}

// ===== FRONT_TRANS_STATUS_CB (102) =====

TEST(CompatWrapperIpcCompatSlot, FrontTransStatusCb)
{
    ipc::GenericResult gr;
    gr.id = 1;
    gr.result = 0;
    gr.msg = "/tmp/job";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_TRANS_STATUS_CB, toJsonStr(gr.as_json())));
}

// ===== FRONT_NOTIFY_FILE_STATUS (105) =====

TEST(CompatWrapperIpcCompatSlot, FrontNotifyFileStatus)
{
    ipc::FileStatus fs;
    fs.total = 100;
    fs.current = 50;
    fs.millisec = 10;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_NOTIFY_FILE_STATUS, toJsonStr(fs.as_json())));
}

// ===== FRONT_APPLY_TRANS_FILE (106): APPLY_TRANS_APPLY 分支 =====

TEST(CompatWrapperIpcCompatSlot, FrontApplyTransFileApplyType)
{
    ipc::ApplyTransFiles atf;
    atf.type = ipc::APPLY_TRANS_APPLY;
    atf.selfIp = "10.0.0.70";
    atf.machineName = "host70";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_APPLY_TRANS_FILE, toJsonStr(atf.as_json())));
}

// ===== FRONT_APPLY_TRANS_FILE (106): APPLY_TRANS_CONFIRM -> accepted =====

TEST(CompatWrapperIpcCompatSlot, FrontApplyTransFileConfirmType)
{
    ipc::ApplyTransFiles atf;
    atf.type = ipc::APPLY_TRANS_CONFIRM;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_APPLY_TRANS_FILE, toJsonStr(atf.as_json())));
}

// ===== FRONT_APPLY_TRANS_FILE (106): APPLY_TRANS_REFUSED -> rejected =====

TEST(CompatWrapperIpcCompatSlot, FrontApplyTransFileRefusedType)
{
    ipc::ApplyTransFiles atf;
    atf.type = ipc::APPLY_TRANS_REFUSED;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_APPLY_TRANS_FILE, toJsonStr(atf.as_json())));
}

// ===== FRONT_SERVER_ONLINE (108) =====

TEST(CompatWrapperIpcCompatSlot, FrontServerOnlineNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SERVER_ONLINE, QString("{}")));
}

// ===== FRONT_SHARE_APPLY_CONNECT (109) =====

TEST(CompatWrapperIpcCompatSlot, FrontShareApplyConnect)
{
    ipc::ShareConnectApply sca;
    sca.data = "zerowf,10.0.0.80";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SHARE_APPLY_CONNECT, toJsonStr(sca.as_json())));
}

// ===== FRONT_SHARE_APPLY_CONNECT_REPLY (110) =====

TEST(CompatWrapperIpcCompatSlot, FrontShareApplyConnectReply)
{
    ipc::ShareConnectReply scr;
    scr.reply = 1;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SHARE_APPLY_CONNECT_REPLY, toJsonStr(scr.as_json())));
}

// ===== FRONT_SHARE_DISCONNECT (111) =====

TEST(CompatWrapperIpcCompatSlot, FrontShareDisconnect)
{
    ipc::ShareDisConnect sdc;
    sdc.msg = "disconnected";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SHARE_DISCONNECT, toJsonStr(sdc.as_json())));
}

// ===== FRONT_SHARE_DISAPPLY_CONNECT (115) =====

TEST(CompatWrapperIpcCompatSlot, FrontShareDisapplyConnect)
{
    ipc::ShareConnectDisApply scda;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SHARE_DISAPPLY_CONNECT, toJsonStr(scda.as_json())));
}

// ===== FRONT_SEND_STATUS (107): status<0 + type=999 -> emit compatConnectResult =====

TEST(CompatWrapperIpcCompatSlot, FrontSendStatusLoginInfo)
{
    ipc::SendStatus ss;
    ss.status = -1;
    ss.type = 999;
    ipc::SendResult sr;
    sr.data = "10.0.0.90";
    picojson::value srVal = sr.as_json();
    ss.msg = srVal.serialize();
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SEND_STATUS, toJsonStr(ss.as_json())));
}

// ===== FRONT_SEND_STATUS (107): status<0 + msg 非合法 JSON -> 早返回 =====

TEST(CompatWrapperIpcCompatSlot, FrontSendStatusBadMsgJson)
{
    ipc::SendStatus ss;
    ss.status = -1;
    ss.type = 999;
    ss.msg = "{bad-json";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SEND_STATUS, toJsonStr(ss.as_json())));
}

// ===== FRONT_SEND_STATUS (107): status>=0 -> 不进入分支 =====

TEST(CompatWrapperIpcCompatSlot, FrontSendStatusPositiveNoBranch)
{
    ipc::SendStatus ss;
    ss.status = 1;
    ss.type = 1;
    ss.msg = "";
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SEND_STATUS, toJsonStr(ss.as_json())));
}

// ===== FRONT_SEARCH_IP_DEVICE_RESULT (116): result=true =====

TEST(CompatWrapperIpcCompatSlot, FrontSearchIpDeviceResultTrue)
{
    ipc::SearchDeviceResult sdr;
    sdr.result = true;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SEARCH_IP_DEVICE_RESULT, toJsonStr(sdr.as_json())));
}

// ===== FRONT_SEARCH_IP_DEVICE_RESULT (116): result=false =====

TEST(CompatWrapperIpcCompatSlot, FrontSearchIpDeviceResultFalse)
{
    ipc::SearchDeviceResult sdr;
    sdr.result = false;
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(ipc::FRONT_SEARCH_IP_DEVICE_RESULT, toJsonStr(sdr.as_json())));
}

// ===== default 分支: 未知 type =====

TEST(CompatWrapperIpcCompatSlot, UnknownTypeDefaultBranch)
{
    EXPECT_NO_FATAL_FAILURE(
        CompatWrapper::instance()->d->ipcCompatSlot(99999, QString("{}")));
}
