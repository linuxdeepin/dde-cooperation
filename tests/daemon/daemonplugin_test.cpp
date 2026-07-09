// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "daemoncoreplugin.h"
#include "common/commonutils.h"
#include "service/servicemanager.h"
#include "stub.h"

using namespace daemon_core;
using namespace deepin_cross;

// Fakes for the static CommonUitls methods invoked by initialize().
static void fakeInitLog() {}
static void fakeManageDaemonProcess(const QString &) {}

// Fakes for the ServiceManager members touched by start() / its constructor.
static void fakeNoop() {}
static fastring fakeGenPeerInfoEmpty()
{
    return "";
}

// Apply every stub needed so plugin.start() can construct a ServiceManager
// without binding sockets or launching coroutines.
static void applyPluginStubs(Stub &stub)
{
    stub.set((void *)CommonUitls::initLog, (void *)fakeInitLog);
    stub.set((void *)CommonUitls::manageDaemonProcess,
             (void *)fakeManageDaemonProcess);
    stub.set(ADDR(ServiceManager, asyncDiscovery), fakeNoop);
    stub.set(ADDR(ServiceManager, startRemoteServer), fakeNoop);
    stub.set(ADDR(ServiceManager, localIPCStart), fakeNoop);
    stub.set(ADDR(ServiceManager, genPeerInfo), fakeGenPeerInfoEmpty);
}

// 1. initialize() only calls CommonUitls::initLog + manageDaemonProcess; with
//    those stubbed it returns without touching the log directory or processes.
TEST(DaemonCorePluginTest, InitializeStubbed)
{
    Stub stub;
    stub.set((void *)CommonUitls::initLog, (void *)fakeInitLog);
    stub.set((void *)CommonUitls::manageDaemonProcess,
             (void *)fakeManageDaemonProcess);

    daemon_core::daemonCorePlugin plugin;
    plugin.initialize();
    SUCCEED();
}

// 2. start() bumps the global thread pool, constructs a ServiceManager and
//    calls startRemoteServer; with all heavy pieces stubbed it returns true.
TEST(DaemonCorePluginTest, StartStubbed)
{
    Stub stub;
    applyPluginStubs(stub);

    daemon_core::daemonCorePlugin plugin;
    bool ok = plugin.start();
    EXPECT_TRUE(ok);
}
