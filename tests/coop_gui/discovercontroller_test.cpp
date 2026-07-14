// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QVariantMap>
#include <QMap>
#include <QVariant>
#include <QSignalSpy>
#include "discover/discovercontroller.h"
#include "discover/discovercontroller_p.h"
#include "discover/deviceinfo.h"

using cooperation_core::DiscoverController;
using cooperation_core::DiscoverControllerPrivate;
using cooperation_core::DeviceInfo;
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

// ===== 以下为状态相关测试: 使用 fixture 在 TearDown 中复位单例状态, 避免跨用例泄漏 =====
// -fno-access-control 允许直接访问 d->onlineDeviceList / _historyDevices / _connectedDevice
//   等私有成员, 并调用 parseDeviceJson / isVaildDevice / deviceLosted / finishDiscovery 等私有方法。

class DiscoverControllerStateTest : public ::testing::Test {
protected:
    DiscoverController *ctrl = nullptr;
    void SetUp() override { ctrl = DiscoverController::instance(); }
    void TearDown() override
    {
        ctrl->d->onlineDeviceList.clear();
        ctrl->d->historyDeviceMap.clear();
        ctrl->d->searchDevice.reset();
        ctrl->_historyDevices.clear();
        ctrl->_connectedDevice.clear();
    }
};

// parseDeviceJson: 合法 JSON → 非空, Connectable 状态
TEST_F(DiscoverControllerStateTest, ParseDeviceJsonValidReturnsNonNull)
{
    QString json = QStringLiteral("{\"IPAddress\":\"10.0.0.5\",\"DeviceName\":\"dev5\"}");
    auto info = ctrl->parseDeviceJson(json);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->ipAddress(), "10.0.0.5");
    EXPECT_EQ(info->connectStatus(), DeviceInfo::Connectable);
}

// parseDeviceJson: 非法 JSON → null
TEST_F(DiscoverControllerStateTest, ParseDeviceJsonInvalidReturnsNull)
{
    EXPECT_EQ(ctrl->parseDeviceJson(QStringLiteral("not json")), nullptr);
    EXPECT_EQ(ctrl->parseDeviceJson(QString()), nullptr);
}

// isVaildDevice: null → false
TEST_F(DiscoverControllerStateTest, IsVaildDeviceNullReturnsFalse)
{
    EXPECT_FALSE(ctrl->isVaildDevice(nullptr));
}

// isVaildDevice: 空 IP → false
TEST_F(DiscoverControllerStateTest, IsVaildDeviceEmptyIpReturnsFalse)
{
    DeviceInfoPointer info(new DeviceInfo("", "empty"));
    EXPECT_FALSE(ctrl->isVaildDevice(info));
}

// isVaildDevice: 历史设备 (ipfilter 旁路) → true
TEST_F(DiscoverControllerStateTest, IsVaildDeviceInHistoryReturnsTrue)
{
    QMap<QString, QString> hist;
    hist.insert("10.0.0.99", "histdev");
    ctrl->updateHistoryDevices(hist);
    DeviceInfoPointer info(new DeviceInfo("10.0.0.99", "histdev"));
    EXPECT_TRUE(ctrl->isVaildDevice(info));
}

// isVaildDevice: 非历史, ipfilter 默认空 → 任意 IP 通过
TEST_F(DiscoverControllerStateTest, IsVaildDeviceWithIpReturnsTrue)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.50", "dev"));
    EXPECT_TRUE(ctrl->isVaildDevice(info));
}

// updateDeviceState: 追加到 onlineDeviceList
TEST_F(DiscoverControllerStateTest, UpdateDeviceStateAddsToOnlineList)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.10", "dev10"));
    info->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info);
    auto list = ctrl->getOnlineDeviceList();
    EXPECT_EQ(list.size(), 1);
    EXPECT_EQ(list.first()->ipAddress(), "10.0.0.10");
}

// updateDeviceState: Connected → 记录 _connectedDevice
TEST_F(DiscoverControllerStateTest, UpdateDeviceStateConnectedSetsConnectedDevice)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.11", "dev11"));
    info->setConnectStatus(DeviceInfo::Connected);
    ctrl->updateDeviceState(info);
    EXPECT_EQ(ctrl->_connectedDevice, "10.0.0.11");
}

// updateDeviceState: 非 Connected → 清空 _connectedDevice
TEST_F(DiscoverControllerStateTest, UpdateDeviceStateNonConnectedClearsConnectedDevice)
{
    ctrl->_connectedDevice = "10.0.0.12";
    DeviceInfoPointer info(new DeviceInfo("10.0.0.13", "dev13"));
    info->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info);
    EXPECT_EQ(ctrl->_connectedDevice, "");
}

// updateDeviceState: 已存在的 IP → 替换 (removeOne + append)
TEST_F(DiscoverControllerStateTest, UpdateDeviceStateReplacesExisting)
{
    DeviceInfoPointer info1(new DeviceInfo("10.0.0.14", "old"));
    info1->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info1);
    DeviceInfoPointer info2(new DeviceInfo("10.0.0.14", "new"));
    info2->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info2);
    EXPECT_EQ(ctrl->getOnlineDeviceList().size(), 1);
    EXPECT_EQ(ctrl->findDeviceByIP("10.0.0.14")->deviceName(), "new");
}

// findDeviceByIP: 找到
TEST_F(DiscoverControllerStateTest, FindDeviceByIPReturnsDevice)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.20", "dev20"));
    info->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info);
    auto found = ctrl->findDeviceByIP("10.0.0.20");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->deviceName(), "dev20");
}

