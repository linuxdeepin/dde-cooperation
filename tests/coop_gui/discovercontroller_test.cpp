// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QVariantMap>
#include <QMap>
#include <QVariant>
#include "discover/discovercontroller.h"
#include "discover/deviceinfo.h"

using cooperation_core::DiscoverController;
using ::DeviceInfoPointer;

// 只测非网络方法;init/publish/refresh/startDiscover 等 ZeroConf 网络操作跳过。

TEST(DiscoverControllerTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(DiscoverController::instance(), DiscoverController::instance());
}

TEST(DiscoverControllerTest, SelfInfoReturnsNonNull)
{
    DeviceInfoPointer info = DiscoverController::selfInfo();
    EXPECT_NE(info, nullptr);
}

TEST(DiscoverControllerTest, IsZeroConfDaemonActiveReturnsBool)
{
    EXPECT_NO_FATAL_FAILURE({ bool v = DiscoverController::isZeroConfDaemonActive(); (void)v; });
}

// 空在线列表 → 未知 IP 返回 null。
TEST(DiscoverControllerTest, FindDeviceByUnknownIpReturnsNull)
{
    EXPECT_EQ(DiscoverController::instance()->findDeviceByIP("0.0.0.0"), nullptr);
}

// 非匹配 group → 早返回。
TEST(DiscoverControllerTest, OnAppAttributeChangedWrongGroupDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(
        DiscoverController::instance()->onAppAttributeChanged("WrongGroup", "key", QVariant()));
}

TEST(DiscoverControllerTest, OnConnectHistoryUpdatedDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(DiscoverController::instance()->onConnectHistoryUpdated());
}

TEST(DiscoverControllerTest, UpdateHistoryDevicesEmptyMapDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(
        DiscoverController::instance()->updateHistoryDevices(QMap<QString, QString>{}));
}
