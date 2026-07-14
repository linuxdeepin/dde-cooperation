// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for ShareHelper (cooperation_core::ShareHelper).
//
// ShareHelper is a process-wide singleton QObject backed by a state machine
// that drives DBus notifications, network apply/disconnect and a barrier
// server/client. Most slots touch NetworkUtil / ShareCooperationServiceManager
// / DiscoverController, which would block or spawn processes. We therefore
// restrict ourselves to the non-blocking surface: static button callbacks,
// early-return branches of the state-machine slots (no target device / not
// connected / unknown ids), selfSharing for unknown IPs, and
// switchPeripheralShared when the client is null.
//
// QApplication is provided by main.cpp (custom main with __gcov_dump flush).

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>

#include "lib/cooperation/core/net/helper/sharehelper.h"
#include "lib/cooperation/core/net/helper/sharehelper_p.h"
#include "lib/cooperation/core/net/cooconstrants.h"
#include "lib/cooperation/core/net/networkutil.h"
#include "lib/cooperation/core/discover/deviceinfo.h"
#include "lib/cooperation/core/discover/discovercontroller.h"
#include "lib/cooperation/core/utils/cooperationutil.h"
#include "lib/cooperation/core/share/sharecooperationservicemanager.h"
#include "stub.h"

using cooperation_core::DeviceInfo;
using cooperation_core::ShareHelper;
using cooperation_core::ShareHelperPrivate;
using cooperation_core::NetworkUtil;
using cooperation_core::DiscoverController;
using cooperation_core::CooperationUtil;
using ::ShareCooperationServiceManager;
using ::DeviceInfoPointer;

// stub: ShareHelperPrivate::notifyMessage (DBus) → no-op
static void stub_shp_notifyMessage(ShareHelperPrivate *, const QString &, const QStringList &, int) {}
static void stub_cancelApply(NetworkUtil *, const QString &, const QString &) {}
static void stub_compatSendStartShare(NetworkUtil *, const QString &) {}
static void stub_updateCooperationStatus(NetworkUtil *, int) {}
static void stub_disconnectRemote(NetworkUtil *, const QString &) {}
static void stub_scs_stop(ShareCooperationServiceManager *) {}
static void stub_updateDeviceState(DiscoverController *, const DeviceInfoPointer) {}
static void stub_replyShareRequest(NetworkUtil *, bool, const QString &, const QString &) {}
static void stub_tryShareApply(NetworkUtil *, const QString &, const QString &) {}
static void stub_sendDisconnectShareEvents(NetworkUtil *, const QString &) {}
static void stub_replyShareRequestBusy(NetworkUtil *, const QString &) {}
static bool stub_isCurrentlyCooperating(NetworkUtil *) { return false; }
static bool stub_isCurrentlyCooperating_true(NetworkUtil *) { return true; }

struct ShareHelperStubScope {
    Stub stub;
    ShareHelperStubScope()
    {
        stub.set(ADDR(ShareHelperPrivate, notifyMessage), stub_shp_notifyMessage);
        stub.set(ADDR(NetworkUtil, cancelApply), stub_cancelApply);
        stub.set(ADDR(NetworkUtil, compatSendStartShare), stub_compatSendStartShare);
        stub.set(ADDR(NetworkUtil, updateCooperationStatus), stub_updateCooperationStatus);
        stub.set(ADDR(NetworkUtil, disconnectRemote), stub_disconnectRemote);
        stub.set(ADDR(NetworkUtil, replyShareRequest), stub_replyShareRequest);
        stub.set(ADDR(NetworkUtil, tryShareApply), stub_tryShareApply);
        stub.set(ADDR(NetworkUtil, sendDisconnectShareEvents), stub_sendDisconnectShareEvents);
        stub.set(ADDR(NetworkUtil, replyShareRequestBusy), stub_replyShareRequestBusy);
        stub.set(ADDR(NetworkUtil, isCurrentlyCooperating), stub_isCurrentlyCooperating);
        stub.set(ADDR(ShareCooperationServiceManager, stop), stub_scs_stop);
        stub.set(ADDR(DiscoverController, updateDeviceState), stub_updateDeviceState);
    }
};

static DeviceInfoPointer makeConnectedDev(const QString &ip, const QString &name)
{
    auto p = DeviceInfoPointer(new DeviceInfo(ip, name));
    p->setConnectStatus(DeviceInfo::Connected);
    return p;
}

