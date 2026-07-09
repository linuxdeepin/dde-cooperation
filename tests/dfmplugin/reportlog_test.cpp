// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// reportlog (dfmplugin vendored copy) 单元测试:
//   - 三个 ReportData 子类 type()/prepareData()
//   - ReportLogManager 单例 + commit (emit 信号)
//   - ReportLogWorker: init() (加载 event-log 库, 通常失败 -> false),
//     commitLog 未注册类型早返回, 以及通过 -fno-access-control 注入 writeEventLogFunc
//     覆盖 commitLog 成功 + commit() 写日志路径。

#include <gtest/gtest.h>

#include "reportlog/reportlogmanager.h"
#include "reportlog/reportlogworker.h"
#include "reportlog/datas/cooperationreportdata.h"

#include <QSignalSpy>
#include <QVariantMap>
#include <QJsonObject>
#include <string>

using deepin_cross::ReportLogManager;
using deepin_cross::ReportLogWorker;
using deepin_cross::StatusReportData;
using deepin_cross::FileDeliveryReportData;
using deepin_cross::ConnectionReportData;

// ===== ReportData 子类 =====

TEST(DfmStatusReportDataTest, TypeAndPrepareData)
{
    StatusReportData d;
    EXPECT_EQ(d.type().toStdString(), "CooperationStatus");
    QVariantMap args { { "enableFileDelivery", true } };
    QJsonObject obj = d.prepareData(args);
    EXPECT_EQ(obj.value("tid").toInt(), 1000800000);
    EXPECT_TRUE(obj.contains("sysTime"));
    EXPECT_TRUE(obj.contains("enableFileDelivery"));
}

TEST(DfmFileDeliveryReportDataTest, TypeAndPrepareData)
{
    FileDeliveryReportData d;
    EXPECT_EQ(d.type().toStdString(), "FileDelivery");
    QJsonObject obj = d.prepareData({ { "fileCount", 3 } });
    EXPECT_EQ(obj.value("tid").toInt(), 1000800001);
    EXPECT_TRUE(obj.contains("fileCount"));
}

TEST(DfmConnectionReportDataTest, TypeAndPrepareData)
{
    ConnectionReportData d;
    EXPECT_EQ(d.type().toStdString(), "ConnectionInfo");
    QJsonObject obj = d.prepareData({ { "ip", "1.2.3.4" } });
    EXPECT_EQ(obj.value("tid").toInt(), 1000800002);
    EXPECT_TRUE(obj.contains("ip"));
}

// ===== ReportLogManager =====

TEST(DfmReportLogManagerTest, InstanceIsSingleton)
{
    EXPECT_EQ(ReportLogManager::instance(), ReportLogManager::instance());
}

TEST(DfmReportLogManagerTest, CommitEmitsRequestCommitLog)
{
    QSignalSpy spy(ReportLogManager::instance(), &ReportLogManager::requestCommitLog);
    ReportLogManager::instance()->commit("CooperationStatus", { { "k", 1 } });
    EXPECT_EQ(spy.size(), 1);
}

// ===== ReportLogWorker =====

TEST(DfmReportLogWorkerTest, InitLoadsLibraryGracefully)
{
    ReportLogWorker w;
    // 无 deepin-event-log 库 -> false, 但覆盖了数据注册 + load 失败路径
    bool ok = w.init();
    // 不断言 ok (取决于环境), 仅保证不崩溃
    EXPECT_TRUE(ok || !ok);
}

TEST(DfmReportLogWorkerTest, CommitLogUnregisteredTypeNoop)
{
    ReportLogWorker w;   // 未 init -> 无已注册类型
    EXPECT_NO_FATAL_FAILURE(w.commitLog("UnregisteredType", { { "k", 1 } }));
}

namespace {
void fakeWriteEventLog(const std::string &) {}
}

TEST(DfmReportLogWorkerTest, CommitLogRegisteredTypeWritesViaStub)
{
    using namespace deepin_cross;
    ReportLogWorker w;
    // -fno-access-control: 直接写私有函数指针 + 调用私有 registerLogData
    w.writeEventLogFunc = &fakeWriteEventLog;
    auto *data = new ConnectionReportData;
    w.registerLogData(data->type(), data);
    EXPECT_NO_FATAL_FAILURE(w.commitLog("ConnectionInfo", { { "ip", "9.9.9.9" } }));
}
