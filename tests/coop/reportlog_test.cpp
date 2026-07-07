// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 注:src/base/reportlog 与 dfmplugin/reportlog 是相同 vendored 副本(同 guard 同类)。
// coop_tests 已 glob src/base 版,故测 src/base 版免 include guard 碰撞。
// 不调 ReportLogManager::init()——它会 new ReportLogWorker 并加载 eventlog .so。

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QVariantMap>
#include <QJsonObject>
#include "base/reportlog/reportlogmanager.h"
#include "base/reportlog/reportlogworker.h"
#include "base/reportlog/datas/cooperationreportdata.h"

using deepin_cross::ReportLogManager;
using deepin_cross::ReportLogWorker;
using deepin_cross::StatusReportData;
using deepin_cross::FileDeliveryReportData;
using deepin_cross::ConnectionReportData;

// ============ StatusReportData ============

TEST(StatusReportDataTest, TypeReturnsCooperationStatus)
{
    StatusReportData d;
    EXPECT_EQ(d.type().toStdString(), "CooperationStatus");
}

TEST(StatusReportDataTest, PrepareDataInsertsTid)
{
    StatusReportData d;
    QJsonObject obj = d.prepareData(QVariantMap{});
    EXPECT_EQ(obj.value("tid").toInt(), 1000800000);
    // mergeCommonAttributes 总会插入 sysTime/machineID。
    EXPECT_FALSE(obj.value("sysTime").toString().isEmpty());
}

// ============ FileDeliveryReportData ============

TEST(FileDeliveryReportDataTest, TypeReturnsFileDelivery)
{
    FileDeliveryReportData d;
    EXPECT_EQ(d.type().toStdString(), "FileDelivery");
}

TEST(FileDeliveryReportDataTest, PrepareDataInsertsTid)
{
    FileDeliveryReportData d;
    QJsonObject obj = d.prepareData(QVariantMap{});
    EXPECT_EQ(obj.value("tid").toInt(), 1000800001);
}

// ============ ConnectionReportData ============

TEST(ConnectionReportDataTest, TypeReturnsConnectionInfo)
{
    ConnectionReportData d;
    EXPECT_EQ(d.type().toStdString(), "ConnectionInfo");
}

TEST(ConnectionReportDataTest, PrepareDataInsertsTid)
{
    ConnectionReportData d;
    QJsonObject obj = d.prepareData(QVariantMap{});
    EXPECT_EQ(obj.value("tid").toInt(), 1000800002);
}

// prepareData 应把调用方传入的 args 合并进结果。
TEST(ConnectionReportDataTest, PrepareDataMergesCallerArgs)
{
    ConnectionReportData d;
    QVariantMap args{{"customKey", "customVal"}};
    QJsonObject obj = d.prepareData(args);
    EXPECT_EQ(obj.value("customKey").toString().toStdString(), "customVal");
}

// ============ ReportLogManager ============

TEST(ReportLogManagerTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(ReportLogManager::instance(), ReportLogManager::instance());
}

// commit() 同步 emit requestCommitLog(未 init 时信号无接收者,QSignalSpy 可捕获)。
TEST(ReportLogManagerTest, CommitEmitsRequestCommitLog)
{
    QSignalSpy spy(ReportLogManager::instance(), &ReportLogManager::requestCommitLog);
    QVariantMap args{{"k", "v"}};
    ReportLogManager::instance()->commit("CooperationStatus", args);
    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> payload = spy.takeFirst();
    EXPECT_EQ(payload.at(0).toString().toStdString(), "CooperationStatus");
}

// ============ ReportLogWorker ============
// 公有构造,可直接实例化;commitLog 未注册类型走 "not registed" 早返回。

TEST(ReportLogWorkerTest, CommitLogWithUnregisteredTypeDoesNotCrash)
{
    ReportLogWorker worker;
    worker.commitLog("UnregisteredType", QVariantMap{{"k", "v"}});
    SUCCEED();
}
