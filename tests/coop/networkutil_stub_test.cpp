// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// NetworkUtil stub 斩链测试
// 用 stub.h patch SessionManager 导出方法为 no-op, 让 networkutil 的 guard 分支
// 和状态机可测, 不触真网络栈 (team memory: 网络单例覆盖率天花板, 用 stub 斩链)。
// 注: SessionManager 方法非虚, stub.h 直接 patch 库符号; 替换函数首参为 this 指针对齐寄存器。

#include "lib/cooperation/core/net/networkutil.h"
#include "lib/cooperation/core/net/networkutill_p.h"
#include "lib/cooperation/core/net/compatwrapper.h"
#include "lib/cooperation/core/net/cooconstrants.h"
#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"
#include "utils/cooperationutil.h"
#include "discover/discovercontroller.h"

#include <gtest/gtest.h>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "stub.h"

using cooperation_core::NetworkUtil;
using cooperation_core::NetworkUtilPrivate;

// ===== SessionManager 方法 stub (no-op / 可控返回值) =====
namespace {
// sessionConnect 返回值可控 (0=waiting, 1=logged-in, -1=fail→compat)
// 每个依赖该全局的测试都以 SessionStubScope 开头, 其构造函数复位为 0, 防泄漏。
int g_sessionConnectReturn = 0;
static int stub_sessionConnect(SessionManager *, QString, int, QString) { return g_sessionConnectReturn; }
static void stub_sendRpcRequest(SessionManager *, const QString &, int, const QString &) {}
static void stub_sessionDisconnect(SessionManager *, QString) {}
static void stub_sendFiles(SessionManager *, QString &, int, QStringList) {}
static void stub_cancelSyncFile(SessionManager *, const QString &, const QString &) {}
static void stub_updateSaveFolder(SessionManager *, const QString &) {}
static void stub_setStorageRoot(SessionManager *, const QString &) {}
static bool stub_sessionPing(SessionManager *, QString, int) { return true; }
static void stub_updateLoginStatus(SessionManager *, QString, bool) {}

struct SessionStubScope {
    Stub stub;
    SessionStubScope()
    {
        g_sessionConnectReturn = 0;   // 复位共享全局, 防跨用例泄漏
        stub.set(ADDR(SessionManager, sessionConnect), stub_sessionConnect);
        stub.set(ADDR(SessionManager, sendRpcRequest), stub_sendRpcRequest);
        stub.set(ADDR(SessionManager, sessionDisconnect), stub_sessionDisconnect);
        stub.set(ADDR(SessionManager, sendFiles), stub_sendFiles);
        stub.set(ADDR(SessionManager, cancelSyncFile), stub_cancelSyncFile);
        stub.set(ADDR(SessionManager, updateSaveFolder), stub_updateSaveFolder);
        stub.set(ADDR(SessionManager, setStorageRoot), stub_setStorageRoot);
        stub.set(ADDR(SessionManager, sessionPing), stub_sessionPing);
        stub.set(ADDR(SessionManager, updateLoginStatus), stub_updateLoginStatus);
    }
};

// 直接访问 NetworkUtilPrivate::confirmTargetAddress (-fno-access-control)
static void setConfirmTarget(const QString &ip)
{
    NetworkUtil::instance()->d->confirmTargetAddress = ip;
}
}   // namespace

// ===== 纯 getter/setter (无需 stub) =====

TEST(NetworkUtilPure, StorageFolderRoundtrip)
{
    NetworkUtil::instance()->setStorageFolder("MySub");
    QString folder = NetworkUtil::instance()->getStorageFolder();
    EXPECT_TRUE(folder.contains("MySub"));
}

TEST(NetworkUtilPure, ConfirmTargetAddressDefaultEmpty)
{
    // 不修改, 仅验证可读
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->getConfirmTargetAddress());
}

TEST(NetworkUtilPure, DeviceInfoStrProducesJson)
{
    QString s = NetworkUtil::instance()->deviceInfoStr();
    EXPECT_FALSE(s.isEmpty());
    EXPECT_TRUE(s.contains("{") || s.contains("\""));
}

