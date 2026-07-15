// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

#include "helper/transferhelper.h"
#include "helper/transferhelper_p.h"
#include "discover/deviceinfo.h"
#include "global_defines.h"

using namespace cooperation_transfer;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

// Helper: build valid device JSON
static QString makeDevJson(const QString &ip, const QString &name,
                           int transMode = 0, int discoveryMode = 0)
{
    QVariantMap map;
    map.insert(AppSettings::IPAddress, ip);
    map.insert(AppSettings::DeviceNameKey, name);
    map.insert(AppSettings::DiscoveryModeKey, discoveryMode);
    map.insert(AppSettings::TransferModeKey, transMode);
    map.insert(AppSettings::LinkDirectionKey, 0);
    map.insert(AppSettings::ClipboardShareKey, false);
    map.insert(AppSettings::PeripheralShareKey, false);
    map.insert(AppSettings::CooperationEnabled, true);
    map.insert(AppSettings::OSType, 0);
    return QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact));
}

// ============ parseFromJson ============

TEST(TransferHelperCtTest, ParseFromJsonValid)
{
    auto *h = TransferHelper::instance();
    auto json = makeDevJson("192.168.1.50", "DevA");
    auto dev = h->parseFromJson(json);
    EXPECT_NE(dev, nullptr);
    EXPECT_EQ(dev->ipAddress(), "192.168.1.50");
    EXPECT_EQ(dev->deviceName(), "DevA");
}

TEST(TransferHelperCtTest, ParseFromJsonInvalid)
{
    auto *h = TransferHelper::instance();
    auto dev = h->parseFromJson("not a json");
    EXPECT_EQ(dev, nullptr);
}

TEST(TransferHelperCtTest, ParseFromJsonEmpty)
{
    auto *h = TransferHelper::instance();
    auto dev = h->parseFromJson("");
    EXPECT_EQ(dev, nullptr);
}

// ============ transable ============

TEST(TransferHelperCtTest, TransableNullDevice)
{
    auto *h = TransferHelper::instance();
    DeviceInfoPointer nullDev;
    EXPECT_FALSE(h->transable(nullDev));
}

TEST(TransferHelperCtTest, TransableInvalidDevice)
{
    auto *h = TransferHelper::instance();
    DeviceInfoPointer invalid(new DeviceInfo("", ""));
    EXPECT_FALSE(h->transable(invalid));
}

TEST(TransferHelperCtTest, TransableEveryoneMode)
{
    auto *h = TransferHelper::instance();
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.1", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::Everyone);
    EXPECT_TRUE(h->transable(dev));
}

TEST(TransferHelperCtTest, TransableOnlyConnectedModeConnected)
{
    auto *h = TransferHelper::instance();
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.2", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::OnlyConnected);
    dev->setConnectStatus(DeviceInfo::Connected);
    EXPECT_TRUE(h->transable(dev));
}

TEST(TransferHelperCtTest, TransableOnlyConnectedModeNotConnected)
{
    auto *h = TransferHelper::instance();
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.3", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::OnlyConnected);
    dev->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_FALSE(h->transable(dev));
}

TEST(TransferHelperCtTest, TransableNotAllowMode)
{
    auto *h = TransferHelper::instance();
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.4", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::NotAllow);
    // NotAllow returns true (device shown but button disabled)
    EXPECT_TRUE(h->transable(dev));
}

// ============ buttonVisible ============

TEST(TransferHelperCtTest, ButtonVisibleEveryoneNotOffline)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.5", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::Everyone);
    dev->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_TRUE(TransferHelper::buttonVisible("transfer-button", dev));
}

TEST(TransferHelperCtTest, ButtonVisibleEveryoneOffline)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.6", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::Everyone);
    dev->setConnectStatus(DeviceInfo::Offline);
    EXPECT_FALSE(TransferHelper::buttonVisible("transfer-button", dev));
}

TEST(TransferHelperCtTest, ButtonVisibleOnlyConnectedConnected)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.7", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::OnlyConnected);
    dev->setConnectStatus(DeviceInfo::Connected);
    EXPECT_TRUE(TransferHelper::buttonVisible("transfer-button", dev));
}

TEST(TransferHelperCtTest, ButtonVisibleOnlyConnectedNotConnected)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.8", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::OnlyConnected);
    dev->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_FALSE(TransferHelper::buttonVisible("transfer-button", dev));
}

TEST(TransferHelperCtTest, ButtonVisibleNotAllow)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.9", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::NotAllow);
    EXPECT_FALSE(TransferHelper::buttonVisible("transfer-button", dev));
}

TEST(TransferHelperCtTest, ButtonVisibleUnknownId)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.10", "Dev"));
    dev->setTransMode(DeviceInfo::TransMode::Everyone);
    dev->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_TRUE(TransferHelper::buttonVisible("other-button", dev));
}

// ============ buttonClickable ============

