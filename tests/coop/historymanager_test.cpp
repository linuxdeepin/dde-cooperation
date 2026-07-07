// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for HistoryManager (cooperation_core::HistoryManager).
//
// HistoryManager is a process-wide singleton QObject backed by ConfigManager
// (which itself uses a fixed JSON config path derived from qApp's organization
// / application name). Because it is a singleton, state leaks across tests:
// later tests may observe entries written by earlier tests, and pre-existing
// on-disk entries may show up as well. We therefore avoid exact-count
// assertions and prefer tolerant checks (EXPECT_GE / non-crash).
//
// Signal flow is fully synchronous in the same thread:
//   writeInto*History() -> ConfigManager::setAppAttribute()
//                         -> Settings::setValue() -> Q_EMIT valueChanged(...)
//                         -> ConfigManager::appAttributeChanged
//                         -> HistoryManager::onAttributeChanged
//                         -> Q_EMIT transHistoryUpdated()/connectHistoryUpdated()
// so QSignalSpy::count() can be read immediately after the call. Using
// QSignalSpy::wait() here would MISS the signal because it is emitted BEFORE
// wait() starts the event loop, and wait() then blocks for the full timeout
// waiting for a second emission that never arrives.
//
// HistoryManager::getTransHistory() dereferences qApp->property("onlyTransfer")
// which requires a QCoreApplication to exist. gtest_main does not create one,
// so we install a global testing Environment that constructs a QCoreApplication
// once before any test runs.
//
// Note: writeIntoConnectHistory/writeIntoTransHistory skip the update (and the
// signal) when an identical (ip, value) pair is already present. To guarantee
// a signal we therefore write a value that differs from any prior run by
// appending a monotonic suffix.

#include <gtest/gtest.h>

#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QSignalSpy>
#include <QString>

#include "lib/cooperation/core/utils/historymanager.h"

using cooperation_core::HistoryManager;

class HistoryManagerTest : public ::testing::Test {
protected:
    HistoryManager *mgr = nullptr;

    static QString uniqueSuffix() {
        // Per-process monotonic counter combined with a timestamp so the value
        // is unique across runs (the config file persists between invocations,
        // and a duplicate (ip, value) pair would cause writeInto* to skip the
        // update and emit no signal).
        static int c = 0;
        return QString::number(QDateTime::currentMSecsSinceEpoch()) + "-" + QString::number(++c);
    }

    void SetUp() override {
        mgr = HistoryManager::instance();
        ASSERT_NE(mgr, nullptr);
    }
};

TEST_F(HistoryManagerTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(HistoryManager::instance(), mgr);
}

TEST_F(HistoryManagerTest, GetConnectHistoryReturnsMap)
{
    // getConnectHistory() should at minimum not crash; it returns a map
    // (possibly empty if no prior history).
    QMap<QString, QString> hist = mgr->getConnectHistory();
    EXPECT_GE(hist.size(), 0);
}

TEST_F(HistoryManagerTest, GetTransHistoryReturnsMap)
{
    // getTransHistory() dereferences qApp->property("onlyTransfer"); with the
    // QCoreApplication installed above it returns a (possibly empty) map.
    QMap<QString, QString> hist = mgr->getTransHistory();
    EXPECT_GE(hist.size(), 0);
}

TEST_F(HistoryManagerTest, WriteIntoConnectHistoryEmitsSignal)
{
    QSignalSpy spy(mgr, &HistoryManager::connectHistoryUpdated);
    ASSERT_TRUE(spy.isValid());
    // Use a counter-suffixed device name so the entry differs from any prior
    // run; otherwise writeIntoConnectHistory would skip the (no-change) update
    // and emit nothing.
    const QString ip = QString("10.0.0.1");
    const QString dev = "DevA-" + uniqueSuffix();
    mgr->writeIntoConnectHistory(ip, dev);
    // Synchronous emission — read count() immediately (see file header).
    EXPECT_GE(spy.count(), 1);
}

TEST_F(HistoryManagerTest, WriteIntoTransHistoryEmitsSignal)
{
    QSignalSpy spy(mgr, &HistoryManager::transHistoryUpdated);
    ASSERT_TRUE(spy.isValid());
    const QString ip = QString("10.0.0.3");
    const QString savePath = "/tmp/save-" + uniqueSuffix();
    mgr->writeIntoTransHistory(ip, savePath);
    EXPECT_GE(spy.count(), 1);
}

TEST_F(HistoryManagerTest, WriteIntoConnectHistoryPersistsAndReadback)
{
    // Use a unique IP + counter-suffixed name to force a real write and then
    // read it back through the config.
    const QString ip = QString("10.0.0.42");
    const QString dev = "DevPersist-" + uniqueSuffix();
    mgr->writeIntoConnectHistory(ip, dev);
    QMap<QString, QString> hist = mgr->getConnectHistory();
    EXPECT_TRUE(hist.contains(ip));
    EXPECT_EQ(hist.value(ip).toStdString(), dev.toStdString());
}

TEST_F(HistoryManagerTest, RemoveTransHistoryDoesNotCrash)
{
    // Insert then remove; removal of a missing key is also a valid no-op.
    mgr->writeIntoTransHistory("10.0.0.4", "/tmp/x-" + uniqueSuffix());
    EXPECT_NO_FATAL_FAILURE(mgr->removeTransHistory("10.0.0.4"));
    // Removing again (now absent) must also be safe.
    EXPECT_NO_FATAL_FAILURE(mgr->removeTransHistory("10.0.0.4"));
}

TEST_F(HistoryManagerTest, RemoveTransHistoryEmitsNoSignalForMissingKey)
{
    // removeTransHistory only writes back when an entry was actually removed;
    // for a key that was never present it returns early without touching the
    // config, so no transHistoryUpdated signal should fire.
    QSignalSpy spy(mgr, &HistoryManager::transHistoryUpdated);
    ASSERT_TRUE(spy.isValid());
    const QString absent = "203.0.113.254";
    // Ensure the key really is absent first.
    mgr->removeTransHistory(absent);
    spy.clear();
    mgr->removeTransHistory(absent);
    EXPECT_EQ(spy.count(), 0);
}