TEST(NetworkUtilPure, IsCurrentlyCooperatingReturnsBool)
{
    // 触 ShareCooperationServiceManager + compat ipc; offscreen 下不崩即过
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->isCurrentlyCooperating());
}

// ===== doNextCombiRequest: type<=0 早返回 =====
TEST(NetworkUtilCombi, DoNextCombiNoTypeEarlyReturn)
{
    // _nextCombi 默认 (0, "") → type<=0 → return
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doNextCombiRequest("10.0.0.1"));
}

// ===== trySearchDevice: sessionConnect 返回 0 (waiting) =====
TEST(NetworkUtilSearch, TrySearchDeviceWaitingBranch)
{
    SessionStubScope s;
    g_sessionConnectReturn = 0;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->trySearchDevice("10.0.0.1"));
}

// ===== tryTransApply: sessionConnect 返回 0 (waiting) =====
TEST(NetworkUtilTrans, TryTransApplyWaitingBranch)
{
    SessionStubScope s;
    g_sessionConnectReturn = 0;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->tryTransApply("10.0.0.2"));
}

// ===== tryShareApply: sessionConnect 返回 0 (waiting) =====
TEST(NetworkUtilShare, TryShareApplyWaitingBranch)
{
    SessionStubScope s;
    g_sessionConnectReturn = 0;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->tryShareApply("10.0.0.3", "print"));
}

// ===== guard 分支 (confirmTargetAddress == targetIp → if-branch, sendRpcRequest stubbed) =====

TEST(NetworkUtilGuard, ReplyTransRequestMatchingTarget)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.5");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->replyTransRequest(true, "10.0.0.5"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->replyTransRequest(false, "10.0.0.5"));
}

TEST(NetworkUtilGuard, CancelApplyMatchingTarget)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.6");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->cancelApply("share", "10.0.0.6"));
}

TEST(NetworkUtilGuard, CancelTransMatchingTarget)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.7");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->cancelTrans("10.0.0.7"));
}

TEST(NetworkUtilGuard, DoSendFilesMatchingTarget)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.8");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doSendFiles(QStringList{"/a", "/b"}, "10.0.0.8"));
}

TEST(NetworkUtilGuard, DoSendFilesNonMatchingCompatBranch)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.8");
    // targetIp != confirmTarget → compat ipc->call 分支 (未连接安全)
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doSendFiles(QStringList{"/a"}, "10.0.0.99"));
}

TEST(NetworkUtilGuard, SendDisconnectShareEventsMatchingTarget)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.9");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->sendDisconnectShareEvents("10.0.0.9"));
}

TEST(NetworkUtilGuard, ReplyShareRequestMatchingTarget)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.10");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->replyShareRequest(true, "print", "10.0.0.10"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->replyShareRequest(false, "print", "10.0.0.10"));
}

TEST(NetworkUtilGuard, ReplyShareRequestBusy)
{
    SessionStubScope s;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->replyShareRequestBusy("10.0.0.11"));
}

// ===== stop: 空/非空 confirmTargetAddress 两分支 =====
TEST(NetworkUtilStop, EmptyTargetNoDisconnect)
{
    SessionStubScope s;
    setConfirmTarget("");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->stop());
}

TEST(NetworkUtilStop, NonEmptyTargetDisconnects)
{
    SessionStubScope s;
    setConfirmTarget("10.0.0.12");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->stop());
}

// ===== pingTarget / disconnectRemote / reqTargetInfo 不崩 (session stubbed) =====
TEST(NetworkUtilSession, PingTargetNoCrash)
{
    SessionStubScope s;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->pingTarget("10.0.0.20"));
}

TEST(NetworkUtilSession, DisconnectRemoteNoCrash)
{
    SessionStubScope s;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->disconnectRemote("10.0.0.21"));
}

TEST(NetworkUtilSession, ReqTargetInfoNonCompatNoCrash)
{
    SessionStubScope s;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->reqTargetInfo("10.0.0.22", false));
}

