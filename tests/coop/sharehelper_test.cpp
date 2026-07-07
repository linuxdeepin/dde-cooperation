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
#include "lib/cooperation/core/discover/deviceinfo.h"

using cooperation_core::DeviceInfo;
using cooperation_core::ShareHelper;
using ::DeviceInfoPointer;

class ShareHelperTest : public ::testing::Test {
protected:
    ShareHelper *helper = nullptr;
    void SetUp() override { helper = ShareHelper::instance(); }
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
