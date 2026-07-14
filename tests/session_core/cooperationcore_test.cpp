// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "cooperationcoreplugin.h"
#include "discover/discovercontroller.h"
#include "discover/discovercontroller_p.h"
#include "discover/deviceinfo.h"
#include "utils/cooperationutil.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QTimer>

using namespace cooperation_core;

// CooperationCorePlugin constructor initializes MainWindow (GUI) and singletons,
// so we only test the non-GUI methods that don't require full initialization.

TEST(CooperationCoreTest, IsMinilizeNoArgs)
{
    auto args = qApp->arguments();
    bool hasMinimize = args.size() == 2 && args.contains("-m");
    EXPECT_FALSE(hasMinimize);
}

TEST(CooperationCoreTest, DeviceInfoVariantMapRoundtrip)
{
    QVariantMap map;
    map.insert(AppSettings::IPAddress, "192.168.1.50");
    map.insert(AppSettings::DeviceNameKey, "TestDevice");
    map.insert(AppSettings::DiscoveryModeKey, 0);
    map.insert(AppSettings::TransferModeKey, 0);
    map.insert(AppSettings::LinkDirectionKey, 0);
    map.insert(AppSettings::ClipboardShareKey, false);
    map.insert(AppSettings::PeripheralShareKey, false);
    map.insert(AppSettings::CooperationEnabled, true);
    map.insert(AppSettings::OSType, 0);

    auto info = DeviceInfo::fromVariantMap(map);
    EXPECT_EQ(info->ipAddress(), "192.168.1.50");
    EXPECT_EQ(info->deviceName(), "TestDevice");
}

TEST(CooperationCoreTest, DiscoverControllerParseDeviceJsonValid)
{
    QVariantMap map;
    map.insert(AppSettings::IPAddress, "10.0.0.1");
    map.insert(AppSettings::DeviceNameKey, "TestPC");
    map.insert(AppSettings::DiscoveryModeKey, 0);
    map.insert(AppSettings::TransferModeKey, 0);
    map.insert(AppSettings::LinkDirectionKey, 0);
    map.insert(AppSettings::ClipboardShareKey, false);
    map.insert(AppSettings::PeripheralShareKey, false);
    map.insert(AppSettings::CooperationEnabled, true);
    map.insert(AppSettings::OSType, 0);

    auto json = QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact));
    auto dev = DiscoverController::instance()->parseDeviceJson(json);
    EXPECT_NE(dev, nullptr);
    EXPECT_EQ(dev->ipAddress(), "10.0.0.1");
}

TEST(CooperationCoreTest, DiscoverControllerParseDeviceJsonInvalid)
{
    auto dev = DiscoverController::instance()->parseDeviceJson("not a json");
    EXPECT_EQ(dev, nullptr);
}
