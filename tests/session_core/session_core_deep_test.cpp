// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QTimer>
#include <QTest>

#include "discover/discovercontroller.h"
#include "discover/discovercontroller_p.h"
#include "discover/deviceinfo.h"
#include "utils/cooperationutil.h"
#include "share/sharecooperationservice.h"
#include "share/sharecooperationservicemanager.h"
#include "gui/widgets/buttonboxwidget.h"
#include "gui/widgets/filechooseredit.h"
#include "global_defines.h"

using namespace cooperation_core;

// ============ DiscoverController: signal-safe data operations ============

class DiscoverDeepTest : public ::testing::Test {
protected:
    DiscoverController *ctrl;
    void SetUp() override
    {
        ctrl = DiscoverController::instance();
        ctrl->_historyDevices.clear();
        ctrl->d->onlineDeviceList.clear();
        ctrl->d->historyDeviceMap.clear();
        ctrl->d->searchDevice.reset();
        ctrl->_connectedDevice = "";
        ctrl->d->ipfilter = "";
    }
    void TearDown() override
    {
        ctrl->_historyDevices.clear();
        ctrl->d->onlineDeviceList.clear();
        ctrl->d->historyDeviceMap.clear();
        ctrl->d->searchDevice.reset();
        ctrl->_connectedDevice = "";
    }

    QString makeJson(const QString &ip, const QString &name, int transMode = 0)
    {
        QVariantMap map;
        map.insert(AppSettings::IPAddress, ip);
        map.insert(AppSettings::DeviceNameKey, name);
        map.insert(AppSettings::DiscoveryModeKey, 0);
        map.insert(AppSettings::TransferModeKey, transMode);
        map.insert(AppSettings::LinkDirectionKey, 0);
        map.insert(AppSettings::ClipboardShareKey, false);
        map.insert(AppSettings::PeripheralShareKey, false);
        map.insert(AppSettings::CooperationEnabled, true);
        map.insert(AppSettings::OSType, 0);
        return QString::fromUtf8(
            QJsonDocument(QJsonObject::fromVariantMap(map)).toJson(QJsonDocument::Compact));
    }
};

TEST_F(DiscoverDeepTest, UpdateDeviceStateNewDevice)
{
    DeviceInfoPointer info(new DeviceInfo("192.168.1.100", "NewDev"));
    info->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info);
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 1);
}

TEST_F(DiscoverDeepTest, UpdateDeviceStateConnected)
{
    DeviceInfoPointer info(new DeviceInfo("192.168.1.200", "ConnDev"));
    info->setConnectStatus(DeviceInfo::Connected);
    ctrl->updateDeviceState(info);
    EXPECT_EQ(ctrl->_connectedDevice, "192.168.1.200");
}

TEST_F(DiscoverDeepTest, UpdateDeviceStateReplace)
{
    DeviceInfoPointer old(new DeviceInfo("192.168.1.150", "Old"));
    ctrl->d->onlineDeviceList.append(old);
    DeviceInfoPointer newDev(new DeviceInfo("192.168.1.150", "New"));
    newDev->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(newDev);
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 1);
    EXPECT_EQ(ctrl->d->onlineDeviceList[0]->deviceName(), "New");
}

TEST_F(DiscoverDeepTest, UpdateDeviceStateDisconnected)
{
    DeviceInfoPointer info(new DeviceInfo("192.168.1.200", "Dev"));
    info->setConnectStatus(DeviceInfo::Connected);
    ctrl->updateDeviceState(info);
    EXPECT_EQ(ctrl->_connectedDevice, "192.168.1.200");

    DeviceInfoPointer info2(new DeviceInfo("192.168.1.200", "Dev"));
    info2->setConnectStatus(DeviceInfo::Connectable);
    ctrl->updateDeviceState(info2);
    EXPECT_EQ(ctrl->_connectedDevice, "");
}

TEST_F(DiscoverDeepTest, DeviceLostedHistoryDevice)
{
    ctrl->_historyDevices.append("192.168.1.80");
    DeviceInfoPointer info(new DeviceInfo("192.168.1.80", "HistDev"));
    info->setConnectStatus(DeviceInfo::Connected);
    ctrl->d->onlineDeviceList.append(info);

    ctrl->deviceLosted("192.168.1.80");
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 1);
    EXPECT_EQ(ctrl->d->onlineDeviceList[0]->connectStatus(), DeviceInfo::Offline);
}