TEST(TransferHelperCtTest, ButtonClickableAlwaysTrue)
{
    DeviceInfoPointer dev(new DeviceInfo("10.0.0.11", "Dev"));
    EXPECT_TRUE(TransferHelper::buttonClickable("transfer-button", dev));
    EXPECT_TRUE(TransferHelper::buttonClickable("any-id", dev));
}

// ============ searchResultSlot ============

TEST(TransferHelperCtTest, SearchResultSlotEmptyInfo)
{
    auto *h = TransferHelper::instance();
    QSignalSpy spy(h, &TransferHelper::finishDiscovery);
    h->searchResultSlot("");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

TEST(TransferHelperCtTest, SearchResultSlotValidTransable)
{
    auto *h = TransferHelper::instance();
    QSignalSpy onlineSpy(h, &TransferHelper::onlineDevices);
    auto json = makeDevJson("192.168.1.60", "TransableDev");
    h->searchResultSlot(json);
    EXPECT_EQ(onlineSpy.count(), 1);
}

TEST(TransferHelperCtTest, SearchResultSlotNonTransable)
{
    auto *h = TransferHelper::instance();
    QSignalSpy offlineSpy(h, &TransferHelper::offlineDevice);
    // OnlyConnected mode but not connected → not transable → offlineDevice signal
    auto json = makeDevJson("192.168.1.61", "NonTransable", 1);
    h->searchResultSlot(json);
    EXPECT_EQ(offlineSpy.count(), 1);
}

TEST(TransferHelperCtTest, SearchResultSlotInvalidJson)
{
    auto *h = TransferHelper::instance();
    QSignalSpy onlineSpy(h, &TransferHelper::onlineDevices);
    QSignalSpy offlineSpy(h, &TransferHelper::offlineDevice);
    h->searchResultSlot("invalid json");
    // parseFromJson returns nullptr → devInfo is null → neither branch taken
    EXPECT_EQ(onlineSpy.count(), 0);
    EXPECT_EQ(offlineSpy.count(), 0);
}

// ============ refreshResultSlot ============

TEST(TransferHelperCtTest, RefreshResultSlotEmpty)
{
    auto *h = TransferHelper::instance();
    QSignalSpy finishSpy(h, &TransferHelper::finishDiscovery);
    h->refreshResultSlot(QStringList{});
    EXPECT_EQ(finishSpy.count(), 1);
    EXPECT_FALSE(finishSpy.takeFirst().at(0).toBool());
}

TEST(TransferHelperCtTest, RefreshResultSlotWithDevices)
{
    auto *h = TransferHelper::instance();
    QSignalSpy onlineSpy(h, &TransferHelper::onlineDevices);
    QSignalSpy finishSpy(h, &TransferHelper::finishDiscovery);
    auto json1 = makeDevJson("10.0.0.20", "Dev1");
    auto json2 = makeDevJson("10.0.0.21", "Dev2");
    h->refreshResultSlot(QStringList{json1, json2});
    EXPECT_EQ(onlineSpy.count(), 1);
    EXPECT_EQ(finishSpy.count(), 1);
    EXPECT_TRUE(finishSpy.takeFirst().at(0).toBool());
}

TEST(TransferHelperCtTest, RefreshResultSlotWithInvalidDevices)
{
    auto *h = TransferHelper::instance();
    QSignalSpy finishSpy(h, &TransferHelper::finishDiscovery);
    h->refreshResultSlot(QStringList{"invalid json", "also invalid"});
    EXPECT_EQ(finishSpy.count(), 1);
    EXPECT_FALSE(finishSpy.takeFirst().at(0).toBool());
}

// ============ deviceChangedSlot ============

TEST(TransferHelperCtTest, DeviceChangedSlotFound)
{
    auto *h = TransferHelper::instance();
    QSignalSpy onlineSpy(h, &TransferHelper::onlineDevices);
    auto json = makeDevJson("10.0.0.30", "FoundDev");
    h->deviceChangedSlot(true, json);
    EXPECT_EQ(onlineSpy.count(), 1);
}

TEST(TransferHelperCtTest, DeviceChangedSlotNotFound)
{
    auto *h = TransferHelper::instance();
    QSignalSpy offlineSpy(h, &TransferHelper::offlineDevice);
    h->deviceChangedSlot(false, "10.0.0.31");
    EXPECT_EQ(offlineSpy.count(), 1);
    EXPECT_EQ(offlineSpy.takeFirst().at(0).toString(), "10.0.0.31");
}

// ============ timeWatchBackend ============

TEST(TransferHelperCtTest, TimeWatchBackendDoesNotCrash)
{
    auto *h = TransferHelper::instance();
    EXPECT_NO_FATAL_FAILURE(h->timeWatchBackend());
}

// ============ instance ============

TEST(TransferHelperCtTest, InstanceSingleton)
{
    EXPECT_EQ(TransferHelper::instance(), TransferHelper::instance());
}
