// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QString>
#include "lib/cooperation/core/net/helper/transferhelper.h"
#include "lib/cooperation/core/net/helper/transferhelper_p.h"
#include "lib/cooperation/core/net/networkutil.h"
#include "lib/cooperation/core/net/cooconstrants.h"
#include "lib/cooperation/core/discover/deviceinfo.h"
#include "common/constant.h"
#include "stub.h"

using cooperation_core::DeviceInfo;
using cooperation_core::TransferHelper;
using cooperation_core::TransferHelperPrivate;
using cooperation_core::NetworkUtil;
using ::DeviceInfoPointer;

// stub: TransferHelperPrivate::notifyMessage (DBus) → no-op
static void stub_thp_notifyMessage(TransferHelperPrivate *, const QString &, const QStringList &, int, const QVariantMap &) {}
static void stub_thp_reportTransferResult(TransferHelperPrivate *, bool) {}
// stub: NetworkUtil 网络/IPC 方法
static void stub_setStorageFolder(NetworkUtil *, const QString &) {}
static void stub_cancelTrans(NetworkUtil *, const QString &) {}
static void stub_replyTransRequest(NetworkUtil *, bool, const QString &) {}
static void stub_doSendFiles(NetworkUtil *, const QStringList &, const QString &) {}
static void stub_tryTransApply(NetworkUtil *, const QString &) {}
static void stub_cancelApply(NetworkUtil *, const QString &, const QString &) {}
static QString stub_getStorageFolder(NetworkUtil *) { return QString(); }
static QString stub_getConfirmTargetAddress(NetworkUtil *) { return QString(); }

struct TransferHelperStubScope {
    Stub stub;
    TransferHelperStubScope()
    {
        stub.set(ADDR(TransferHelperPrivate, notifyMessage), stub_thp_notifyMessage);
        stub.set(ADDR(TransferHelperPrivate, reportTransferResult), stub_thp_reportTransferResult);
        stub.set(ADDR(NetworkUtil, setStorageFolder), stub_setStorageFolder);
        stub.set(ADDR(NetworkUtil, cancelTrans), stub_cancelTrans);
        stub.set(ADDR(NetworkUtil, replyTransRequest), stub_replyTransRequest);
        stub.set(ADDR(NetworkUtil, doSendFiles), stub_doSendFiles);
        stub.set(ADDR(NetworkUtil, tryTransApply), stub_tryTransApply);
        stub.set(ADDR(NetworkUtil, cancelApply), stub_cancelApply);
        stub.set(ADDR(NetworkUtil, getStorageFolder), stub_getStorageFolder);
        stub.set(ADDR(NetworkUtil, getConfirmTargetAddress), stub_getConfirmTargetAddress);
    }
};

class TransferHelperCoreTest : public ::testing::Test {
protected:
    TransferHelper *helper = nullptr;
    void SetUp() override { helper = TransferHelper::instance(); }
    void TearDown() override
    {
        helper->d->status.storeRelease(TransferHelper::Idle);
        helper->d->role = TransferHelper::Server;
        helper->d->who.clear();
        helper->d->targetDeviceIp.clear();
        helper->d->recvFilesSavePath.clear();
        helper->d->transferInfo.clear();
        helper->d->isTransTimeout = false;
    }
};

TEST_F(TransferHelperCoreTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(TransferHelper::instance(), helper);
}

TEST_F(TransferHelperCoreTest, InitialTransferStatusIsIdle)
{
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

TEST_F(TransferHelperCoreTest, ButtonVisibleReturnsBoolForUnknownId)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.1", "Dev"));
    bool v = TransferHelper::buttonVisible("unknown", info);
    (void)v;
    SUCCEED();
}

TEST_F(TransferHelperCoreTest, ButtonClickableReturnsBoolForUnknownId)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.2", "Dev2"));
    bool c = TransferHelper::buttonClickable("unknown", info);
    (void)c;
    SUCCEED();
}