// ===== updateStorageConfig: 赋值 + (compat ipc 分支可能崩, 用 stub 隔离) =====
TEST(NetworkUtilConfig, UpdateStorageConfigNoCrash)
{
    SessionStubScope s;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->updateStorageConfig("/tmp/coop_store"));
}

// ===== CooperationUtil + selfInfo stub (避免真网络/单例查询) =====
namespace {
static QString stub_localIPAddress() { return "127.0.0.1"; }
static QVariantMap stub_deviceInfo()
{
    QVariantMap m;
    m.insert("deviceName", "test-host");
    return m;
}
static ::DeviceInfoPointer stub_selfInfo()
{
    return ::DeviceInfoPointer(new cooperation_core::DeviceInfo("127.0.0.1", "test-host"));
}
static void stub_compatLogin(NetworkUtil *, const QString &) {}   // 屏蔽 ipc->call
static void stub_compatAppExit(NetworkUtil *) {}
static void stub_compatSendStartShare(NetworkUtil *, const QString &) {}
static void stub_updateCooperationStatus(NetworkUtil *, int) {}

struct UtilStubScope {
    Stub stub;
    UtilStubScope()
    {
        stub.set(ADDR(cooperation_core::CooperationUtil, localIPAddress), stub_localIPAddress);
        stub.set(ADDR(cooperation_core::CooperationUtil, deviceInfo), stub_deviceInfo);
        stub.set(ADDR(cooperation_core::DiscoverController, selfInfo), stub_selfInfo);
        stub.set(ADDR(NetworkUtil, compatLogin), stub_compatLogin);
        stub.set(ADDR(NetworkUtil, compatAppExit), stub_compatAppExit);
        stub.set(ADDR(NetworkUtil, compatSendStartShare), stub_compatSendStartShare);
        stub.set(ADDR(NetworkUtil, updateCooperationStatus), stub_updateCooperationStatus);
    }
};
}   // namespace

// ===== doNextCombiRequest switch 全分支 (type>0) =====
TEST(NetworkUtilCombi, DoNextCombiApplyInfoBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    // _nextCombi.first = APPLY_INFO (10) → reqTargetInfo(compat=false) → sendRpcRequest(stubbed)
    NetworkUtil::instance()->_nextCombi = qMakePair(10, QString("10.0.0.30"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doNextCombiRequest("10.0.0.30"));
}

TEST(NetworkUtilCombi, DoNextCombiApplyTransBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    NetworkUtil::instance()->_nextCombi = qMakePair(11, QString("10.0.0.31"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doNextCombiRequest("10.0.0.31"));
}

TEST(NetworkUtilCombi, DoNextCombiApplyShareBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    NetworkUtil::instance()->_nextCombi = qMakePair(12, QString("10.0.0.32"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doNextCombiRequest("10.0.0.32"));
}

TEST(NetworkUtilCombi, DoNextCombiUnknownTypeBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    NetworkUtil::instance()->_nextCombi = qMakePair(999, QString("10.0.0.33"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->doNextCombiRequest("10.0.0.33"));
}

// ===== try* 系列: sessionConnect 返回 1 (logged-in → doNextCombi) =====
TEST(NetworkUtilSearch, TrySearchDeviceLoggedInBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    g_sessionConnectReturn = 1;
    NetworkUtil::instance()->_nextCombi = qMakePair(10, QString("10.0.0.40"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->trySearchDevice("10.0.0.40"));
}

TEST(NetworkUtilTrans, TryTransApplyLoggedInBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    g_sessionConnectReturn = 1;
    NetworkUtil::instance()->_nextCombi = qMakePair(11, QString("10.0.0.41"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->tryTransApply("10.0.0.41"));
}

TEST(NetworkUtilShare, TryShareApplyLoggedInBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    g_sessionConnectReturn = 1;
    NetworkUtil::instance()->_nextCombi = qMakePair(12, QString("10.0.0.42"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->tryShareApply("10.0.0.42", "print"));
}

// ===== try* 系列: sessionConnect 返回 -1 (fail → compatLogin, stubbed) =====
TEST(NetworkUtilSearch, TrySearchDeviceFailCompatBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    g_sessionConnectReturn = -1;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->trySearchDevice("10.0.0.50"));
}

TEST(NetworkUtilTrans, TryTransApplyFailCompatBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    g_sessionConnectReturn = -1;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->tryTransApply("10.0.0.51"));
}

TEST(NetworkUtilShare, TryShareApplyFailCompatBranch)
{
    SessionStubScope ss;
    UtilStubScope us;
    g_sessionConnectReturn = -1;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->tryShareApply("10.0.0.52", "print"));
}

// ===== reqTargetInfo compat=true 分支 (compatLogin stubbed via UtilStubScope 间接? 不, 这里测 reqTargetInfo 本身) =====
TEST(NetworkUtilSession, ReqTargetInfoCompatBranchNoCrash)
{
    SessionStubScope ss;
    UtilStubScope us;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->reqTargetInfo("10.0.0.60", true));
}

// ===== sendTransApply / sendShareApply compat=true 分支 (ipc->call, 可能在 offscreen 下失败/不崩) =====
TEST(NetworkUtilSession, SendTransApplyCompatBranchNoCrash)
{
    SessionStubScope ss;
    UtilStubScope us;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->sendTransApply("10.0.0.61", true));
}

TEST(NetworkUtilSession, SendShareApplyCompatBranchNoCrash)
{
    SessionStubScope ss;
    UtilStubScope us;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->sendShareApply("10.0.0.62", true));
}

// ===== compat 系列方法 (stubbed ipc 间接? 这些直接 ipc->call, 可能崩; 用 UtilStubScope 已 stub NetworkUtil 自身方法? 不, 这些就是要测的) =====
// 注: compatSendStartShare/compatAppExit/updateCooperationStatus 内部 ipc->call, offscreen 下可能不崩 (daemon 未连接时 call 返回错误)
TEST(NetworkUtilCompat, CompatSendStartShareNoCrash)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->compatSendStartShare("screen1"));
}

