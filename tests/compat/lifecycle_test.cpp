// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// framework/lifecycle 测试：PluginManager 配置/扫描 API + PluginMetaObject getter。

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QStringList>

#include <dde-cooperation-framework/lifecycle/pluginmanager.h>
#include <dde-cooperation-framework/lifecycle/pluginmetaobject.h>

DPF_USE_NAMESPACE

TEST(PluginManagerTest, CtorAndDefaults)
{
    PluginManager pm;
    EXPECT_TRUE(pm.pluginIIDs().isEmpty());
    EXPECT_TRUE(pm.pluginPaths().isEmpty());
    EXPECT_TRUE(pm.blackList().isEmpty());
    EXPECT_TRUE(pm.lazyLoadList().isEmpty());
}

TEST(PluginManagerTest, AddListsAndPaths)
{
    PluginManager pm;
    pm.addPluginIID("org.deepin.Plugin.1.0");
    pm.addBlackPluginName("disabled-plugin");
    pm.addLazyLoadPluginName("lazy-plugin");
    pm.setPluginPaths({"/non/existent/path"});

    EXPECT_TRUE(pm.pluginIIDs().contains("org.deepin.Plugin.1.0"));
    EXPECT_TRUE(pm.blackList().contains("disabled-plugin"));
    EXPECT_TRUE(pm.lazyLoadList().contains("lazy-plugin"));
    EXPECT_EQ(pm.pluginPaths().size(), 1);
}

TEST(PluginManagerTest, ReadPluginsEmptyPathFailsGracefully)
{
    PluginManager pm;
    pm.setPluginPaths({"/no/such/dir/xyz"});
    // 无可加载插件，返回 false 或 true（实现决定），不应崩溃
    bool r = pm.readPlugins();
    (void)r;
    SUCCEED();
}

TEST(PluginManagerTest, LoadInitStartStopOnEmptyIsSafe)
{
    PluginManager pm;
    EXPECT_NO_THROW((void)pm.loadPlugins());
    EXPECT_NO_THROW((void)pm.initPlugins());
    EXPECT_NO_THROW((void)pm.startPlugins());
    EXPECT_NO_THROW((void)pm.stopPlugins());
    // 无插件时状态应可查询
    EXPECT_NO_THROW((void)pm.isAllPluginsInitialized());
    EXPECT_NO_THROW((void)pm.isAllPluginsStarted());
}

TEST(PluginManagerTest, PluginMetaObjLookupMissReturnsNull)
{
    PluginManager pm;
    auto meta = pm.pluginMetaObj("nonexistent-plugin");
    EXPECT_EQ(meta, nullptr);
}

TEST(PluginMetaObjectTest, DefaultCtorAccessors)
{
    PluginMetaObject obj;
    EXPECT_TRUE(obj.name().isEmpty());
    EXPECT_TRUE(obj.iid().isEmpty());
    EXPECT_TRUE(obj.version().isEmpty());
    EXPECT_TRUE(obj.fileName().isEmpty());
    EXPECT_FALSE(obj.isVirtual());
}

TEST(PluginMetaObjectTest, ErrorStringIsEmptyByDefault)
{
    PluginMetaObject obj;
    EXPECT_TRUE(obj.errorString().isEmpty());
}