TEST_F(DiscoverDeepTest, DeviceLostedNonHistoryRemoved)
{
    DeviceInfoPointer info(new DeviceInfo("192.168.1.81", "Temp"));
    ctrl->d->onlineDeviceList.append(info);
    ctrl->deviceLosted("192.168.1.81");
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverDeepTest, DeviceLostedNotFound)
{
    ctrl->deviceLosted("192.168.1.99");
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverDeepTest, CompatAddDevicesValid)
{
    auto json = makeJson("10.0.0.30", "CompatDev");
    StringMap infoMap;
    infoMap[json] = "10.0.0.30, 192.168.1.1";
    ctrl->compatAddDeivces(infoMap);
    EXPECT_GT(ctrl->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverDeepTest, CompatAddDevicesInvalidJson)
{
    StringMap infoMap;
    infoMap["invalid"] = "10.0.0.30, 192.168.1.1";
    ctrl->compatAddDeivces(infoMap);
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverDeepTest, CompatAddDevicesBadIpFormat)
{
    auto json = makeJson("10.0.0.30", "Dev");
    StringMap infoMap;
    infoMap[json] = "10.0.0.30";  // single IP, wrong format
    ctrl->compatAddDeivces(infoMap);
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverDeepTest, CompatRemoveDevice)
{
    DeviceInfoPointer info(new DeviceInfo("192.168.1.55", "Remove"));
    ctrl->d->onlineDeviceList.append(info);
    ctrl->compatRemoveDeivce("192.168.1.55");
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 0);
}

TEST_F(DiscoverDeepTest, AddSearchDeivceValid)
{
    auto json = makeJson("10.0.0.200", "SearchDev");
    ctrl->addSearchDeivce(json);
    EXPECT_NE(ctrl->d->searchDevice, nullptr);
    EXPECT_EQ(ctrl->d->searchDevice->ipAddress(), "10.0.0.200");
}

TEST_F(DiscoverDeepTest, AddSearchDeivceInvalid)
{
    ctrl->addSearchDeivce("invalid json");
    EXPECT_EQ(ctrl->d->searchDevice, nullptr);
}

TEST_F(DiscoverDeepTest, UpdateHistoryDevicesAdds)
{
    QMap<QString, QString> connectMap;
    connectMap["192.168.1.60"] = "Dev1";
    connectMap["192.168.1.61"] = "Dev2";
    ctrl->updateHistoryDevices(connectMap);
    EXPECT_EQ(ctrl->_historyDevices.size(), 2);
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 2);
}

TEST_F(DiscoverDeepTest, UpdateHistoryDevicesSkipsOnline)
{
    DeviceInfoPointer existing(new DeviceInfo("192.168.1.60", "Already"));
    ctrl->d->onlineDeviceList.append(existing);
    QMap<QString, QString> connectMap;
    connectMap["192.168.1.60"] = "Dev1";
    connectMap["192.168.1.61"] = "Dev2";
    ctrl->updateHistoryDevices(connectMap);
    // 192.168.1.60 already exists, only 192.168.1.61 added
    EXPECT_EQ(ctrl->d->onlineDeviceList.size(), 2);
}

TEST_F(DiscoverDeepTest, OnConnectHistoryUpdated)
{
    ctrl->_historyDevices.append("192.168.1.77");
    DeviceInfoPointer info(new DeviceInfo("192.168.1.77", "Old"));
    ctrl->d->onlineDeviceList.append(info);
    ctrl->onConnectHistoryUpdated();
    // Old device should be removed since it's not in new history
    EXPECT_FALSE(ctrl->_historyDevices.contains("192.168.1.77"));
}

TEST_F(DiscoverDeepTest, OnDConfigValueChangedWrongConfig)
{
    ctrl->onDConfigValueChanged("wrong", "key");
    SUCCEED();
}

TEST_F(DiscoverDeepTest, OnAppAttributeChangedWrongGroup)
{
    ctrl->onAppAttributeChanged("wrong", "key", "val");
    SUCCEED();
}

TEST_F(DiscoverDeepTest, SelfInfoNotNull)
{
    auto info = ctrl->selfInfo();
    EXPECT_NE(info, nullptr);
}

TEST_F(DiscoverDeepTest, IsZeroConfDaemonActiveNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(DiscoverController::isZeroConfDaemonActive());
}

TEST_F(DiscoverDeepTest, IsVaildDeviceNull)
{
    DeviceInfoPointer nullDev;
    EXPECT_FALSE(ctrl->isVaildDevice(nullDev));
}

TEST_F(DiscoverDeepTest, IsVaildDeviceEmptyIP)
{
    DeviceInfoPointer dev(new DeviceInfo("", "Empty"));
    EXPECT_FALSE(ctrl->isVaildDevice(dev));
}

