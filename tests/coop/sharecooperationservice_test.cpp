// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QDir>
#include <QCoreApplication>
#include "lib/cooperation/core/share/sharecooperationservice.h"
#include "lib/cooperation/core/share/sharecooperationservicemanager.h"
#include "lib/cooperation/core/discover/deviceinfo.h"

using ::ShareCooperationService;
using ::ShareCooperationServiceManager;
using ::BarrierType;
using ::ShareServerConfig;
using ::DeviceInfoPointer;
using cooperation_core::DeviceInfo;

// ============ ShareCooperationService ============
// Manager 构造时:client()->setBarrierType(Client),server()->setBarrierType(Server)。
// 单例由 Manager 持有 QSharedPointer,经 client()/server() 取得。

// getter:client 实例为 Client,server 实例为 Server(覆盖 barrierType + Manager 装配)。
TEST(ShareCooperationServiceTest, ClientInstanceIsClientType)
{
    EXPECT_EQ(ShareCooperationServiceManager::instance()->client()->barrierType(), BarrierType::Client);
}

TEST(ShareCooperationServiceTest, ServerInstanceIsServerType)
{
    EXPECT_EQ(ShareCooperationServiceManager::instance()->server()->barrierType(), BarrierType::Server);
}

// setBarrierType 往返(注意:内部会调 terminateAllBarriers 跑 pkill,同步快速返回)。
TEST(ShareCooperationServiceTest, SetBarrierTypeRoundTrip)
{
    auto svc = ShareCooperationServiceManager::instance()->server();
    svc->setBarrierType(BarrierType::Server);
    EXPECT_EQ(svc->barrierType(), BarrierType::Server);
    svc->setBarrierType(BarrierType::Client);
    EXPECT_EQ(svc->barrierType(), BarrierType::Client);
    svc->setBarrierType(BarrierType::Server);  // 还原,保持 server 实例语义
    EXPECT_EQ(svc->barrierType(), BarrierType::Server);
}

// 无进程时 isRunning() 必为 false。
TEST(ShareCooperationServiceTest, IsRunningInitiallyFalse)
{
    EXPECT_FALSE(ShareCooperationServiceManager::instance()->server()->isRunning());
}

// 纯 setter:委托给 CooConfig,不崩即可。
TEST(ShareCooperationServiceTest, SetClientTargetIpDoesNotCrash)
{
    ShareCooperationServiceManager::instance()->client()->setClientTargetIp("10.0.0.1");
    SUCCEED();
}

TEST(ShareCooperationServiceTest, SetEnableCryptoDoesNotCrash)
{
    auto svc = ShareCooperationServiceManager::instance()->client();
    svc->setEnableCrypto(true);
    svc->setEnableCrypto(false);
    SUCCEED();
}

// setBarrierProfile 传入不存在的深层子目录,触发 pdir.exists()==false → mkpath 分支。
TEST(ShareCooperationServiceTest, SetBarrierProfileCreatesNonExistentDir)
{
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    QString deep = tmp.path() + "/sub/deep";  // 不存在
    ShareCooperationServiceManager::instance()->server()->setBarrierProfile(deep);
    EXPECT_TRUE(QDir(deep).exists());
}

// setServerConfig(ShareServerConfig) 三分支:
// (a) client 实例(Client 类型)→ 立即 false。
TEST(ShareCooperationServiceTest, SetServerConfigClientReturnsFalse)
{
    ShareServerConfig cfg;
    EXPECT_FALSE(ShareCooperationServiceManager::instance()->client()->setServerConfig(cfg));
}

// (b) server 实例但 screen_left/right 空 → checkParam false → false。
TEST(ShareCooperationServiceTest, SetServerConfigServerEmptyScreensReturnsFalse)
{
    ShareServerConfig cfg;  // screen_left/screen_right 默认空
    EXPECT_FALSE(ShareCooperationServiceManager::instance()->server()->setServerConfig(cfg));
}