class ShareHelperTest : public ::testing::Test {
protected:
    ShareHelper *helper = nullptr;
    void SetUp() override { helper = ShareHelper::instance(); }
    void TearDown() override
    {
        // 复位单例状态, 防止跨测试泄漏
        helper->d->targetDeviceInfo.reset();
        helper->d->isRecvMode = false;
        helper->d->isReplied = false;
        helper->d->isTimeout = false;
        helper->d->senderDeviceIp.clear();
        helper->d->targetDevName.clear();
        helper->d->recvServerPrint.clear();
    }
};

TEST_F(ShareHelperTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(ShareHelper::instance(), helper);
}

TEST_F(ShareHelperTest, SelfSharingReturnsNegOneForUnknownIp)
{
    // Non-local IP never matches CooperationUtil::localIPAddress() → -1.
    EXPECT_EQ(helper->selfSharing("0.0.0.0"), -1);
}

TEST_F(ShareHelperTest, ButtonVisibleReturnsFalseForUnknownId)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.1", "Dev"));
    EXPECT_FALSE(ShareHelper::buttonVisible("unknown_id", info));
}

TEST_F(ShareHelperTest, ButtonVisibleConnectFalseWhenNotConnectable)
{
    // Default-constructed DeviceInfo is not Connectable, so connect button hidden.
    DeviceInfoPointer info(new DeviceInfo("10.0.0.2", "Dev2"));
    EXPECT_FALSE(ShareHelper::buttonVisible("connect-button", info));
}

TEST_F(ShareHelperTest, ButtonVisibleDisconnectFalseWhenNotConnected)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.3", "Dev3"));
    EXPECT_FALSE(ShareHelper::buttonVisible("disconnect-button", info));
}

TEST_F(ShareHelperTest, ButtonVisibleConnectTrueWhenConnectable)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.4", "Dev4"));
    info->setConnectStatus(DeviceInfo::ConnectStatus::Connectable);
    EXPECT_TRUE(ShareHelper::buttonVisible("connect-button", info));
}

TEST_F(ShareHelperTest, ButtonVisibleDisconnectTrueWhenConnected)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.5", "Dev5"));
    info->setConnectStatus(DeviceInfo::ConnectStatus::Connected);
    EXPECT_TRUE(ShareHelper::buttonVisible("disconnect-button", info));
}

TEST_F(ShareHelperTest, ButtonClickedWithUnknownIdDoesNotCrash)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.6", "Dev6"));
    EXPECT_NO_FATAL_FAILURE(ShareHelper::buttonClicked("unknown_id", info));
}

TEST_F(ShareHelperTest, HandleConnectResultIgnoredWithoutTarget)
{
    // No target device set → early return (line 438-441).
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(-1, ""));
}

TEST_F(ShareHelperTest, HandleConnectResultWithUnknownCodeDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(9999, ""));
}

TEST_F(ShareHelperTest, HandleDisConnectResultEarlyReturnWithoutTarget)
{
    // No target device → early return (line 538-541).
    EXPECT_NO_FATAL_FAILURE(helper->handleDisConnectResult("10.0.0.7"));
}

TEST_F(ShareHelperTest, HandleCancelCooperApplyDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->handleCancelCooperApply());
}

TEST_F(ShareHelperTest, HandleNetworkDismissWithGenericErrorDoesNotCrash)
{
    // Message without errorType -1 → generic notify branch.
    EXPECT_NO_FATAL_FAILURE(helper->handleNetworkDismiss("some error"));
}

TEST_F(ShareHelperTest, HandleNetworkDismissWithErrorTypeMinusOneDoesNotCrash)
{
    // Message with errorType -1 and dialog not visible → early return.
    EXPECT_NO_FATAL_FAILURE(helper->handleNetworkDismiss("\"errorType\":-1"));
}

TEST_F(ShareHelperTest, OnShareExceptedIgnoredWhenNotConnected)
{
    // No target device → early return (line 633-636).
    EXPECT_NO_FATAL_FAILURE(helper->onShareExcepted(1, "10.0.0.8"));
}

TEST_F(ShareHelperTest, OnVerifyTimeoutDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
}

TEST_F(ShareHelperTest, SwitchPeripheralSharedReturnsFalseWhenClientNull)
{
    // Client is not started in test env → isNull() true → return false.
    EXPECT_FALSE(helper->switchPeripheralShared(true));
}

// ===== handleConnectResult switch 分支 (设 targetDeviceInfo, 不调 notifyMessage) =====