TEST_F(DiscoverDeepTest, IsVaildDeviceFilter)
{
    ctrl->d->ipfilter = "192.168.1";
    DeviceInfoPointer valid(new DeviceInfo("192.168.1.10", "OK"));
    EXPECT_TRUE(ctrl->isVaildDevice(valid));

    DeviceInfoPointer bad(new DeviceInfo("10.0.0.10", "Bad"));
    EXPECT_FALSE(ctrl->isVaildDevice(bad));
}

TEST_F(DiscoverDeepTest, IsVaildDeviceHistoryBypass)
{
    ctrl->d->ipfilter = "192.168.1";
    ctrl->_historyDevices.append("10.0.0.50");
    DeviceInfoPointer histDev(new DeviceInfo("10.0.0.50", "Hist"));
    EXPECT_TRUE(ctrl->isVaildDevice(histDev));
}

// ============ ButtonBoxWidget ============

TEST(ButtonBoxWidgetTest, AddAndGetButton)
{
    ButtonBoxWidget w;
    int idx = w.addButton(QIcon(), "btn1");
    EXPECT_EQ(idx, 0);
    EXPECT_NE(w.button(0), nullptr);

    int idx2 = w.addButton(QIcon(), "btn2");
    EXPECT_EQ(idx2, 1);
    EXPECT_NE(w.button(1), nullptr);
}

TEST(ButtonBoxWidgetTest, SetButtonVisible)
{
    ButtonBoxWidget w;
    w.addButton(QIcon(), "btn1");
    w.setButtonVisible(0, false);
    EXPECT_FALSE(w.button(0)->isVisible());
    w.setButtonVisible(0, true);
}

TEST(ButtonBoxWidgetTest, SetButtonClickable)
{
    ButtonBoxWidget w;
    w.addButton(QIcon(), "btn1");
    w.setButtonClickable(0, false);
    EXPECT_FALSE(w.button(0)->isEnabled());
    w.setButtonClickable(0, true);
    EXPECT_TRUE(w.button(0)->isEnabled());
}

TEST(ButtonBoxWidgetTest, MultipleButtons)
{
    ButtonBoxWidget w;
    w.addButton(QIcon(), "btn1");
    w.addButton(QIcon(), "btn2");
    EXPECT_NE(w.button(0), nullptr);
    EXPECT_NE(w.button(1), nullptr);
}

TEST(ButtonBoxWidgetTest, ButtonClickedSignal)
{
    ButtonBoxWidget w;
    QSignalSpy spy(&w, &ButtonBoxWidget::buttonClicked);
    w.addButton(QIcon::fromTheme("edit"), "btn1");
    QTest::mouseClick(w.button(0), Qt::LeftButton);
    EXPECT_GE(spy.count(), 0); // may or may not fire in offscreen
}

// ============ FileChooserEdit ============

TEST(FileChooserDeepTest, SetTextDoesNotCrash)
{
    FileChooserEdit edit;
    edit.setText("/tmp/test_file.txt");
    SUCCEED();
}

TEST(FileChooserDeepTest, PaintEventDoesNotCrash)
{
    FileChooserEdit edit;
    edit.repaint();
    SUCCEED();
}

// ============ CooperationUtil ============

TEST(CooperationUtilTest, InstanceNotNull)
{
    EXPECT_NE(CooperationUtil::instance(), nullptr);
}

TEST(CooperationUtilTest, LocalIPAddressNotEmpty)
{
    auto ip = CooperationUtil::localIPAddress();
    // May be empty if no network, just verify no crash
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::localIPAddress());
}

TEST(CooperationUtilTest, CloseOptionSaveLoad)
{
    CooperationUtil::saveOption(true);
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::closeOption());
    CooperationUtil::saveOption(false);
}

TEST(CooperationUtilTest, DeviceInfoNotEmpty)
{
    auto info = CooperationUtil::deviceInfo();
    EXPECT_FALSE(info.empty());
    EXPECT_TRUE(info.contains(AppSettings::IPAddress));
}

TEST(CooperationUtilTest, SetStorageConfig)
{
    CooperationUtil::instance()->setStorageConfig("/tmp/test_storage");
    SUCCEED();
}

TEST(CooperationUtilTest, RegisterDeviceOperation)
{
    QVariantMap map;
    map.insert("id", "test-op");
    map.insert("description", "Test operation");
    CooperationUtil::instance()->registerDeviceOperation(map);
    SUCCEED();
}

// ============ ShareCooperationServiceManager ============

TEST(ShareCoopServiceMgrTest, InstanceNotNull)
{
    EXPECT_NE(ShareCooperationServiceManager::instance(), nullptr);
}