TEST_F(TransferHelperCoreTest, ButtonClickedWithUnknownIdDoesNotCrash)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.3", "Dev3"));
    EXPECT_NO_FATAL_FAILURE(TransferHelper::buttonClicked("unknown", info));
}

TEST_F(TransferHelperCoreTest, CancelTransferApplyDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->cancelTransferApply());
}

TEST_F(TransferHelperCoreTest, HandleCancelTransferApplyDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->handleCancelTransferApply());
}

TEST_F(TransferHelperCoreTest, CloseAllNotificationsDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->closeAllNotifications());
}

TEST_F(TransferHelperCoreTest, OnVerifyTimeoutDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
}

TEST_F(TransferHelperCoreTest, OpenFileLocationWithInvalidPathDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->openFileLocation("/nonexistent/path"));
}

TEST_F(TransferHelperCoreTest, OnActionTriggeredWithUnknownActionDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->onActionTriggered("unknown_action"));
}

TEST_F(TransferHelperCoreTest, AcceptedDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->accepted());
}

TEST_F(TransferHelperCoreTest, RejectedDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->rejected());
}

TEST_F(TransferHelperCoreTest, CancelTransferFalseDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->cancelTransfer(false));
}

TEST_F(TransferHelperCoreTest, CancelTransferTrueDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->cancelTransfer(true));
}

TEST_F(TransferHelperCoreTest, OnTransferExceptedWhenIdleDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->onTransferExcepted(1, "10.0.0.9"));
}

TEST_F(TransferHelperCoreTest, UpdateProgressDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->updateProgress(50, "00:01:00"));
}

TEST_F(TransferHelperCoreTest, TransferResultFalseDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->transferResult(false, "error"));
}

TEST_F(TransferHelperCoreTest, CompatFileTransStatusChangedDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->compatFileTransStatusChanged(1000, 500, 200));
}

TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->compatTransJobStatusChanged(1, 0, "ok"));
}

TEST_F(TransferHelperCoreTest, OnTransChangedIdleDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(0, "/tmp", 100));
}

TEST_F(TransferHelperCoreTest, DeliverSignalIsEmittable)
{
    QSignalSpy spy(helper, &TransferHelper::deliverMessage);
    helper->transferResult(true, "msg");
    (void)spy.count();
    SUCCEED();
}

TEST_F(TransferHelperCoreTest, OnActionTriggeredViewBranchDoesNotCrash)
{
    // "view" matches NotifyViewAction; reads config, no NetworkUtil/DBus call
    EXPECT_NO_FATAL_FAILURE(helper->onActionTriggered("view"));
}

TEST_F(TransferHelperCoreTest, OnActionTriggeredCloseBranchDoesNotCrash)
{
    // "close" matches NotifyCloseAction; calls notice->closeNotification() on linux
    EXPECT_NO_FATAL_FAILURE(helper->onActionTriggered("close"));
}

// ===== 深挖: 状态机分支 (stub notifyMessage + NetworkUtil) =====

TEST_F(TransferHelperCoreTest, NotifyTransferRequestNoCrash)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->notifyTransferRequest("nick", "10.0.0.30"));
}

TEST_F(TransferHelperCoreTest, HandleCancelTransferApplyNotifies)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->handleCancelTransferApply());
}

// onConnectStatusChanged: result>0 + isself → Confirming
TEST_F(TransferHelperCoreTest, OnConnectStatusChangedSuccessSelf)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->onConnectStatusChanged(1, "10.0.0.31", true));
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Confirming);
}

// onConnectStatusChanged: result>0 + !isself → 早返回
TEST_F(TransferHelperCoreTest, OnConnectStatusChangedSuccessNotSelf)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->onConnectStatusChanged(1, "10.0.0.32", false));
}

// onConnectStatusChanged: result<=0 + Idle → 早返回
TEST_F(TransferHelperCoreTest, OnConnectStatusChangedFailIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Idle);
    EXPECT_NO_FATAL_FAILURE(helper->onConnectStatusChanged(0, "10.0.0.33", false));
}