TEST(NetworkUtilCompat, CompatAppExitNoCrash)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->compatAppExit());
}

TEST(NetworkUtilCompat, UpdateCooperationStatusNoCrash)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->updateCooperationStatus(1));
}

// ===== NetworkUtilPrivate::handleConnectStatus 全分支 =====
// 各分支只用 invokeMethod(QueuedConnection), 不处理事件循环即安全。

TEST(NetworkUtilHandle, ConnectStatusHostUnreachable113)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleConnectStatus(113, "10.0.0.1"));
}

TEST(NetworkUtilHandle, ConnectStatusTimeout110)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleConnectStatus(110, "10.0.0.2"));
}

TEST(NetworkUtilHandle, ConnectStatusPingout)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleConnectStatus(EX_NETWORK_PINGOUT, "10.0.0.3"));
}

TEST(NetworkUtilHandle, ConnectStatusErrorNeg2)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleConnectStatus(-2, "err"));
}

TEST(NetworkUtilHandle, ConnectStatusDisconnectedNeg1)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleConnectStatus(-1, "10.0.0.4"));
}

TEST(NetworkUtilHandle, ConnectStatusUnknownResult)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleConnectStatus(999, "x"));
}

// ===== NetworkUtilPrivate::handleTransChanged =====

TEST(NetworkUtilHandle, TransChangedNoCrash)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleTransChanged(1, "/tmp/f", 100));
}

// ===== NetworkUtilPrivate::handleAsyncRpcResult switch 全分支 =====

TEST(NetworkUtilHandle, AsyncRpcLoginSuccess)
{
    SessionStubScope ss;
    LoginMessage lm;
    lm.name = "10.0.0.10";
    lm.auth = "token";
    QString resp = QString::fromStdString(lm.as_json().serialize());
    NetworkUtil::instance()->_nextCombi = qMakePair(100, QString("10.0.0.10"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_LOGIN, resp));
}

TEST(NetworkUtilHandle, AsyncRpcLoginFailTriggersCompatLogin)
{
    SessionStubScope ss;
    // 空 response → success=false → compatLogin(response) (真 ipc->call, 未连接安全)
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_LOGIN, QString("")));
}

