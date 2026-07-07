// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QList>
#include "gui/utils/sortfilterworker.h"
#include "discover/deviceinfo.h"
#include "addr_pri.h"

using cooperation_core::SortFilterWorker;
using cooperation_core::DeviceInfo;

// DeviceInfoPointer 是 global namespace typedef (在 deviceinfo.h 文件作用域)
using ::DeviceInfoPointer;

// 访问私有字段 allDeviceList / visibleDeviceList 用于验证内部状态
ACCESS_PRIVATE_FIELD(SortFilterWorker, QList<DeviceInfoPointer>, allDeviceList)
ACCESS_PRIVATE_FIELD(SortFilterWorker, QList<DeviceInfoPointer>, visibleDeviceList)

class SortFilterWorkerTest : public ::testing::Test
{
protected:
    SortFilterWorker *worker = nullptr;
    void SetUp() override
    {
        worker = new SortFilterWorker();
        // 设置 selfip 避免测试设备被当作本机过滤掉
        worker->setSelfip("127.0.0.1");
    }
    void TearDown() override
    {
        delete worker;
        worker = nullptr;
    }
    DeviceInfoPointer makeDev(const QString &ip, const QString &name)
    {
        return DeviceInfoPointer(new DeviceInfo(ip, name));
    }
};

// addDevice 对每个新设备发出 sortFilterResult(index, info),最后发出 filterFinished
TEST_F(SortFilterWorkerTest, AddDeviceEmitsSortFilterResult)
{
    QSignalSpy spy(worker, &SortFilterWorker::sortFilterResult);
    worker->addDevice({makeDev("10.0.0.1", "A")});
    EXPECT_GE(spy.count(), 1);
}

// addDevice 将设备存入 allDeviceList (内部私有列表)
TEST_F(SortFilterWorkerTest, AddDeviceStoresInAllList)
{
    worker->addDevice({makeDev("10.0.0.2", "B"), makeDev("10.0.0.3", "C")});
    auto &all = access_private_field::SortFilterWorkerallDeviceList(*worker);
    EXPECT_EQ(all.size(), 2);
}

// addDevice 同时把设备放入 visibleDeviceList (filter 为空时全部可见)
TEST_F(SortFilterWorkerTest, AddDeviceStoresInVisibleList)
{
    worker->addDevice({makeDev("10.0.0.8", "Vis")});
    auto &vis = access_private_field::SortFilterWorkervisibleDeviceList(*worker);
    EXPECT_EQ(vis.size(), 1);
}

// addDevice 跳过 selfip 匹配的设备
TEST_F(SortFilterWorkerTest, AddDeviceSkipsSelfIp)
{
    worker->addDevice({makeDev("127.0.0.1", "Self"), makeDev("10.0.0.9", "Other")});
    auto &all = access_private_field::SortFilterWorkerallDeviceList(*worker);
    EXPECT_EQ(all.size(), 1);
    EXPECT_EQ(all.first()->ipAddress(), QString("10.0.0.9"));
}

// removeDevice 从 visibleDeviceList 找到设备并发出 deviceRemoved(index)
TEST_F(SortFilterWorkerTest, RemoveDeviceEmitsDeviceRemoved)
{
    worker->addDevice({makeDev("10.0.0.4", "D")});
    QSignalSpy spy(worker, &SortFilterWorker::deviceRemoved);
    worker->removeDevice("10.0.0.4");
    EXPECT_GE(spy.count(), 1);
    // 验证删除后内部列表也同步移除
    auto &all = access_private_field::SortFilterWorkerallDeviceList(*worker);
    EXPECT_EQ(all.size(), 0);
}

// removeDevice 对不存在的 IP 不发信号
TEST_F(SortFilterWorkerTest, RemoveDeviceNoSignalForUnknownIp)
{
    worker->addDevice({makeDev("10.0.0.10", "X")});
    QSignalSpy spy(worker, &SortFilterWorker::deviceRemoved);
    worker->removeDevice("192.168.99.99");
    EXPECT_EQ(spy.count(), 0);
}

// filterDevice 同步发出 sortFilterResult(匹配项) 和 filterFinished
TEST_F(SortFilterWorkerTest, FilterDeviceEmitsFilterFinished)
{
    worker->addDevice({makeDev("10.0.0.5", "Alpha"), makeDev("10.0.0.6", "Beta")});
    QSignalSpy sortSpy(worker, &SortFilterWorker::sortFilterResult);
    QSignalSpy finishSpy(worker, &SortFilterWorker::filterFinished);
    worker->filterDevice("Alpha");
    // filterDevice 是同步的;直接检查 count
    EXPECT_GE(finishSpy.count(), 1);
    EXPECT_GE(sortSpy.count(), 1);
    // visibleDeviceList 应只包含匹配 "Alpha" 的设备
    auto &vis = access_private_field::SortFilterWorkervisibleDeviceList(*worker);
    EXPECT_EQ(vis.size(), 1);
    EXPECT_EQ(vis.first()->deviceName(), QString("Alpha"));
}

// filterDevice 空字符串匹配所有设备
TEST_F(SortFilterWorkerTest, FilterDeviceEmptyMatchesAll)
{
    worker->addDevice({makeDev("10.0.0.11", "Foo"), makeDev("10.0.0.12", "Bar")});
    QSignalSpy finishSpy(worker, &SortFilterWorker::filterFinished);
    worker->filterDevice("");
    EXPECT_GE(finishSpy.count(), 1);
    auto &vis = access_private_field::SortFilterWorkervisibleDeviceList(*worker);
    EXPECT_EQ(vis.size(), 2);
}

// clear 清空所有内部列表,不发信号
TEST_F(SortFilterWorkerTest, ClearEmptiesAllList)
{
    worker->addDevice({makeDev("10.0.0.7", "E")});
    QSignalSpy spy(worker, &SortFilterWorker::sortFilterResult);
    worker->clear();
    // clear 不发 sortFilterResult 信号
    EXPECT_EQ(spy.count(), 0);
    auto &all = access_private_field::SortFilterWorkerallDeviceList(*worker);
    auto &vis = access_private_field::SortFilterWorkervisibleDeviceList(*worker);
    EXPECT_EQ(all.size(), 0);
    EXPECT_EQ(vis.size(), 0);
}

// stop 后 addDevice 不再处理设备
TEST_F(SortFilterWorkerTest, StopBlocksAddDevice)
{
    worker->stop();
    QSignalSpy spy(worker, &SortFilterWorker::sortFilterResult);
    worker->addDevice({makeDev("10.0.0.20", "Stopped")});
    EXPECT_EQ(spy.count(), 0);
    auto &all = access_private_field::SortFilterWorkerallDeviceList(*worker);
    EXPECT_EQ(all.size(), 0);
}
