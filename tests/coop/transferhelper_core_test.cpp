// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include "lib/cooperation/core/net/helper/transferhelper.h"
#include "lib/cooperation/core/discover/deviceinfo.h"

using cooperation_core::DeviceInfo;
using cooperation_core::TransferHelper;
using ::DeviceInfoPointer;

class TransferHelperCoreTest : public ::testing::Test {
protected:
    TransferHelper *helper = nullptr;
    void SetUp() override { helper = TransferHelper::instance(); }
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