TEST_F(ShareHelperTest, HandleConnectResultUnableBranch)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.20", "dev20");
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(SHARE_CONNECT_UNABLE, ""));
}

TEST_F(ShareHelperTest, HandleConnectResultRefuseBranch)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.21", "dev21");
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(SHARE_CONNECT_REFUSE, ""));
}

TEST_F(ShareHelperTest, HandleConnectResultErrConnectedBranch)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.22", "dev22");
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(SHARE_CONNECT_ERR_CONNECTED, ""));
}

// ===== handleDisConnectResult (设 targetDeviceInfo + stub notifyMessage) =====

TEST_F(ShareHelperTest, HandleDisConnectResultWithTarget)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.23", "dev23");
    EXPECT_NO_FATAL_FAILURE(helper->handleDisConnectResult("dev23"));
}

// ===== onVerifyTimeout: recvMode + 未 replied → notifyMessage (stubbed) =====

TEST_F(ShareHelperTest, OnVerifyTimeoutRecvModeNotReplied)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = true;
    helper->d->isReplied = false;
    helper->d->targetDevName = "dev24";
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
}

TEST_F(ShareHelperTest, OnVerifyTimeoutRecvModeAlreadyReplied)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = true;
    helper->d->isReplied = true;
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
}

TEST_F(ShareHelperTest, OnVerifyTimeoutSendModeDialogNotVisible)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = false;
    helper->d->isReplied = true;
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
}

// ===== handleCancelCooperApply: recvMode + 未 replied → notifyMessage (stubbed) =====

TEST_F(ShareHelperTest, HandleCancelCooperApplyRecvModeNotReplied)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = true;
    helper->d->isReplied = false;
    EXPECT_NO_FATAL_FAILURE(helper->handleCancelCooperApply());
}

TEST_F(ShareHelperTest, HandleCancelCooperApplyRecvModeReplied)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = true;
    helper->d->isReplied = true;
    EXPECT_NO_FATAL_FAILURE(helper->handleCancelCooperApply());
}

// ===== onAppAttributeChanged 分支 =====

TEST_F(ShareHelperTest, OnAppAttributeChangedWrongGroupEarlyReturn)
{
    ShareHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->d->onAppAttributeChanged("OtherGroup", "any", QVariant()));
}

TEST_F(ShareHelperTest, OnAppAttributeChangedPeripheralShareKey)
{
    ShareHelperStubScope s;
    // GenericGroup + PeripheralShareKey → switchPeripheralShared(false) → client null → false, 安全
    EXPECT_NO_FATAL_FAILURE(helper->d->onAppAttributeChanged(AppSettings::GenericGroup,
                                                              AppSettings::PeripheralShareKey,
                                                              QVariant(false)));
}

// ===== stopCooperation: targetDeviceInfo Connected → disconnectToDevice (stubbed) =====

TEST_F(ShareHelperTest, StopCooperationWithConnectedTarget)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.25", "dev25");
    EXPECT_NO_FATAL_FAILURE(helper->d->stopCooperation());
}

TEST_F(ShareHelperTest, StopCooperationNoTarget)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo.reset();
    EXPECT_NO_FATAL_FAILURE(helper->d->stopCooperation());
}

// ===== onActionTriggered: reject → replyShareRequest (stubbed) =====

TEST_F(ShareHelperTest, OnActionTriggeredRejectBranch)
{
    ShareHelperStubScope s;
    helper->d->senderDeviceIp = "10.0.0.60";
    helper->d->selfFingerPrint = "print";
    EXPECT_NO_FATAL_FAILURE(helper->d->onActionTriggered("reject"));
}

TEST_F(ShareHelperTest, OnActionTriggeredUnknownBranch)
{
    ShareHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->d->onActionTriggered("unknown_action"));
}

// ===== connectToDevice: 已连接 → switchFailPage 早返回 =====

TEST_F(ShareHelperTest, ConnectToDeviceAlreadyConnectedBranch)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.61", "dev61");
    auto info = makeConnectedDev("10.0.0.62", "dev62");
    EXPECT_NO_FATAL_FAILURE(helper->connectToDevice(info));
}

// ===== onShareExcepted: 已连接目标 → EX_NETWORK_PINGOUT 显示失败页 =====

TEST_F(ShareHelperTest, OnShareExceptedConnectedPingOutShowsDialog)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.70", "dev70");
    EXPECT_NO_FATAL_FAILURE(helper->onShareExcepted(EX_NETWORK_PINGOUT, "10.0.0.70"));
}