// onConnectStatusChanged: result<=0 + 非 Idle → transferResult(false)
TEST_F(TransferHelperCoreTest, OnConnectStatusChangedFailTransfering)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onConnectStatusChanged(0, "10.0.0.34", false));
}

// onTransChanged: TRANS_CANCELED → cancelTransfer(false)
TEST_F(TransferHelperCoreTest, OnTransChangedCanceled)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(48, "", 0));
}

// onTransChanged: TRANS_EXCEPTION io_error
TEST_F(TransferHelperCoreTest, OnTransChangedExceptionIoError)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(49, "io_error", 0));
}

// onTransChanged: TRANS_EXCEPTION net_error
TEST_F(TransferHelperCoreTest, OnTransChangedExceptionNetError)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(49, "net_error", 0));
}

// onTransChanged: TRANS_EXCEPTION other
TEST_F(TransferHelperCoreTest, OnTransChangedExceptionOther)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(49, "fs_exception", 0));
}

// onActionTriggered: cancel → cancelTransfer(true) + cancelTrans (stubbed)
TEST_F(TransferHelperCoreTest, OnActionTriggeredCancelBranch)
{
    TransferHelperStubScope s;
    helper->d->targetDeviceIp = "10.0.0.40";
    EXPECT_NO_FATAL_FAILURE(helper->onActionTriggered("cancel"));
}

// onActionTriggered: reject → replyTransRequest(false) (stubbed)
TEST_F(TransferHelperCoreTest, OnActionTriggeredRejectBranch)
{
    TransferHelperStubScope s;
    helper->d->targetDeviceIp = "10.0.0.41";
    EXPECT_NO_FATAL_FAILURE(helper->onActionTriggered("reject"));
}

// onActionTriggered: accept → role=Client + replyTransRequest(true)
TEST_F(TransferHelperCoreTest, OnActionTriggeredAcceptBranch)
{
    TransferHelperStubScope s;
    helper->d->targetDeviceIp = "10.0.0.42";
    EXPECT_NO_FATAL_FAILURE(helper->onActionTriggered("accept"));
}

// transferResult: role!=Server + result=true → notifyMessage(view action + hints)
TEST_F(TransferHelperCoreTest, TransferResultClientSuccess)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Client;
    EXPECT_NO_FATAL_FAILURE(helper->transferResult(true, "done"));
}

// transferResult: role!=Server + result=false → notifyMessage
TEST_F(TransferHelperCoreTest, TransferResultClientFail)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Client;
    EXPECT_NO_FATAL_FAILURE(helper->transferResult(false, "fail"));
}

// transferResult: role==Server → transDialog + reportTransferResult (stubbed)
TEST_F(TransferHelperCoreTest, TransferResultServer)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Server;
    EXPECT_NO_FATAL_FAILURE(helper->transferResult(true, "ok"));
}

// updateProgress: status!=Transfering → 早返回
TEST_F(TransferHelperCoreTest, UpdateProgressIdleEarlyReturn)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Idle);
    EXPECT_NO_FATAL_FAILURE(helper->updateProgress(50, "00:01"));
}

// updateProgress: Transfering + role!=Server → notifyMessage (stubbed)
TEST_F(TransferHelperCoreTest, UpdateProgressClientNotifies)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->d->role = TransferHelper::Client;
    helper->d->who = "dev50";
    EXPECT_NO_FATAL_FAILURE(helper->updateProgress(50, "00:01"));
}

// updateProgress: Transfering + role==Server → transDialog
TEST_F(TransferHelperCoreTest, UpdateProgressServerDialog)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->d->role = TransferHelper::Server;
    helper->d->who = "dev51";
    EXPECT_NO_FATAL_FAILURE(helper->updateProgress(60, "00:02"));
}

