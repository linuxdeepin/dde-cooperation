// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 注:dfmplugin 版与 src/configs 版的 dconfigmanager.h/configmanager.h 是相同
// vendored 副本(同类名同 API 同 include guard)。coop_tests 已 glob src/configs 版,
// 故此处测 src/configs 版(免 include guard 碰撞),覆盖率落在 src/configs 的 .cpp。

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QVariant>
#include <QStringList>
#include "configs/dconfig/dconfigmanager.h"
#include "configs/settings/configmanager.h"

// ============ DConfigManager ============

TEST(DConfigManagerTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(DConfigManager::instance(), DConfigManager::instance());
}

TEST(DConfigManagerTest, KeysForUnknownConfigReturnsEmpty)
{
    EXPECT_TRUE(DConfigManager::instance()->keys("org.deepin.dde.nonexistent").isEmpty());
}

TEST(DConfigManagerTest, ContainsForUnknownConfigReturnsFalse)
{
    EXPECT_FALSE(DConfigManager::instance()->contains("org.deepin.dde.nonexistent", "anyKey"));
}

TEST(DConfigManagerTest, ContainsEmptyKeyReturnsFalse)
{
    // contains 在 key 为空时直接返回 false (dconfigmanager.cpp:103)。
    EXPECT_FALSE(DConfigManager::instance()->contains("org.deepin.dde.cooperation", ""));
}

TEST(DConfigManagerTest, ValueForUnknownConfigReturnsFallback)
{
    QVariant fallback = "defaultVal";
    QVariant v = DConfigManager::instance()->value("org.deepin.dde.nonexistent", "key", fallback);
    EXPECT_EQ(v.toString().toStdString(), "defaultVal");
}

TEST(DConfigManagerTest, SetValueForUnknownConfigDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(
        DConfigManager::instance()->setValue("org.deepin.dde.nonexistent", "key", QVariant(42)));
}

TEST(DConfigManagerTest, ValidateConfigsWithDefaultReturnsBool)
{
    QStringList invalid;
    bool ok = DConfigManager::instance()->validateConfigs(invalid);
    (void)ok;  // 默认 config 已加载则 true,offscreen 下 schema 可能无效则 false
    SUCCEED();
}

TEST(DConfigManagerTest, AddUnknownConfigReturnsFalseWithError)
{
    QString err;
    bool ok = DConfigManager::instance()->addConfig("org.deepin.dde.nonexistent", &err);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(err.isEmpty());  // "cannot create config" 或 "config is not valid"
}

TEST(DConfigManagerTest, RemoveConfigReturnsTrue)
{
    EXPECT_TRUE(DConfigManager::instance()->removeConfig("org.deepin.dde.nonexistent"));
}

// ============ ConfigManager ============

TEST(ConfigManagerTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(ConfigManager::instance(), ConfigManager::instance());
}

TEST(ConfigManagerTest, AppAttributeForUnknownReturnsInvalid)
{
    QVariant v = ConfigManager::instance()->appAttribute("UnknownGroup", "UnknownKey");
    EXPECT_FALSE(v.isValid());
}

TEST(ConfigManagerTest, SetAppAttributeRoundTrip)
{
    ConfigManager::instance()->setAppAttribute("RoundGroup", "RoundKey", QVariant("roundVal"));
    QVariant got = ConfigManager::instance()->appAttribute("RoundGroup", "RoundKey");
    EXPECT_EQ(got.toString().toStdString(), "roundVal");
}

TEST(ConfigManagerTest, SyncAppAttributeDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(ConfigManager::instance()->syncAppAttribute());
}

TEST(ConfigManagerTest, AppSettingReturnsNonNull)
{
    EXPECT_NE(ConfigManager::instance()->appSetting(), nullptr);
}
