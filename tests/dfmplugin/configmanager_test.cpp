// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ConfigManager (dfmplugin vendored copy) 单元测试。
// 依赖 qApp 的 organizationName/applicationName (在 main.cpp 中已设置)。

#include <gtest/gtest.h>

#include "configs/settings/configmanager.h"
#include "configs/settings/settings.h"

#include <QVariant>

TEST(DfmConfigManagerTest, InstanceIsSingleton)
{
    EXPECT_EQ(ConfigManager::instance(), ConfigManager::instance());
}

TEST(DfmConfigManagerTest, AppSettingReturnsValidSettings)
{
    Settings *s = ConfigManager::instance()->appSetting();
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s, nullptr);
}

TEST(DfmConfigManagerTest, SetAndReadAppAttributeRoundTrip)
{
    ConfigManager::instance()->setAppAttribute("UnitTestGroup", "someKey", 12345);
    QVariant v = ConfigManager::instance()->appAttribute("UnitTestGroup", "someKey");
    EXPECT_EQ(v.toInt(), 12345);
}

TEST(DfmConfigManagerTest, AppAttributeUnknownReturnsInvalid)
{
    QVariant v = ConfigManager::instance()->appAttribute("NoSuchGroup", "NoSuchKey");
    EXPECT_FALSE(v.isValid());
}

TEST(DfmConfigManagerTest, SyncAppAttributeReturnsBool)
{
    EXPECT_NO_FATAL_FAILURE(ConfigManager::instance()->syncAppAttribute());
}