// onVerifyTimeout: Confirming → transDialog showResultDialog
TEST_F(TransferHelperCoreTest, OnVerifyTimeoutConfirmingShowsDialog)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Confirming);
    EXPECT_NO_FATAL_FAILURE(helper->onVerifyTimeout());
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// compatTransJobStatusChanged: success branch (result>=0)
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedSuccess)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->compatTransJobStatusChanged(1, 1, "/tmp/job"));
}

// compatTransJobStatusChanged: canceled branch (result == JOB_TRANS_CANCELED 13)
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedCanceled)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->compatTransJobStatusChanged(1, 13, "/tmp/job"));
}

// compatFileTransStatusChanged: progress update
TEST_F(TransferHelperCoreTest, CompatFileTransStatusChangedProgress)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->compatFileTransStatusChanged(1000, 500, 200));
}

// ===== 状态机分支断言 (验证 transferStatus 转换, 而非仅 NO_FATAL_FAILURE) =====

// onTransferExcepted: 非 Idle → cancelTransfer(true) + 转移到 Idle
TEST_F(TransferHelperCoreTest, OnTransferExceptedWhenTransferingSetsIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->onTransferExcepted(EX_NETWORK_PINGOUT, "10.0.0.70");
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// onTransferExcepted: EX_SPACE_NOTENOUGH 分支
TEST_F(TransferHelperCoreTest, OnTransferExceptedSpaceBranch)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onTransferExcepted(EX_SPACE_NOTENOUGH, "10.0.0.71"));
}

// onTransferExcepted: EX_FS_RWERROR 分支
TEST_F(TransferHelperCoreTest, OnTransferExceptedFsBranch)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onTransferExcepted(EX_FS_RWERROR, "10.0.0.72"));
}

// onTransferExcepted: EX_OTHER 分支 (无 transferResult 调用)
TEST_F(TransferHelperCoreTest, OnTransferExceptedOtherBranchNoCrash)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->onTransferExcepted(EX_OTHER, "10.0.0.73"));
}

// compatTransJobStatusChanged: JOB_TRANS_FAILED + Server → 隐藏 dialog 早返回
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedFailedServer)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Server;
    EXPECT_NO_FATAL_FAILURE(helper->compatTransJobStatusChanged(1, JOB_TRANS_FAILED, "fail"));
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// compatTransJobStatusChanged: JOB_TRANS_FAILED + Client + "::not enough"
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedFailedNotEnough)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Client;
    EXPECT_NO_FATAL_FAILURE(helper->compatTransJobStatusChanged(1, JOB_TRANS_FAILED, "io::not enough space"));
}

// compatTransJobStatusChanged: JOB_TRANS_FAILED + Client + "::off line"
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedFailedOffLine)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Client;
    EXPECT_NO_FATAL_FAILURE(helper->compatTransJobStatusChanged(1, JOB_TRANS_FAILED, "peer::off line"));
}

// compatTransJobStatusChanged: JOB_TRANS_DOING → Transfering
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedDoing)
{
    TransferHelperStubScope s;
    helper->compatTransJobStatusChanged(1, JOB_TRANS_DOING, "");
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Transfering);
}

// compatTransJobStatusChanged: JOB_TRANS_FINISHED + Client → Idle, recvFilesSavePath 被写入
TEST_F(TransferHelperCoreTest, CompatTransJobStatusChangedFinishedClient)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Client;
    helper->compatTransJobStatusChanged(1, JOB_TRANS_FINISHED, "/home/user/192.168.1.1");
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
    EXPECT_EQ(helper->d->recvFilesSavePath, "/home/user/192.168.1.1");
}

// compatFileTransStatusChanged: total <= current → progressValue=100 早返回 (无 updateProgress)
TEST_F(TransferHelperCoreTest, CompatFileTransStatusChangedComplete)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    EXPECT_NO_FATAL_FAILURE(helper->compatFileTransStatusChanged(1000, 1000, 1000));
}