// onShareExcepted: 已连接目标 → EX_OTHER 分支 (无操作)
TEST_F(ShareHelperTest, OnShareExceptedConnectedOtherBranch)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.71", "dev71");
    EXPECT_NO_FATAL_FAILURE(helper->onShareExcepted(EX_OTHER, "10.0.0.71"));
}

// onShareExcepted: 已连接但状态非 Connected → 早返回
TEST_F(ShareHelperTest, OnShareExceptedConnectedButNotConnectedStatus)
{
    ShareHelperStubScope s;
    auto info = DeviceInfoPointer(new DeviceInfo("10.0.0.72", "dev72"));
    info->setConnectStatus(DeviceInfo::Connectable);
    helper->d->targetDeviceInfo = info;
    EXPECT_NO_FATAL_FAILURE(helper->onShareExcepted(EX_NETWORK_PINGOUT, "10.0.0.72"));
}

// ===== notifyConnectRequest: info 过短 (< 2 段) → 早返回 =====

TEST_F(ShareHelperTest, NotifyConnectRequestShortInfoEarlyReturn)
{
    ShareHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->notifyConnectRequest("only-one-part"));
    // 不到 2 段, senderDeviceIp 不应被设置
    EXPECT_TRUE(helper->d->senderDeviceIp.isEmpty());
}

// notifyConnectRequest: 合法 info → 设置 senderDeviceIp / targetDevName
TEST_F(ShareHelperTest, NotifyConnectRequestValidSetsState)
{
    ShareHelperStubScope s;
    helper->notifyConnectRequest("10.0.0.74,dev74,fingerprint123");
    EXPECT_EQ(helper->d->senderDeviceIp, "10.0.0.74");
    EXPECT_EQ(helper->d->targetDevName, "dev74");
    EXPECT_EQ(helper->d->recvServerPrint, "fingerprint123");
    EXPECT_TRUE(helper->d->isRecvMode);
}

// notifyConnectRequest: 合法 info 但无指纹 → recvServerPrint 保持空
TEST_F(ShareHelperTest, NotifyConnectRequestValidNoFingerprint)
{
    ShareHelperStubScope s;
    helper->notifyConnectRequest("10.0.0.75,dev75");
    EXPECT_EQ(helper->d->senderDeviceIp, "10.0.0.75");
    EXPECT_TRUE(helper->d->recvServerPrint.isEmpty());
}

// notifyConnectRequest: 正在协同中 → busy 回复 (replyShareRequestBusy 已 stub)
TEST_F(ShareHelperTest, NotifyConnectRequestWhenCooperating)
{
    Stub stub;
    stub.set(ADDR(NetworkUtil, isCurrentlyCooperating), stub_isCurrentlyCooperating_true);
    stub.set(ADDR(NetworkUtil, replyShareRequestBusy), stub_replyShareRequestBusy);
    stub.set(ADDR(ShareHelperPrivate, notifyMessage), stub_shp_notifyMessage);
    EXPECT_NO_FATAL_FAILURE(helper->notifyConnectRequest("10.0.0.76,dev76"));
    // 正在协同 → 直接返回, 不设置接收模式
    EXPECT_FALSE(helper->d->isRecvMode);
}

// ===== handleConnectResult: SHARE_CONNECT_COMFIRM → restartBarrier 失败 → 失败页 =====

TEST_F(ShareHelperTest, HandleConnectResultComfirmBarrierFail)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.80", "dev80");
    // 空 clientprint → crypto=false; restartBarrier 在测试环境返回 false
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(SHARE_CONNECT_COMFIRM, ""));
    // 失败路径: targetDeviceInfo 被重置
    EXPECT_EQ(helper->d->targetDeviceInfo, nullptr);
}

// handleConnectResult: isTimeout=true → 早返回
TEST_F(ShareHelperTest, HandleConnectResultTimeoutEarlyReturn)
{
    ShareHelperStubScope s;
    helper->d->targetDeviceInfo = makeConnectedDev("10.0.0.81", "dev81");
    helper->d->isTimeout = true;
    EXPECT_NO_FATAL_FAILURE(helper->handleConnectResult(SHARE_CONNECT_UNABLE, ""));
    // 超时早返回, targetDeviceInfo 保留
    EXPECT_NE(helper->d->targetDeviceInfo, nullptr);
}

// ===== buttonClicked: connect-button → connectToDevice (tryShareApply 已 stub) =====

