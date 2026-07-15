// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "configs/settings/configmanager.h"
#include "configs/settings/settings.h"

#include <QVariant>

TEST(CoopConfigManagerTest, InstanceIsSingleton)
{
    EXPECT_EQ(ConfigManager::instance(), ConfigManager::instance());
}

TEST(CoopConfigManagerTest, AppSettingReturnsValidSettings)
{
    Settings *s = ConfigManager::instance()->appSetting();
    ASSERT_NE(s, nullptr);
    EXPECT_NE(s, nullptr);
}

TEST(CoopConfigManagerTest, SetAndReadAppAttributeRoundTrip)
{
    ConfigManager::instance()->setAppAttribute("CoopUnitTestGroup", "someKey", 12345);
    QVariant v = ConfigManager::instance()->appAttribute("CoopUnitTestGroup", "someKey");
    EXPECT_EQ(v.toInt(), 12345);
}

TEST(CoopConfigManagerTest, AppAttributeUnknownReturnsInvalid)
{
    QVariant v = ConfigManager::instance()->appAttribute("NoSuchCoopGroup", "NoSuchKey");
    EXPECT_FALSE(v.isValid());
}

TEST(CoopConfigManagerTest, AppAttributeStringRoundTrip)
{
    ConfigManager::instance()->setAppAttribute("CoopStrGroup", "name", QVariant("hello"));
    QVariant v = ConfigManager::instance()->appAttribute("CoopStrGroup", "name");
    EXPECT_EQ(v.toString().toStdString(), "hello");
}

TEST(CoopConfigManagerTest, SyncAppAttributeReturnsBool)
{
    EXPECT_NO_FATAL_FAILURE(ConfigManager::instance()->syncAppAttribute());
}