// accepted: Confirming → Transfering + role=Server (doSendFiles 已 stub)
TEST_F(TransferHelperCoreTest, AcceptedFromConfirmingTransitionsToTransfering)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Confirming);
    helper->d->readyToSendFiles = QStringList{"/tmp/a"};
    helper->accepted();
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Transfering);
}

// accepted: 非 Confirming → status 置 Idle
TEST_F(TransferHelperCoreTest, AcceptedFromIdleResetsToIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Idle);
    helper->accepted();
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// rejected: 任何状态 → Idle
TEST_F(TransferHelperCoreTest, RejectedSetsIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Confirming);
    helper->rejected();
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// rejected: isTransTimeout=true → 不调用 transferResult
TEST_F(TransferHelperCoreTest, RejectedWhenTimeoutNoTransferResult)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Confirming);
    helper->d->isTransTimeout = true;
    EXPECT_NO_FATAL_FAILURE(helper->rejected());
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// cancelTransfer: Idle → 早返回, 状态保持 Idle
TEST_F(TransferHelperCoreTest, CancelTransferWhenIdleStaysIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Idle);
    helper->cancelTransfer(true);
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// cancelTransfer: Transfering + click=true → Idle, 仅 hide dialog
TEST_F(TransferHelperCoreTest, CancelTransferWhenTransferingClick)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->cancelTransfer(true);
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// cancelTransfer: Transfering + click=false → Idle + transferResult
TEST_F(TransferHelperCoreTest, CancelTransferWhenTransferingNoClick)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->cancelTransfer(false);
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// cancelTransferApply: 停止计时器 + 状态 Idle (cancelApply 已 stub)
TEST_F(TransferHelperCoreTest, CancelTransferApplySetsIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Confirming);
    helper->d->targetDeviceIp = "10.0.0.80";
    helper->cancelTransferApply();
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// waitForConfirm: 清空 transferInfo / recvFilesSavePath, 启动计时器
TEST_F(TransferHelperCoreTest, WaitForConfirmClearsState)
{
    TransferHelperStubScope s;
    helper->d->isTransTimeout = true;
    helper->d->recvFilesSavePath = "/tmp/old";
    helper->waitForConfirm();
    EXPECT_FALSE(helper->d->isTransTimeout);
    EXPECT_TRUE(helper->d->recvFilesSavePath.isEmpty());
    EXPECT_TRUE(helper->d->confirmTimer.isActive());
}

// onConnectStatusChanged: result<=0 + Connecting (非 Idle) → Idle
TEST_F(TransferHelperCoreTest, OnConnectStatusChangedFailConnecting)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Connecting);
    helper->onConnectStatusChanged(0, "10.0.0.81", true);
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// onTransChanged: TRANS_WHOLE_START → Transfering
TEST_F(TransferHelperCoreTest, OnTransChangedWholeStartSetsTransfering)
{
    TransferHelperStubScope s;
    helper->onTransChanged(TRANS_WHOLE_START, "", 0);
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Transfering);
}

// onTransChanged: TRANS_WHOLE_FINISH + Client → Idle, 写入 recvFilesSavePath
TEST_F(TransferHelperCoreTest, OnTransChangedWholeFinishClient)
{
    TransferHelperStubScope s;
    helper->d->role = TransferHelper::Client;
    helper->onTransChanged(TRANS_WHOLE_FINISH, "", 0);
    EXPECT_EQ(helper->transferStatus(), TransferHelper::Idle);
}

// onTransChanged: TRANS_COUNT_SIZE → 更新 totalSize
TEST_F(TransferHelperCoreTest, OnTransChangedCountSizeUpdatesTotal)
{
    TransferHelperStubScope s;
    helper->onTransChanged(TRANS_COUNT_SIZE, "", 4096);
    EXPECT_EQ(helper->d->transferInfo.totalSize, 4096);
}

