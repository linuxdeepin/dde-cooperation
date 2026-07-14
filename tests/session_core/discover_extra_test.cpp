// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "discover/discovercontroller.h"
#include "discover/discovercontroller_p.h"
#include "discover/deviceinfo.h"
#include "global_defines.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QList>
#include <QMap>

using namespace cooperation_core;

// These tests use the DiscoverController singleton's data manipulation methods
// that don't emit Qt signals (avoiding crash-prone signal chains).
class DiscoverExtraTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto *ctrl = DiscoverController::instance();
        ctrl->_historyDevices.clear();
        ctrl->d->onlineDeviceList.clear();
        ctrl->d->historyDeviceMap.clear();
        ctrl->d->searchDevice.reset();
        ctrl->_connectedDevice = "";
        ctrl->d->ipfilter = "";
    }
    void TearDown() override
    {
        auto *ctrl = DiscoverController::instance();
        ctrl->_historyDevices.clear();
        ctrl->d->onlineDeviceList.clear();
        ctrl->d->historyDeviceMap.clear();
        ctrl->d->searchDevice.reset();
        ctrl->_connectedDevice = "";
    }
};

TEST_F(DiscoverExtraTest, OnDConfigValueChangedWrongConfig)
{
    DiscoverController::instance()->onDConfigValueChanged("wrong_config", "some_key");
    SUCCEED();
}

TEST_F(DiscoverExtraTest, OnAppAttributeChangedWrongGroup)
{
    DiscoverController::instance()->onAppAttributeChanged("wrong_group", "key", "value");
    SUCCEED();
}

TEST_F(DiscoverExtraTest, FindDeviceByIPEmpty)
{
    auto result = DiscoverController::instance()->findDeviceByIP("1.2.3.4");
    EXPECT_EQ(result, nullptr);
}

TEST_F(DiscoverExtraTest, IsVaildDeviceNull)
{
    DeviceInfoPointer nullDev;
    EXPECT_FALSE(DiscoverController::instance()->isVaildDevice(nullDev));
}

TEST_F(DiscoverExtraTest, IsVaildDeviceEmptyIP)
{
    DeviceInfoPointer emptyIp(new DeviceInfo("", "EmptyIP"));
    EXPECT_FALSE(DiscoverController::instance()->isVaildDevice(emptyIp));
}

TEST_F(DiscoverExtraTest, IsVaildDeviceWithFilter)
{
    auto *ctrl = DiscoverController::instance();
    ctrl->d->ipfilter = "192.168.1";
    ctrl->_historyDevices.clear();

    DeviceInfoPointer valid(new DeviceInfo("192.168.1.10", "Valid"));
    EXPECT_TRUE(ctrl->isVaildDevice(valid));

    DeviceInfoPointer wrongSubnet(new DeviceInfo("10.0.0.10", "WrongSubnet"));
    EXPECT_FALSE(ctrl->isVaildDevice(wrongSubnet));
}

TEST_F(DiscoverExtraTest, IsVaildDeviceHistoryBypass)
{
    auto *ctrl = DiscoverController::instance();
    ctrl->d->ipfilter = "192.168.1";
    ctrl->_historyDevices.append("10.0.0.50");

    DeviceInfoPointer histDev(new DeviceInfo("10.0.0.50", "HistoryDev"));
    EXPECT_TRUE(ctrl->isVaildDevice(histDev));
}

TEST_F(DiscoverExtraTest, CollectOfflineHistoryDevicesEmpty)
{
    DiscoverController::instance()->collectOfflineHistoryDevices();
    EXPECT_EQ(DiscoverController::instance()->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverExtraTest, CollectOfflineHistoryDevicesAdd)
{
    auto *ctrl = DiscoverController::instance();
    ctrl->_historyDevices.append("192.168.1.40");
    ctrl->d->historyDeviceMap["192.168.1.40"] = "OfflineDev";
    ctrl->collectOfflineHistoryDevices();
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 1);
}

TEST_F(DiscoverExtraTest, CollectOfflineHistoryDevicesSkipExisting)
{
    auto *ctrl = DiscoverController::instance();
    DeviceInfoPointer existing(new DeviceInfo("192.168.1.41", "Existing"));
    ctrl->d->onlineDeviceList.append(existing);
    ctrl->_historyDevices.append("192.168.1.41");
    ctrl->d->historyDeviceMap["192.168.1.41"] = "Existing";
    ctrl->collectOfflineHistoryDevices();
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 1);
}

TEST_F(DiscoverExtraTest, AddSearchDeviceIfExistsNull)
{
    DiscoverController::instance()->addSearchDeviceIfExists();
    EXPECT_EQ(DiscoverController::instance()->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverExtraTest, AddSearchDeviceIfExistsAdd)
{
    auto *ctrl = DiscoverController::instance();
    ctrl->d->searchDevice = DeviceInfoPointer(new DeviceInfo("10.0.0.99", "SearchDev"));
    ctrl->addSearchDeviceIfExists();
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 1);
}

TEST_F(DiscoverExtraTest, ParseDeviceJsonInvalid)
{
    auto dev = DiscoverController::instance()->parseDeviceJson("not a json");
    EXPECT_EQ(dev, nullptr);
}