// deviceLosted: 非历史设备 → 从 onlineDeviceList 移除
TEST_F(DiscoverControllerStateTest, DeviceLostedRemovesFromList)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.30", "dev30"));
    info->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info);
    ASSERT_NE(ctrl->findDeviceByIP("10.0.0.30"), nullptr);
    ctrl->deviceLosted("10.0.0.30");
    EXPECT_EQ(ctrl->findDeviceByIP("10.0.0.30"), nullptr);
}

// deviceLosted: 历史设备 → 状态置 Offline, 保留在列表
TEST_F(DiscoverControllerStateTest, DeviceLostedHistoryDeviceSetsOffline)
{
    QMap<QString, QString> hist;
    hist.insert("10.0.0.31", "histdev31");
    ctrl->updateHistoryDevices(hist);
    ASSERT_NE(ctrl->findDeviceByIP("10.0.0.31"), nullptr);
    ctrl->deviceLosted("10.0.0.31");
    auto found = ctrl->findDeviceByIP("10.0.0.31");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->connectStatus(), DeviceInfo::Offline);
}

// deviceLosted: 未知 IP → 发出 deviceOffline 信号
TEST_F(DiscoverControllerStateTest, DeviceLostedEmitsDeviceOffline)
{
    QSignalSpy spy(ctrl, &DiscoverController::deviceOffline);
    ctrl->deviceLosted("10.0.0.32");
    EXPECT_EQ(spy.count(), 1);
}

// updateHistoryDevices: 非空 map → 追加 Offline 设备 + 发出 deviceOnline
TEST_F(DiscoverControllerStateTest, UpdateHistoryDevicesAddsOfflineDevices)
{
    QMap<QString, QString> hist;
    hist.insert("10.0.0.40", "histdev40");
    hist.insert("10.0.0.41", "histdev41");
    QSignalSpy spy(ctrl, &DiscoverController::deviceOnline);
    ctrl->updateHistoryDevices(hist);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(ctrl->_historyDevices.size(), 2);
    EXPECT_EQ(ctrl->findDeviceByIP("10.0.0.40")->connectStatus(), DeviceInfo::Offline);
}

// onAppAttributeChanged: 正确 group + StoragePathKey → 不崩 (updatePublish 因 zeroConf 为空早返回)
TEST_F(DiscoverControllerStateTest, OnAppAttributeChangedStoragePathKeyNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(ctrl->onAppAttributeChanged(
        AppSettings::GenericGroup, AppSettings::StoragePathKey, QVariant("/tmp")));
}

// onAppAttributeChanged: 正确 group + 未知 key → 仅 updatePublish
TEST_F(DiscoverControllerStateTest, OnAppAttributeChangedGenericGroupUnknownKey)
{
    EXPECT_NO_FATAL_FAILURE(ctrl->onAppAttributeChanged(
        AppSettings::GenericGroup, "unknown_key", QVariant()));
}

// compatRemoveDeivce: 转发到 deviceLosted
TEST_F(DiscoverControllerStateTest, CompatRemoveDeivceRemovesDevice)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.50", "dev50"));
    info->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info);
    ctrl->compatRemoveDeivce("10.0.0.50");
    EXPECT_EQ(ctrl->findDeviceByIP("10.0.0.50"), nullptr);
}

// finishDiscoveryWithError: 发出 discoveryFinished(false)
TEST_F(DiscoverControllerStateTest, FinishDiscoveryWithErrorEmitsDiscoveryFinished)
{
    QSignalSpy spy(ctrl, &DiscoverController::discoveryFinished);
    ctrl->finishDiscoveryWithError("test error");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

// finishDiscovery: 发出 deviceOnline + discoveryFinished
TEST_F(DiscoverControllerStateTest, FinishDiscoveryEmitsSignals)
{
    QSignalSpy spyFinished(ctrl, &DiscoverController::discoveryFinished);
    QSignalSpy spyOnline(ctrl, &DiscoverController::deviceOnline);
    ctrl->finishDiscovery();
    EXPECT_EQ(spyFinished.count(), 1);
    EXPECT_EQ(spyOnline.count(), 1);
}

// addSearchDeivce: 合法 JSON 且 isValid → 发出 deviceOnline
TEST_F(DiscoverControllerStateTest, AddSearchDeivceValidEmitsDeviceOnline)
{
    QSignalSpy spy(ctrl, &DiscoverController::deviceOnline);
    QString json = QStringLiteral("{\"IPAddress\":\"10.0.0.60\",\"DeviceName\":\"searchdev\"}");
    ctrl->addSearchDeivce(json);
    EXPECT_GE(spy.count(), 1);
    EXPECT_NE(ctrl->d->searchDevice, nullptr);
}

// addSearchDeivce: 非法 JSON → 发出 discoveryFinished(false)
TEST_F(DiscoverControllerStateTest, AddSearchDeivceInvalidEmitsDiscoveryFinished)
{
    QSignalSpy spy(ctrl, &DiscoverController::discoveryFinished);
    ctrl->addSearchDeivce("not json");
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

// compatAddDeivces: 空 map → 无 deviceOnline 信号
TEST_F(DiscoverControllerStateTest, CompatAddDeivcesEmptyMapNoSignal)
{
    QSignalSpy spy(ctrl, &DiscoverController::deviceOnline);
    EXPECT_NO_FATAL_FAILURE(ctrl->compatAddDeivces(StringMap{}));
    EXPECT_EQ(spy.count(), 0);
}