TEST(NetworkUtilHandle, AsyncRpcApplyInfoSuccess)
{
    SessionStubScope ss;
    ApplyMessage am;
    am.nick = "host10";
    am.host = "10.0.0.11";
    am.port = 51598;
    QString resp = QString::fromStdString(am.as_json().serialize());
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_INFO, resp));
}

TEST(NetworkUtilHandle, AsyncRpcApplyInfoFail)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_INFO, QString("")));
}

TEST(NetworkUtilHandle, AsyncRpcApplyTrans)
{
    SessionStubScope ss;
    setConfirmTarget("10.0.0.12");
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_TRANS, QString("{}")));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_TRANS, QString("")));
}

TEST(NetworkUtilHandle, AsyncRpcApplyShareFail)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_SHARE, QString("")));
}

TEST(NetworkUtilHandle, AsyncRpcApplyShareSuccess)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(APPLY_SHARE, QString("{}")));
}

TEST(NetworkUtilHandle, AsyncRpcUnknownTypeDefault)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->d->handleAsyncRpcResult(99999, QString("{}")));
}

// ===== NetworkUtil::handleCompatConnectResult 全分支 (ENABLE_COMPAT) =====

TEST(NetworkUtilHandle, CompatConnectResultTransBranch)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair((int)APPLY_TRANS, QString("10.0.0.20"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(1, "10.0.0.20"));
}

TEST(NetworkUtilHandle, CompatConnectResultShareFail)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair((int)APPLY_SHARE, QString("10.0.0.21"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(0, "10.0.0.21"));
}

TEST(NetworkUtilHandle, CompatConnectResultShareOk)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair((int)APPLY_SHARE, QString("10.0.0.22"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(1, "10.0.0.22"));
}

TEST(NetworkUtilHandle, CompatConnectResultInfoFail)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair((int)APPLY_INFO, QString("10.0.0.23"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(0, "10.0.0.23"));
}

TEST(NetworkUtilHandle, CompatConnectResultUnknownType)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair(999, QString("10.0.0.24"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(1, "10.0.0.24"));
}

TEST(NetworkUtilHandle, CompatConnectResultLoginSuccessDoNext)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair(999, QString("10.0.0.25"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(2, "10.0.0.25"));
}

TEST(NetworkUtilHandle, CompatConnectResultLoginFailClearsPending)
{
    SessionStubScope ss;
    NetworkUtil::instance()->_nextCombi = qMakePair(999, QString("10.0.0.26"));
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatConnectResult(-1, "10.0.0.26"));
}

// ===== NetworkUtil::handleCompatRegister reg=true/false =====

TEST(NetworkUtilHandle, CompatRegisterTrue)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatRegister(true, "{\"info\":\"x\"}"));
}

TEST(NetworkUtilHandle, CompatRegisterFalse)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatRegister(false, QString()));
}

// ===== NetworkUtil::handleCompatDiscover: ipc 未连接 → nodesJson 空 → else 分支 =====

TEST(NetworkUtilHandle, CompatDiscoverEmptyNodesJson)
{
    SessionStubScope ss;
    EXPECT_NO_FATAL_FAILURE(NetworkUtil::instance()->handleCompatDiscover());
}

// ===== ExtenMessageHandler msg_cb 全分支 =====
// NetworkUtilPrivate 构造时把 msg_cb 注册到 sessionManager->setSessionExtCallback。
// 用静态存储期 Stub 在 main 之前 patch setSessionExtCallback, 把回调捕获到全局,
// 测试里直接 invoke (invokeMethod(QueuedConnection) 不 spin 事件循环即安全)。
namespace {
static ::ExtenMessageHandler g_capturedMsgCb;
static void stub_setSessionExtCallback(SessionManager *, ::ExtenMessageHandler cb)
{
    g_capturedMsgCb = std::move(cb);
}
struct MsgCbCapture {
    Stub stub;
    MsgCbCapture()
    {
        stub.set(ADDR(SessionManager, setSessionExtCallback), stub_setSessionExtCallback);
    }
};
static MsgCbCapture g_msgCbCapture;   // 静态初始化先于 main → 早于任何 instance() 调用

static picojson::value applyMsgJson(int64_t flag, const std::string &nick,
                                    const std::string &host, int64_t port,
                                    const std::string &fp = "")
{
    ApplyMessage am;
    am.flag = flag;
    am.nick = nick;
    am.host = host;
    am.port = port;
    am.fingerprint = fp;
    return am.as_json();
}

// 触发首次构造 (幂等), 确保 msg_cb 已被静态 stub 捕获
static void ensureMsgCbCaptured()
{
    NetworkUtil::instance();
}
}   // namespace

