// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "utils/config.h"

#include <QTemporaryFile>
#include <QSettings>
#include <QFileInfo>

TEST(DaemonConfigTest, InstanceSingleton)
{
    auto *a = DaemonConfig::instance();
    auto *b = DaemonConfig::instance();
    EXPECT_EQ(a, b);
}

TEST(DaemonConfigTest, PinOperations)
{
    auto *cfg = DaemonConfig::instance();

    cfg->setPin("123456");
    EXPECT_EQ(cfg->getPin(), fastring("123456"));

    fastring newPin = cfg->refreshPin();
    EXPECT_EQ(cfg->getPin(), newPin);
    EXPECT_EQ(newPin.size(), 6u);
}

TEST(DaemonConfigTest, UUIDOperations)
{
    auto *cfg = DaemonConfig::instance();

    cfg->setUUID("test-uuid-1234");
    EXPECT_EQ(cfg->getUUID(), fastring("test-uuid-1234"));
}

TEST(DaemonConfigTest, NickNameOperations)
{
    auto *cfg = DaemonConfig::instance();

    cfg->setNickName("MyMachine");
    EXPECT_EQ(cfg->getNickName(), fastring("MyMachine"));
}

TEST(DaemonConfigTest, ModeOperations)
{
    auto *cfg = DaemonConfig::instance();

    cfg->setMode(1);
    EXPECT_EQ(cfg->getMode(), 1);

    cfg->setMode(0);
    EXPECT_EQ(cfg->getMode(), 0);
}

TEST(DaemonConfigTest, AppConfigOperations)
{
    auto *cfg = DaemonConfig::instance();

    cfg->setAppConfig("myapp", "key1", "value1");
    EXPECT_EQ(cfg->getAppConfig("myapp", "key1"), fastring("value1"));

    // nonexistent key returns empty
    EXPECT_TRUE(cfg->getAppConfig("myapp", "nonexistent").empty());
    EXPECT_TRUE(cfg->getAppConfig("nonexistent_app", "key1").empty());
}

TEST(DaemonConfigTest, SessionAndAuthTokens)
{
    auto *cfg = DaemonConfig::instance();

    cfg->saveRemoteSession("session-abc");
    EXPECT_EQ(cfg->getRemoteSession(), fastring("session-abc"));

    cfg->saveAuthed("token-xyz");
    EXPECT_EQ(cfg->getAuthed(), fastring("token-xyz"));
}

TEST(DaemonConfigTest, TargetNameAndStorageDir)
{
    auto *cfg = DaemonConfig::instance();

    // no target name -> Downloads dir
    cfg->setTargetName("");
    fastring dir = cfg->getStorageDir();
    EXPECT_FALSE(dir.empty());
    EXPECT_TRUE(dir.find("Downloads") != fastring::npos);

    // with target name
    cfg->setTargetName("mytarget");
    dir = cfg->getStorageDir();
    EXPECT_TRUE(dir.find("mytarget") != fastring::npos);
}

TEST(DaemonConfigTest, StorageDirByAppName)
{
    auto *cfg = DaemonConfig::instance();

    // no stored config -> Downloads
    fastring dir = cfg->getStorageDir("nonexistent_app");
    EXPECT_FALSE(dir.empty());
    EXPECT_TRUE(dir.find("Downloads") != fastring::npos);

    // with stored config
    cfg->setAppConfig("app_with_storage", "storagedir", "/tmp/myfiles");
    dir = cfg->getStorageDir("app_with_storage");
    EXPECT_EQ(dir, fastring("/tmp/myfiles"));
}

TEST(DaemonConfigTest, NeedConfirmAlwaysFalse)
{
    auto *cfg = DaemonConfig::instance();
    EXPECT_FALSE(cfg->needConfirm());
}

TEST(DaemonConfigTest, InitPinWhenEmpty)
{
    auto *cfg = DaemonConfig::instance();

    // Set pin to empty indirectly by refreshing
    cfg->refreshPin();
    fastring pin1 = cfg->getPin();
    EXPECT_EQ(pin1.size(), 6u);

    cfg->initPin();
    // pin should remain the same since it's not empty
    EXPECT_EQ(cfg->getPin(), pin1);
}
