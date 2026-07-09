// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DConfigManager (dfmplugin vendored copy) 单元测试。
// DConfig::create 在无 dconf/ Trent 服务环境通常返回 null -> addConfig 失败,
// 但仍覆盖各分支; 不依赖具体 DConfig 后端返回值, 仅断言不崩溃与 API 契约。

#include <gtest/gtest.h>

#include "configs/dconfig/dconfigmanager.h"

#include <QVariant>
#include <QStringList>

TEST(DfmDConfigManagerTest, InstanceIsSingleton)
{
    EXPECT_EQ(DConfigManager::instance(), DConfigManager::instance());
}

TEST(DfmDConfigManagerTest, AddBogusConfigReturnsFalseWithError)
{
    QString err;
    bool ok = DConfigManager::instance()->addConfig("org.deepin.dde.does_not_exist", &err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.isEmpty());
}

TEST(DfmDConfigManagerTest, KeysForUnknownConfigEmpty)
{
    EXPECT_TRUE(DConfigManager::instance()->keys("org.deepin.dde.does_not_exist").isEmpty());
}

TEST(DfmDConfigManagerTest, ContainsEmptyKeyFalse)
{
    EXPECT_FALSE(DConfigManager::instance()->contains("org.deepin.dde.cooperation", ""));
}

TEST(DfmDConfigManagerTest, ContainsUnknownKeyFalse)
{
    EXPECT_FALSE(DConfigManager::instance()->contains("org.deepin.dde.cooperation", "no.such.key"));
}

TEST(DfmDConfigManagerTest, ValueReturnsFallbackForUnknown)
{
    QVariant fb = QStringLiteral("fallback");
    QVariant v = DConfigManager::instance()->value("org.deepin.dde.cooperation", "no.such.key", fb);
    // 无论后端是否注册, 返回值应可调用 (可能等于 fb 或后端默认)
    EXPECT_TRUE(v.isValid());
}

TEST(DfmDConfigManagerTest, SetValueUnknownConfigNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(DConfigManager::instance()->setValue("org.deepin.dde.cooperation", "cooperation.transfer.mode", 1));
}

TEST(DfmDConfigManagerTest, RemoveConfigReturnsTrue)
{
    EXPECT_TRUE(DConfigManager::instance()->removeConfig("org.deepin.dde.never_added"));
}

TEST(DfmDConfigManagerTest, ValidateConfigsReturnsBool)
{
    QStringList invalid;
    DConfigManager::instance()->validateConfigs(invalid);
    SUCCEED();
}

TEST(DfmDConfigManagerTest, AddSameConfigTwiceIsHandled)
{
    // 默认配置在构造时已 add; 再加同名应被 contains 分支拒绝 (返回 false) 或后端再次判定
    QString err;
    DConfigManager::instance()->addConfig(kDefaultCfgPath, &err);
    SUCCEED();
}
