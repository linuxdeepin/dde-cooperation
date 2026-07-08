// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTest>
#include <QCoreApplication>
#include <QVariantMap>
#include "gui/widgets/workspacewidget.h"
#include "gui/widgets/workspacewidget_p.h"  // QScopedPointer<Private> 析构需完整类型
#include "discover/deviceinfo.h"

using cooperation_core::WorkspaceWidget;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

// WorkspaceWidget 构造时启动 SortFilterWorker 后台线程(处理 addDevice/filterDevice 等
// queued 信号)。每个用例的 widget 析构时 quit+wait 该线程,清理干净。

TEST(WorkspaceWidgetTest, ConstructAndInitialItemCountZero)
{
    WorkspaceWidget w;
    EXPECT_EQ(w.itemCount(), 0);
}

// switchWidget 各页面分支:kLookignForDeviceWidget(动画+hide tip)/kNoNetworkWidget/
// kNoResultWidget/kDeviceListWidget(show label+refresh)。
TEST(WorkspaceWidgetTest, SwitchWidgetAllPagesDoesNotCrash)
{
    WorkspaceWidget w;
    w.switchWidget(WorkspaceWidget::kLookignForDeviceWidget);
    w.switchWidget(WorkspaceWidget::kNoNetworkWidget);
    w.switchWidget(WorkspaceWidget::kNoResultWidget);
    w.switchWidget(WorkspaceWidget::kDeviceListWidget);
    SUCCEED();
}

// kUnknownPage → 早返回。
TEST(WorkspaceWidgetTest, SwitchWidgetUnknownPageEarlyReturns)
{
    WorkspaceWidget w;
    w.switchWidget(WorkspaceWidget::kUnknownPage);
    SUCCEED();
}

// 切到同一页面 → 早返回(currentPage == page)。
TEST(WorkspaceWidgetTest, SwitchWidgetSamePageEarlyReturns)
{
    WorkspaceWidget w;
    w.switchWidget(WorkspaceWidget::kNoNetworkWidget);
    w.switchWidget(WorkspaceWidget::kNoNetworkWidget);  // 同页 → 早返回
    SUCCEED();
}

TEST(WorkspaceWidgetTest, FindDeviceInfoUnknownIpReturnsNull)
{
    WorkspaceWidget w;
    EXPECT_EQ(w.findDeviceInfo("10.0.0.99"), nullptr);
}

TEST(WorkspaceWidgetTest, ClearWhenEmptyDoesNotCrash)
{
    WorkspaceWidget w;
    EXPECT_NO_FATAL_FAILURE(w.clear());
    EXPECT_EQ(w.itemCount(), 0);
}

TEST(WorkspaceWidgetTest, SetFirstStartTipDoesNotCrash)
{
    WorkspaceWidget w;
    w.setFirstStartTip(true);
    w.setFirstStartTip(false);
    SUCCEED();
}

TEST(WorkspaceWidgetTest, AddDeviceOperationEmptyMapDoesNotCrash)
{
    WorkspaceWidget w;
    EXPECT_NO_FATAL_FAILURE(w.addDeviceOperation(QVariantMap{}));
    SUCCEED();
}

// addDeviceInfos/removeDeviceInfos 经 queued 连接到 SortFilterWorker 线程,
// 这里只验不崩 + 排空事件循环,不强断言异步结果。
TEST(WorkspaceWidgetTest, AddAndRemoveDeviceInfosDoesNotCrash)
{
    WorkspaceWidget w;
    QList<DeviceInfoPointer> list{DeviceInfoPointer(new DeviceInfo("10.0.0.1", "DevA"))};
    EXPECT_NO_FATAL_FAILURE(w.addDeviceInfos(list));
    QCoreApplication::processEvents();
    QTest::qWait(50);
    EXPECT_NO_FATAL_FAILURE(w.removeDeviceInfos("10.0.0.1"));
    QCoreApplication::processEvents();
    QTest::qWait(50);
    SUCCEED();
}
