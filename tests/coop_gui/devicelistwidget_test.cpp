// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "gui/widgets/devicelistwidget.h"
#include "discover/deviceinfo.h"

using cooperation_core::DeviceListWidget;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

class DeviceListWidgetTest : public ::testing::Test
{
protected:
    DeviceListWidget *w = nullptr;
    void SetUp() override { w = new DeviceListWidget(); }
    void TearDown() override { delete w; }
    DeviceInfoPointer makeDev(const QString &ip, const QString &name)
    {
        return DeviceInfoPointer(new DeviceInfo(ip, name));
    }
};

TEST_F(DeviceListWidgetTest, EmptyWidgetHasZeroItemCount)
{
    EXPECT_EQ(w->itemCount(), 0);
}

TEST_F(DeviceListWidgetTest, AppendItemIncreasesCount)
{
    w->appendItem(makeDev("192.168.1.1", "Dev1"));
    EXPECT_EQ(w->itemCount(), 1);
}

TEST_F(DeviceListWidgetTest, FindDeviceInfoReturnsInsertedByIp)
{
    w->appendItem(makeDev("192.168.1.2", "Dev2"));
    auto found = w->findDeviceInfo("192.168.1.2");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->deviceName().toStdString(), "Dev2");
}

TEST_F(DeviceListWidgetTest, FindDeviceInfoReturnsNullForAbsentIp)
{
    EXPECT_EQ(w->findDeviceInfo("0.0.0.0"), nullptr);
}

TEST_F(DeviceListWidgetTest, IndexOfReturnsPosition)
{
    w->appendItem(makeDev("10.0.0.1", "A"));
    w->appendItem(makeDev("10.0.0.2", "B"));
    EXPECT_EQ(w->indexOf("10.0.0.2"), 1);
}

TEST_F(DeviceListWidgetTest, RemoveItemDecreasesCount)
{
    w->appendItem(makeDev("10.0.0.3", "C"));
    w->removeItem(0);
    EXPECT_EQ(w->itemCount(), 0);
}

TEST_F(DeviceListWidgetTest, ClearEmptiesAllItems)
{
    w->appendItem(makeDev("10.0.0.4", "D"));
    w->appendItem(makeDev("10.0.0.5", "E"));
    w->clear();
    EXPECT_EQ(w->itemCount(), 0);
}
