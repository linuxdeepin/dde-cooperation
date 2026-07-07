// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>
#include "lib/cooperation/core/net/transferwrapper.h"
#include "lib/cooperation/core/discover/deviceinfo.h"

using cooperation_core::TransferWrapper;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

// ============ 单例身份 ============

TEST(TransferWrapperTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(TransferWrapper::instance(), TransferWrapper::instance());
}

// ============ public slot:DoesNotCrash(委托给控制器,可能触发发现/网络) ============
// 用 EXPECT_NO_FATAL_FAILURE 包裹,避免后台异常带垮整个测试进程。

TEST(TransferWrapperTest, OnRefreshDeviceDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(TransferWrapper::instance()->onRefreshDevice());
}

TEST(TransferWrapperTest, OnSearchDeviceWithInvalidIpDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(TransferWrapper::instance()->onSearchDevice("0.0.0.0"));
}

TEST(TransferWrapperTest, OnSendFilesWithEmptyListDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(
        TransferWrapper::instance()->onSendFiles("10.0.0.1", "Dev", QStringList{}));
}

// ============ 私有 slot:onDeviceOnline 三分支(纯信号转发) ============
// -fno-access-control 允许直接调用 private slot。

// 空 infoList → devList 为空 → 早返回,不发任何信号。
TEST(TransferWrapperTest, OnDeviceOnlineEmptyListEmitsNothing)
{
    QSignalSpy searchedSpy(TransferWrapper::instance(), &TransferWrapper::searched);
    QSignalSpy refreshedSpy(TransferWrapper::instance(), &TransferWrapper::refreshed);
    TransferWrapper::instance()->onDeviceOnline(QList<DeviceInfoPointer>{});
    EXPECT_EQ(searchedSpy.count(), 0);
    EXPECT_EQ(refreshedSpy.count(), 0);
}

// 单设备(size<2) → 发 searched,负载是该设备的 JSON 字符串。
TEST(TransferWrapperTest, OnDeviceOnlineSingleDeviceEmitsSearched)
{
    QSignalSpy searchedSpy(TransferWrapper::instance(), &TransferWrapper::searched);
    QSignalSpy refreshedSpy(TransferWrapper::instance(), &TransferWrapper::refreshed);
    QList<DeviceInfoPointer> list{ DeviceInfoPointer(new DeviceInfo("10.0.0.1", "DevA")) };
    TransferWrapper::instance()->onDeviceOnline(list);
    EXPECT_EQ(searchedSpy.count(), 1);
    EXPECT_EQ(refreshedSpy.count(), 0);
    // 负载是 JSON,包含我们设置的 IP。
    QString payload = searchedSpy.takeFirst().at(0).toString();
    EXPECT_TRUE(payload.contains("10.0.0.1")) << payload.toStdString();
}

// 多设备(size>=2) → 发 refreshed,负载是 QStringList。
TEST(TransferWrapperTest, OnDeviceOnlineMultipleDevicesEmitsRefreshed)
{
    QSignalSpy searchedSpy(TransferWrapper::instance(), &TransferWrapper::searched);
    QSignalSpy refreshedSpy(TransferWrapper::instance(), &TransferWrapper::refreshed);
    QList<DeviceInfoPointer> list{
        DeviceInfoPointer(new DeviceInfo("10.0.0.1", "DevA")),
        DeviceInfoPointer(new DeviceInfo("10.0.0.2", "DevB")),
    };
    TransferWrapper::instance()->onDeviceOnline(list);
    EXPECT_EQ(searchedSpy.count(), 0);
    EXPECT_EQ(refreshedSpy.count(), 1);
    QStringList payload = refreshedSpy.takeFirst().at(0).toStringList();
    EXPECT_EQ(payload.size(), 2);
}

// ============ 私有 slot:onDeviceOffline / onFinishedDiscovery ============

TEST(TransferWrapperTest, OnDeviceOfflineEmitsDeviceChanged)
{
    QSignalSpy spy(TransferWrapper::instance(), &TransferWrapper::deviceChanged);
    TransferWrapper::instance()->onDeviceOffline("10.0.0.9");
    EXPECT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_FALSE(args.at(0).toBool());          // found = false
    EXPECT_EQ(args.at(1).toString().toStdString(), "10.0.0.9");
}

// hasFound=false → 发空 searched("")。
TEST(TransferWrapperTest, OnFinishedDiscoveryFalseEmitsEmptySearched)
{
    QSignalSpy spy(TransferWrapper::instance(), &TransferWrapper::searched);
    TransferWrapper::instance()->onFinishedDiscovery(false);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString().toStdString(), "");
}

// hasFound=true → 不发 searched(无操作分支)。
TEST(TransferWrapperTest, OnFinishedDiscoveryTrueEmitsNothing)
{
    QSignalSpy spy(TransferWrapper::instance(), &TransferWrapper::searched);
    TransferWrapper::instance()->onFinishedDiscovery(true);
    EXPECT_EQ(spy.count(), 0);
}

// ============ 私有方法:variantMapToQString ============

TEST(TransferWrapperTest, VariantMapToQStringProducesValidJson)
{
    QVariantMap map;
    map.insert("ip", "10.0.0.1");
    map.insert("name", "DevA");
    QString json = TransferWrapper::instance()->variantMapToQString(map);
    ASSERT_FALSE(json.isEmpty());
    QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    EXPECT_EQ(obj.value("ip").toString().toStdString(), "10.0.0.1");
    EXPECT_EQ(obj.value("name").toString().toStdString(), "DevA");
}

TEST(TransferWrapperTest, VariantMapToQStringEmptyMapProducesEmptyJson)
{
    QString json = TransferWrapper::instance()->variantMapToQString(QVariantMap{});
    QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    EXPECT_TRUE(obj.isEmpty());
}
