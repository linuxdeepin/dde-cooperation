// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTest>
#include <QEvent>
#include <QShowEvent>
#include <QCoreApplication>
#include "gui/widgets/deviceitem.h"
#include "discover/deviceinfo.h"
#include "addr_pri.h"

using cooperation_core::DeviceItem;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

// 访问私有 indexOperaMap,用于确定性触发 onButtonClicked 的 clickedCb 分支。
using IndexOperaMap = QMap<int, DeviceItem::Operation>;
ACCESS_PRIVATE_FIELD(DeviceItem, IndexOperaMap, indexOperaMap)

// 构造一个最小 Operation(三类回调齐备)。
static DeviceItem::Operation makeOp(const QString &id, bool *clickedFlag)
{
    DeviceItem::Operation op;
    op.id = id;
    op.description = id;
    op.icon = "";
    op.location = 0;
    op.style = 0;
    op.visibleCb = [](const QString &, const DeviceInfoPointer) { return true; };
    op.clickableCb = [](const QString &, const DeviceInfoPointer) { return true; };
    op.clickedCb = [clickedFlag](const QString &, const DeviceInfoPointer) {
        if (clickedFlag) *clickedFlag = true;
    };
    return op;
}

// ============ StateLabel ============

TEST(StateLabelTest, SetStateAndGetReturnsSame)
{
    cooperation_core::StateLabel label;
    label.setState(DeviceInfo::Connected);
    EXPECT_EQ(label.state(), DeviceInfo::Connected);
    label.setState(DeviceInfo::Offline);
    EXPECT_EQ(label.state(), DeviceInfo::Offline);
}

// paintEvent 三分支(Connected/Connectable/Offline)。offscreen 下需 show 进入可见态
// 才能让 repaint 真正触发 paintEvent。
TEST(StateLabelTest, PaintEventAllStatusesDoNotCrash)
{
    cooperation_core::StateLabel label;
    label.resize(80, 24);
    label.setText("connected");
    label.show();
    QTest::qWait(20);
    for (auto s : {DeviceInfo::Connected, DeviceInfo::Connectable, DeviceInfo::Offline}) {
        label.setState(s);
        EXPECT_NO_FATAL_FAILURE(label.repaint());
    }
}

// ============ DeviceItem ============

TEST(DeviceItemTest, IsAliveInitiallyTrue)
{
    DeviceItem item;
    EXPECT_TRUE(item.isAlive());
}

TEST(DeviceItemTest, SetDeviceInfoAndGetReturnsSame)
{
    DeviceItem item;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.1", "DevA"));
    item.setDeviceInfo(info);
    auto got = item.deviceInfo();
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->ipAddress().toStdString(), "10.0.0.1");
    EXPECT_EQ(got->deviceName().toStdString(), "DevA");
}

// setDeviceStatus 三分支(Connected/Connectable/Offline)经 setDeviceInfo 触发,
// 覆盖 setDeviceStatus + QIcon::fromTheme + stateLabel setText。
TEST(DeviceItemTest, SetDeviceInfoWithAllConnectStatusesDoesNotCrash)
{
    for (auto status : {DeviceInfo::Connected, DeviceInfo::Connectable, DeviceInfo::Offline}) {
        DeviceItem item;
        DeviceInfoPointer info(new DeviceInfo("10.0.0.2", "DevS"));
        info->setConnectStatus(status);
        EXPECT_NO_FATAL_FAILURE(item.setDeviceInfo(info));
    }
}

TEST(DeviceItemTest, SetOperationsAndUpdateOperationsDoNotCrash)
{
    DeviceItem item;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.3", "DevO"));
    item.setDeviceInfo(info);
    bool clicked = false;
    item.setOperations({makeOp("op1", &clicked)});
    EXPECT_NO_FATAL_FAILURE(item.updateOperations());
}

// onButtonClicked 无效 index → 早返回不崩。
TEST(DeviceItemTest, OnButtonClickedInvalidIndexDoesNotCrash)
{
    DeviceItem item;
    EXPECT_NO_FATAL_FAILURE(item.onButtonClicked(-1));
    EXPECT_NO_FATAL_FAILURE(item.onButtonClicked(999));
}

// onButtonClicked 有效 index → clickedCb 被调用(经 ACCESS_PRIVATE_FIELD 取真实 key)。
TEST(DeviceItemTest, OnButtonClickedValidIndexInvokesClickedCb)
{
    DeviceItem item;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.4", "DevC"));
    item.setDeviceInfo(info);
    bool clicked = false;
    item.setOperations({makeOp("op-click", &clicked)});

    auto &map = access_private_field::DeviceItemindexOperaMap(item);
    QList<int> keys = map.keys();
    ASSERT_FALSE(keys.isEmpty());
    EXPECT_NO_FATAL_FAILURE(item.onButtonClicked(keys.first()));
    EXPECT_TRUE(clicked);
}

// 长设备名经 QFontMetrics::elidedText 截断,触发 setToolTip 分支。
TEST(DeviceItemTest, SetDeviceInfoWithLongNameTruncates)
{
    DeviceItem item;
    QString longName(200, QChar('X'));
    DeviceInfoPointer info(new DeviceInfo("10.0.0.5", longName));
    EXPECT_NO_FATAL_FAILURE(item.setDeviceInfo(info));
}

// 多个 operation 触发 setOperations 的 sort 比较器。
TEST(DeviceItemTest, SetOperationsMultipleTriggersSort)
{
    DeviceItem item;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.6", "DevM"));
    item.setDeviceInfo(info);
    bool c1 = false, c2 = false;
    DeviceItem::Operation opA = makeOp("opA", &c1);
    opA.location = 1;
    DeviceItem::Operation opB = makeOp("opB", &c2);
    opB.location = 0;
    EXPECT_NO_FATAL_FAILURE(item.setOperations({opA, opB}));
}

// operation 无回调时 updateOperations 走 continue 分支(visibleCb/clickableCb 为空)。
TEST(DeviceItemTest, UpdateOperationsWithNullCallbacksDoesNotCrash)
{
    DeviceItem item;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.7", "DevN"));
    item.setDeviceInfo(info);
    DeviceItem::Operation op;  // 三回调默认构造为空
    op.id = "null-op";
    op.location = 0;
    op.style = 0;
    item.setOperations({op});
    EXPECT_NO_FATAL_FAILURE(item.updateOperations());
}

// leaveEvent(QEvent)/showEvent(QShowEvent) 经 sendEvent 触发覆盖。
TEST(DeviceItemTest, LeaveAndShowEventsDoNotCrash)
{
    DeviceItem item;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.8", "DevE"));
    item.setDeviceInfo(info);
    QEvent leave(QEvent::Leave);
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&item, &leave));
    QShowEvent show;
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&item, &show));
}
