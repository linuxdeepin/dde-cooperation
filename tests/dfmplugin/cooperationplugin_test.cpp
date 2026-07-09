// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// cooperationplugin (dfmplugin vendored copy) 单元测试。
// -fno-access-control 允许直接调用 private addCooperationSettingItem()。
// initialize() 内部会 ReportLogManager::init() (本机无 event-log 库 -> 安全返回),
// 并走 DPF_NAMESPACE::LifeCycle::isAllPluginsStarted()=false 分支 connect(dpfListener).
// bindMenuScene/onMenuSceneAdded 调 dpf 单例的 channel/dispatcher (真实实例, 安全)。

#include <gtest/gtest.h>

#include "cooperationplugin.h"
#include "reportlog/reportlogmanager.h"

#include <QCoreApplication>
#include <QString>

using namespace dfmplugin_cooperation;

TEST(CooperationPluginTest, InitializeRunsCleanly)
{
    CooperationPlugin plugin;
    EXPECT_NO_FATAL_FAILURE(plugin.initialize());
}

TEST(CooperationPluginTest, StartReturnsTrueNonDfmApp)
{
    QString orig = qApp->applicationName();
    qApp->setApplicationName("not-dde-file-manager");
    CooperationPlugin plugin;
    bool ok = plugin.start();
    EXPECT_TRUE(ok);
    qApp->setApplicationName(orig);
}

TEST(CooperationPluginTest, StartReturnsTrueForDfmApp)
{
    QString orig = qApp->applicationName();
    qApp->setApplicationName("dde-file-manager");
    CooperationPlugin plugin;
    bool ok = plugin.start();
    EXPECT_TRUE(ok);
    qApp->setApplicationName(orig);
}

TEST(CooperationPluginTest, AddCooperationSettingItem)
{
    CooperationPlugin plugin;
    EXPECT_NO_FATAL_FAILURE(plugin.addCooperationSettingItem());
}

TEST(CooperationPluginTest, BindMenuSceneRuns)
{
    CooperationPlugin plugin;
    EXPECT_NO_FATAL_FAILURE(plugin.bindMenuScene());
}

TEST(CooperationPluginTest, OnMenuSceneAddedParentBinds)
{
    CooperationPlugin plugin;
    // 传入父场景名 -> 走绑定分支
    EXPECT_NO_FATAL_FAILURE(plugin.onMenuSceneAdded(QString("ExtendMenu")));
}

TEST(CooperationPluginTest, OnMenuSceneAddedOtherSceneNoop)
{
    CooperationPlugin plugin;
    EXPECT_NO_FATAL_FAILURE(plugin.onMenuSceneAdded(QString("SomeOtherScene")));
}
