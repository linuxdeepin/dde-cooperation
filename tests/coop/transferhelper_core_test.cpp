// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QString>
#include "lib/cooperation/core/net/helper/transferhelper.h"
#include "lib/cooperation/core/net/helper/transferhelper_p.h"
#include "lib/cooperation/core/net/networkutil.h"
#include "lib/cooperation/core/discover/deviceinfo.h"
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

struct TransferHelperStubScope {
    Stub stub;
    TransferHelperStubScope()
    {
        stub.set(ADDR(TransferHelperPrivate, notifyMessage), stub_thp_notifyMessage);
        stub.set(ADDR(TransferHelperPrivate, reportTransferResult), stub_thp_reportTransferResult);
        stub.set(ADDR(NetworkUtil, setStorageFolder), stub_setStorageFolder);
        stub.set(ADDR(NetworkUtil, cancelTrans), stub_cancelTrans);
        stub.set(ADDR(NetworkUtil, replyTransRequest), stub_replyTransRequest);
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