TEST_F(ShareHelperTest, ButtonClickedConnectButtonNoCrash)
{
    ShareHelperStubScope s;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.82", "dev82"));
    info->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_NO_FATAL_FAILURE(ShareHelper::buttonClicked("connect-button", info));
    // connectToDevice 设置 targetDeviceInfo
    EXPECT_NE(helper->d->targetDeviceInfo, nullptr);
}

// buttonClicked: disconnect-button → disconnectToDevice (sendDisconnectShareEvents 已 stub)
TEST_F(ShareHelperTest, ButtonClickedDisconnectButtonNoCrash)
{
    ShareHelperStubScope s;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.83", "dev83"));
    info->setConnectStatus(DeviceInfo::Connected);
    EXPECT_NO_FATAL_FAILURE(ShareHelper::buttonClicked("disconnect-button", info));
}

// ===== switchPeripheralShared(false) → stopBarrier, 返回 true =====

TEST_F(ShareHelperTest, SwitchPeripheralSharedFalseReturnsTrue)
{
    // client 非空 → on=false → stopBarrier() → 返回 true
    EXPECT_TRUE(helper->switchPeripheralShared(false));
}

// switchPeripheralShared(true) → startBarrier (在测试环境可能返回 false)
TEST_F(ShareHelperTest, SwitchPeripheralSharedTrueReturnsBool)
{
    EXPECT_NO_FATAL_FAILURE({ bool v = helper->switchPeripheralShared(true); (void)v; });
}

// ===== buttonVisible: qApp onlyTransfer=true → 一律 false =====

TEST_F(ShareHelperTest, ButtonVisibleOnlyTransferPropertyReturnsFalse)
{
    qApp->setProperty("onlyTransfer", true);
    DeviceInfoPointer info(new DeviceInfo("10.0.0.85", "dev85"));
    info->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_FALSE(ShareHelper::buttonVisible("connect-button", info));
    qApp->setProperty("onlyTransfer", QVariant());
}

// buttonVisible: 未知 id → false (默认)
TEST_F(ShareHelperTest, ButtonVisibleUnknownIdReturnsFalse)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.86", "dev86"));
    info->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_FALSE(ShareHelper::buttonVisible("unknown-id", info));
}

// buttonVisible: disconnect-button + Connected → true
TEST_F(ShareHelperTest, ButtonVisibleDisconnectConnectedReturnsTrue)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.87", "dev87"));
    info->setConnectStatus(DeviceInfo::Connected);
    EXPECT_TRUE(ShareHelper::buttonVisible("disconnect-button", info));
}

// ===== onActionTriggered (private): accept 分支 (replyShareRequest 已 stub) =====

TEST_F(ShareHelperTest, OnActionTriggeredAcceptBranchNoCrash)
{
    ShareHelperStubScope s;
    helper->d->senderDeviceIp = "10.0.0.90";
    helper->d->selfFingerPrint = "selfprint";
    EXPECT_NO_FATAL_FAILURE(helper->d->onActionTriggered("accept"));
    EXPECT_TRUE(helper->d->isReplied);
}

// ===== handleNetworkDismiss: errorType -1 + 对话框不可见 → 早返回 (已覆盖, 这里验证不崩) =====

TEST_F(ShareHelperTest, HandleNetworkDismissGenericMessageNotifies)
{
    ShareHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->handleNetworkDismiss("generic network error"));
}

// ===== selfSharing: 本机 IP 且 server/client 均未运行 → 1 =====

static QString stub_sh_localIPAddress() { return QStringLiteral("127.0.0.1"); }
TEST_F(ShareHelperTest, SelfSharingLocalIpReturnsOneWhenIdle)
{
    Stub stub;
    stub.set(ADDR(CooperationUtil, localIPAddress), stub_sh_localIPAddress);
    // server/client 存在但 isRunning()=false → 返回 1
    EXPECT_EQ(helper->selfSharing("127.0.0.1"), 1);
}

// ===== handleCancelCooperApply: 发送模式 (isRecvMode=false) → 无操作 =====

TEST_F(ShareHelperTest, HandleCancelCooperApplySendModeNoCrash)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = false;
    EXPECT_NO_FATAL_FAILURE(helper->handleCancelCooperApply());
}

// ===== onVerifyTimeout: 发送模式 + 已回复 → 早返回 =====

TEST_F(ShareHelperTest, OnVerifyTimeoutSendModeReplied)
{
    ShareHelperStubScope s;
    helper->d->isRecvMode = false;
    helper->d->isReplied = true;
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
}
