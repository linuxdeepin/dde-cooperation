// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for CooperationUtil (cooperation_core::CooperationUtil).
//
// CooperationUtil is a process-wide singleton QObject. Its instance() does not
// dereference qApp directly, but several related facilities (ConfigManager,
// deviceInfo()) rely on QCoreApplication being available. The QtAppEnvironment
// registered in historymanager_test.cpp runs SetUp() before any test, so a
// QCoreApplication already exists when these tests run — we deliberately do
// NOT register a second one (that would create two QApp instances and crash).
//
// setStorageConfig emits the storageConfig(QString) signal synchronously
// (direct emit, no queueing), so QSignalSpy::count() is read immediately.
//
// closeOption() reads ConfigManager::appAttribute(CacheGroup, CloseOptionKey)
// which may be empty when no value has been persisted; we only assert no-crash.
// localIPAddress() returns the first non-loopback IPv4 found via
// deepin_cross::CommonUitls::getFirstIp(), which may be empty on hosts without
// a usable network interface; we only assert no-crash.

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>
#include <QVariantMap>

#include "lib/cooperation/core/utils/cooperationutil.h"

using cooperation_core::CooperationUtil;

TEST(CooperationUtilTest, InstanceReturnsNonNullSingleton)
{
    EXPECT_NE(CooperationUtil::instance(), nullptr);
}

TEST(CooperationUtilTest, InstanceReturnsSamePointer)
{
    auto *a = CooperationUtil::instance();
    auto *b = CooperationUtil::instance();
    EXPECT_EQ(a, b);
}

TEST(CooperationUtilTest, LocalIPAddressReturnsQStringNoCrash)
{
    // 静态方法; 返回值可能为空(无网卡)或 IP 字符串, 只验证不崩
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::localIPAddress());
    QString ip = CooperationUtil::localIPAddress();
    (void)ip;  // 避免未使用变量告警
    SUCCEED();
}

TEST(CooperationUtilTest, CloseOptionReturnsQStringNoCrash)
{
    // closeOption() 读取 ConfigManager 缓存项, 可能为空(未持久化), 只验证不崩
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::closeOption());
    QString opt = CooperationUtil::closeOption();
    (void)opt;
    SUCCEED();
}

TEST(CooperationUtilTest, SaveOptionDoesNotCrash)
{
    // saveOption 写入 ConfigManager, 不直接返回值; 只验证不崩
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::saveOption(true));
    EXPECT_NO_FATAL_FAILURE(CooperationUtil::saveOption(false));
}

TEST(CooperationUtilTest, SetStorageConfigEmitsSignalSynchronously)
{
    auto *util = CooperationUtil::instance();
    ASSERT_NE(util, nullptr);
    QSignalSpy spy(util, &CooperationUtil::storageConfig);
    ASSERT_TRUE(spy.isValid());
    const QString payload = "/tmp/coop-util-test-storage-9";
    util->setStorageConfig(payload);
    // 同步 emit, 直接读取 count()
    EXPECT_GE(spy.count(), 1);
    if (spy.count() > 0) {
        const auto args = spy.takeFirst();
        ASSERT_EQ(args.size(), 1);
        EXPECT_EQ(args.at(0).toString().toStdString(), payload.toStdString());
    }
}
