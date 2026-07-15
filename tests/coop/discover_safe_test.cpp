// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "lib/cooperation/core/discover/discovercontroller.h"
#include "lib/cooperation/core/discover/discovercontroller_p.h"
#include "lib/cooperation/core/discover/deviceinfo.h"
#include "lib/cooperation/core/global_defines.h"

using namespace cooperation_core;

namespace {
QString makeValidDeviceJson(const QString &ip, const QString &name)
{
    QVariantMap map;
    map.insert(AppSettings::IPAddress, ip);
    map.insert(AppSettings::DeviceNameKey, name);
    map.insert(AppSettings::DiscoveryModeKey, 0);
    map.insert(AppSettings::TransferModeKey, 0);
    map.insert(AppSettings::LinkDirectionKey, 0);
    map.insert(AppSettings::ClipboardShareKey, false);
    map.insert(AppSettings::PeripheralShareKey, false);
    map.insert(AppSettings::CooperationEnabled, true);
    map.insert(AppSettings::OSType, 0);

    QJsonObject obj = QJsonObject::fromVariantMap(map);
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}
}   // namespace

class DiscoverSafeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto *c = DiscoverController::instance();
        c->_historyDevices.clear();
        c->d->onlineDeviceList.clear();
        c->d->historyDeviceMap.clear();
        c->d->searchDevice.reset();
    }
};

TEST_F(DiscoverSafeTest, UpdatePublishNoopBeforeZeroConfInit)
{
    EXPECT_NO_FATAL_FAILURE(DiscoverController::instance()->updatePublish());
}

TEST_F(DiscoverSafeTest, RefreshEmitsDiscoveryFinishedFalseBeforeInit)
{
    DiscoverController *c = DiscoverController::instance();
    int calls = 0;
    bool lastOk = true;
    QObject sink;
    QObject::connect(c, &DiscoverController::discoveryFinished, &sink, [&](bool ok) {
        ++calls;
        lastOk = ok;
    });

    EXPECT_NO_FATAL_FAILURE(c->refresh());
    EXPECT_GE(calls, 1);
    EXPECT_FALSE(lastOk);
}

TEST_F(DiscoverSafeTest, StartDiscoverEmitsDiscoveryFinishedFalseBeforeInit)
{
    DiscoverController *c = DiscoverController::instance();
    int calls = 0;
    bool lastOk = true;
    QObject sink;
    QObject::connect(c, &DiscoverController::discoveryFinished, &sink, [&](bool ok) {
        ++calls;
        lastOk = ok;
    });

    EXPECT_NO_FATAL_FAILURE(c->startDiscover());
    EXPECT_GE(calls, 1);
    EXPECT_FALSE(lastOk);
}

TEST_F(DiscoverSafeTest, UnpublishEmitsCompatUnregistration)
{
    DiscoverController *c = DiscoverController::instance();
    int calls = 0;
    bool lastReg = true;
    QString lastJson = "sentinel";
    QObject sink;
    QObject::connect(c, &DiscoverController::registCompatAppInfo, &sink, [&](bool reg, const QString &json) {
        ++calls;
        lastReg = reg;
        lastJson = json;
    });

    EXPECT_NO_FATAL_FAILURE(c->unpublish());
    EXPECT_GE(calls, 1);
    EXPECT_FALSE(lastReg);
    EXPECT_TRUE(lastJson.isEmpty());
}

TEST_F(DiscoverSafeTest, SelfInfoReturnsNonNullDeviceInfo)
{
    DeviceInfoPointer info = DiscoverController::selfInfo();
    ASSERT_NE(info, nullptr);
}

TEST_F(DiscoverSafeTest, ParseDeviceJsonValidReturnsConnectableDevice)
{
    DiscoverController *c = DiscoverController::instance();
    QString json = makeValidDeviceJson("172.16.0.9", "HostZ");
    DeviceInfoPointer info = c->parseDeviceJson(json);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->ipAddress().toStdString(), "172.16.0.9");
    EXPECT_EQ(info->deviceName().toStdString(), "HostZ");
    EXPECT_EQ(info->connectStatus(), DeviceInfo::ConnectStatus::Connectable);
}

TEST_F(DiscoverSafeTest, ParseDeviceJsonInvalidReturnsNull)
{
    DiscoverController *c = DiscoverController::instance();
    EXPECT_EQ(c->parseDeviceJson(QString("not-valid-json")), nullptr);
}

TEST_F(DiscoverSafeTest, IsValidDeviceRejectsNullAndEmptyIp)
{
    DiscoverController *c = DiscoverController::instance();
    EXPECT_FALSE(c->isVaildDevice(nullptr));
    DeviceInfoPointer emptyIp(new DeviceInfo("", "NoIp"));
    EXPECT_FALSE(c->isVaildDevice(emptyIp));
}

TEST_F(DiscoverSafeTest, IsValidDeviceAcceptsHistoryDeviceWithoutIpFilter)
{
    DiscoverController *c = DiscoverController::instance();
    c->_historyDevices.append("198.51.100.7");
    DeviceInfoPointer hist(new DeviceInfo("198.51.100.7", "Hist"));
    EXPECT_TRUE(c->isVaildDevice(hist));
}

TEST_F(DiscoverSafeTest, OnlineDeviceListStartsEmpty)
{
    EXPECT_TRUE(DiscoverController::instance()->getOnlineDeviceList().isEmpty());
}

TEST_F(DiscoverSafeTest, FindDeviceByUnknownIPReturnsNull)
{
    EXPECT_EQ(DiscoverController::instance()->findDeviceByIP("0.0.0.0"), nullptr);
}