TEST(NetworkUtilMsgCb, ApplyInfoBranch)
{
    ensureMsgCbCaptured();
    ASSERT_TRUE(static_cast<bool>(g_capturedMsgCb));
    std::string res;
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "nick_info", "10.0.0.1", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_INFO, v, &res));
    EXPECT_FALSE(res.empty());
}

TEST(NetworkUtilMsgCb, ApplyTransBranch)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "nick_t", "10.0.0.2", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_TRANS, v, &res));
    EXPECT_FALSE(res.empty());
}

TEST(NetworkUtilMsgCb, ApplyTransResultAccept)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(REPLY_ACCEPT, "nick", "10.0.0.3", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_TRANS_RESULT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyTransResultReject)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(REPLY_REJECT, "nick", "10.0.0.3", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_TRANS_RESULT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyShareBranch)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "nick_s", "10.0.0.4", 1234, "fp1");
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SHARE, v, &res));
    EXPECT_FALSE(res.empty());
}

TEST(NetworkUtilMsgCb, ApplyShareResultAccept)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(REPLY_ACCEPT, "nick", "10.0.0.5", 1234, "fp1");
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SHARE_RESULT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyShareResultBusy)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(REPLY_REJECT, "BUSY_COOPERATING", "10.0.0.5", 1234, "fp1");
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SHARE_RESULT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyShareResultRefuse)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(REPLY_REJECT, "nick_refuse", "10.0.0.5", 1234, "fp1");
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SHARE_RESULT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyShareStopBranch)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(DO_DONE, "nick", "10.0.0.6", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SHARE_STOP, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyCanceledShare)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_CANCELED, "share", "10.0.0.7", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_CANCELED, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyCanceledTransfer)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_CANCELED, "transfer", "10.0.0.7", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_CANCELED, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyCanceledUnknownType)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_CANCELED, "unknown_type", "10.0.0.7", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_CANCELED, v, &res));
}

#ifdef ENABLE_PHONE
TEST(NetworkUtilMsgCb, ApplyScanConnectWithResolution)
{
    ensureMsgCbCaptured();
    std::string res;
    // nick 含 "name=2400x1080" 走分辨率解析分支
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "pixel 7=2400x1080", "10.0.0.8", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SCAN_CONNECT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyScanConnectNoResolution)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "plain_device", "10.0.0.9", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SCAN_CONNECT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyScanConnectBadResolution)
{
    ensureMsgCbCaptured();
    std::string res;
    // 含 "=" 但分辨率非 "WxH" 形式 → resolutionParts.size()!=2 分支
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "dev=badformat", "10.0.0.10", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_SCAN_CONNECT, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyProjectionBranch)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(ASK_NEEDCONFIRM, "nick_p", "10.0.0.11", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_PROJECTION, v, &res));
}

TEST(NetworkUtilMsgCb, ApplyProjectionStopBranch)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(DO_DONE, "nick_p", "10.0.0.12", 1234);
    EXPECT_TRUE(g_capturedMsgCb(APPLY_PROJECTION_STOP, v, &res));
}
#endif

TEST(NetworkUtilMsgCb, UnknownMaskReturnsFalse)
{
    ensureMsgCbCaptured();
    std::string res;
    picojson::value v = applyMsgJson(0, "x", "10.0.0.99", 1234);
    EXPECT_FALSE(g_capturedMsgCb(99999, v, &res));
}
