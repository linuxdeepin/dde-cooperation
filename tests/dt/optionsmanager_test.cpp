// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// OptionsManager: 公开构造, 直接 new 实例 + instance() 双路覆盖。
// get/set/addUserOption/clear 全分支。

#include "utils/optionsmanager.h"

#include <gtest/gtest.h>
#include <QMap>
#include <QStringList>

TEST(OptionsManagerTest, InstanceSingleton)
{
    OptionsManager *a = OptionsManager::instance();
    OptionsManager *b = OptionsManager::instance();
    EXPECT_EQ(a, b);
}

TEST(OptionsManagerTest, SetGetUserOptions)
{
    OptionsManager mgr;
    QMap<QString, QStringList> opts;
    opts["app"] = QStringList{"a", "b"};
    opts["file"] = QStringList{"f1"};
    mgr.setUserOptions(opts);
    EXPECT_EQ(mgr.getUserOptions().size(), 2);
}

TEST(OptionsManagerTest, GetUserOptionHitAndMiss)
{
    OptionsManager mgr;
    mgr.addUserOption(Options::kApp, QStringList{"x"});
    EXPECT_EQ(mgr.getUserOption(Options::kApp).size(), 1);
    // 命中
    mgr.addUserOption(Options::kApp, QStringList{"y", "z"});   // 替换已存在
    EXPECT_EQ(mgr.getUserOption(Options::kApp).size(), 2);
    // 未命中
    EXPECT_TRUE(mgr.getUserOption("not_exists").isEmpty());
}

TEST(OptionsManagerTest, Clear)
{
    OptionsManager mgr;
    mgr.addUserOption("k1", QStringList{"v1"});
    mgr.addUserOption("k2", QStringList{"v2"});
    mgr.clear();
    EXPECT_EQ(mgr.getUserOptions().size(), 0);
}