// (c) server 实例 + 有效 screen → 写文件成功 → true。
// 连带执行 setScreen/setScreenLink/setScreenOptions(~90 行纯逻辑)。
TEST(ShareCooperationServiceTest, SetServerConfigServerValidWritesFileReturnsTrue)
{
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    auto svc = ShareCooperationServiceManager::instance()->server();
    svc->setBarrierProfile(tmp.path());  // 保证 configFilename 落在可写临时目录
    ShareServerConfig cfg;
    cfg.screen_left = "10.0.0.1";
    cfg.screen_right = "10.0.0.2";
    EXPECT_TRUE(svc->setServerConfig(cfg));
}

// setServerConfig(DeviceInfoPointer, DeviceInfoPointer):据设备 linkMode 组装 config 再写文件。
TEST(ShareCooperationServiceTest, SetServerConfigFromDevicesDoesNotCrash)
{
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    auto svc = ShareCooperationServiceManager::instance()->server();
    svc->setBarrierProfile(tmp.path());
    DeviceInfoPointer self(new DeviceInfo("10.0.0.1", "Self"));
    DeviceInfoPointer target(new DeviceInfo("10.0.0.2", "Target"));
    EXPECT_NO_FATAL_FAILURE(svc->setServerConfig(self, target));
}

// stopBarrier 无进程时走 terminateAllBarriers(pkill) 分支。
TEST(ShareCooperationServiceTest, StopBarrierNoProcessDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(ShareCooperationServiceManager::instance()->server()->stopBarrier());
}

// terminateAllBarriers 在无 barrier 进程时同步快速返回。
TEST(ShareCooperationServiceTest, TerminateAllBarriersDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(ShareCooperationServiceManager::instance()->server()->terminateAllBarriers());
}

// ============ ShareCooperationServiceManager ============

TEST(ShareCooperationServiceManagerTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(ShareCooperationServiceManager::instance(), ShareCooperationServiceManager::instance());
}

TEST(ShareCooperationServiceManagerTest, ClientReturnsNonNull)
{
    EXPECT_NE(ShareCooperationServiceManager::instance()->client(), nullptr);
}

TEST(ShareCooperationServiceManagerTest, ServerReturnsNonNull)
{
    EXPECT_NE(ShareCooperationServiceManager::instance()->server(), nullptr);
}

TEST(ShareCooperationServiceManagerTest, BarrierProfileReturnsNonEmpty)
{
    EXPECT_FALSE(ShareCooperationServiceManager::instance()->barrierProfile().isEmpty());
}

// stop():同步执行 client->stopBarrier + setClientTargetIp("") + emit stopShareServer(queued)。
// 紧接 processEvents 触发排队槽 handleStopShareSever → _server->stopBarrier(pkill,无进程安全)。
// 关键:此测试必须排在 StartServer 测试之前,否则 pending 的 startShareServer 会被一并触发。
TEST(ShareCooperationServiceManagerTest, StopAndProcessEventsCoverHandleStopShareSever)
{
    EXPECT_NO_FATAL_FAILURE(ShareCooperationServiceManager::instance()->stop());
    QCoreApplication::processEvents();  // flush 排队的 stopShareServer → handleStopShareSever
    SUCCEED();
}

// stopServer() 仅 emit(queued)。
TEST(ShareCooperationServiceManagerTest, StopServerDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(ShareCooperationServiceManager::instance()->stopServer());
}

// startServer():同步 emit startShareServer(queued) + return true。
// 排队槽 handleStartShareSever 会 spawn 进程,故本测试排在最后且不 spin 事件循环,
// pending 事件在进程退出时被安全丢弃(gtest 不跑 Qt 事件循环)。
TEST(ShareCooperationServiceManagerTest, StartServerEmitsAndReturnsTrue)
{
    EXPECT_TRUE(ShareCooperationServiceManager::instance()->startServer("msg"));
}