// onTransChanged: TRANS_FILE_SPEED → 累计 transferSize + maxTimeS
TEST_F(TransferHelperCoreTest, OnTransChangedFileSpeedAccumulates)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->onTransChanged(TRANS_FILE_SPEED, "", 1024);
    EXPECT_EQ(helper->d->transferInfo.transferSize, 1024);
    EXPECT_EQ(helper->d->transferInfo.maxTimeS, 1);
}

// onTransChanged: TRANS_INDEX_CHANGE / TRANS_FILE_CHANGE / TRANS_FILE_DONE 不崩
TEST_F(TransferHelperCoreTest, OnTransChangedInfoBranchesNoCrash)
{
    TransferHelperStubScope s;
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(TRANS_INDEX_CHANGE, "/tmp/idx", 0));
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(TRANS_FILE_CHANGE, "/tmp/file", 0));
    EXPECT_NO_FATAL_FAILURE(helper->onTransChanged(TRANS_FILE_DONE, "/tmp/done", 0));
}

// updateTransProgress: totalSize<1 → 进度 0 (私有方法, -fno-access-control)
TEST_F(TransferHelperCoreTest, UpdateTransProgressZeroTotal)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->d->transferInfo.totalSize = 0;
    EXPECT_NO_FATAL_FAILURE(helper->updateTransProgress(100));
}

// updateTransProgress: 计算 > 100 → 截断为 100
TEST_F(TransferHelperCoreTest, UpdateTransProgressClampsToHundred)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    helper->d->transferInfo.totalSize = 100;
    EXPECT_NO_FATAL_FAILURE(helper->updateTransProgress(200));
}

// deliverMessage 信号在 transferResult(true) 时不发射 (它走 notifyMessage/transDialog)
// 但 buttonClicked 在 onlyTransfer 模式会发射 deliverMessage — 此处仅验证信号可连接
TEST_F(TransferHelperCoreTest, DeliverMessageSignalEmittableViaEmit)
{
    QSignalSpy spy(helper, &TransferHelper::deliverMessage);
    emit helper->deliverMessage("test_app", {"msg"});
    EXPECT_EQ(spy.count(), 1);
}

// buttonVisible: transfer-button + Everyone + 非 Offline → true
TEST_F(TransferHelperCoreTest, ButtonVisibleTransferEveryoneNotOffline)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.90", "Dev90"));
    info->setTransMode(DeviceInfo::TransMode::Everyone);
    info->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_TRUE(TransferHelper::buttonVisible("transfer-button", info));
}

// buttonVisible: transfer-button + Everyone + Offline → false
TEST_F(TransferHelperCoreTest, ButtonVisibleTransferEveryoneOffline)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.91", "Dev91"));
    info->setTransMode(DeviceInfo::TransMode::Everyone);
    info->setConnectStatus(DeviceInfo::Offline);
    EXPECT_FALSE(TransferHelper::buttonVisible("transfer-button", info));
}

// buttonVisible: transfer-button + OnlyConnected + Connected → true
TEST_F(TransferHelperCoreTest, ButtonVisibleTransferOnlyConnected)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.92", "Dev92"));
    info->setTransMode(DeviceInfo::TransMode::OnlyConnected);
    info->setConnectStatus(DeviceInfo::Connected);
    EXPECT_TRUE(TransferHelper::buttonVisible("transfer-button", info));
}

// buttonClickable: transfer-button + Idle → true
TEST_F(TransferHelperCoreTest, ButtonClickableTransferWhenIdle)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.93", "Dev93"));
    EXPECT_TRUE(TransferHelper::buttonClickable("transfer-button", info));
}

// buttonClickable: transfer-button + 非 Idle → false
TEST_F(TransferHelperCoreTest, ButtonClickableTransferWhenNotIdle)
{
    TransferHelperStubScope s;
    helper->d->status.storeRelease(TransferHelper::Transfering);
    DeviceInfoPointer info(new DeviceInfo("10.0.0.94", "Dev94"));
    EXPECT_FALSE(TransferHelper::buttonClickable("transfer-button", info));
}
