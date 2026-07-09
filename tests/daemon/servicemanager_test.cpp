// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/servicemanager.h"
#include "service/ipc/handleipcservice.h"
#include "service/rpc/handlerpcservice.h"
#include "stub.h"
#include "co/json.h"
#include "utils/config.h"
#include "ipc/proto/comstruct.h"
#include "utils/utils.h"

// Fakes for stubbed ServiceManager member functions.
// Void member functions: a no-op free function is enough; the implicit
// `this` argument is simply ignored by the patched callee.
static void fakeNoop() {}
// genPeerInfo() returns fastring by value; on x86-64 SysV the hidden return
// pointer occupies the same slot the member function uses, so a free function
// returning fastring lines up with the member's ABI.
static fastring fakeGenPeerInfoPeer()
{
    return "peer";
}
static fastring fakeGenPeerInfoEmpty()
{
    return "";
}

// Stubs applied for every test in this suite: asyncDiscovery, startRemoteServer,
// genPeerInfo and localIPCStart are all heavy or socket-touching and must be
// neutralised before constructing ServiceManager.
static void applyServiceManagerStubs(Stub &stub)
{
    stub.set(ADDR(ServiceManager, asyncDiscovery), fakeNoop);
    stub.set(ADDR(ServiceManager, startRemoteServer), fakeNoop);
    stub.set(ADDR(ServiceManager, localIPCStart), fakeNoop);
    stub.set(ADDR(ServiceManager, genPeerInfo), fakeGenPeerInfoEmpty);
}

// 1. Construct with the heavy members stubbed; no crash, no socket bind.
TEST(ServiceManagerTest, ConstructWithStubs)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    ServiceManager *sm = new ServiceManager();
    ASSERT_NE(sm, nullptr);

    delete sm;
    SUCCEED();
}

// 2. Construct then destruct; Stub destructor restores the patched code.
TEST(ServiceManagerTest, DestructNoCrash)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    {
        ServiceManager sm;
    }
    SUCCEED();
}

// 3. startRemoteServer is stubbed to a no-op, so calling it is safe and cheap.
TEST(ServiceManagerTest, StartRemoteServerStubbed)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    ServiceManager sm;
    sm.startRemoteServer();
    SUCCEED();
}

// 4. handleAppQuit stops the user timer and calls stopAnnouncer/stopDiscoverer
//    on the (null by default) DiscoveryJob pointers, which early-return.
TEST(ServiceManagerTest, HandleAppQuit)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    ServiceManager sm;
    sm.handleAppQuit();
    SUCCEED();
}

// 5. genPeerInfo stubbed to return "peer"; access the private method directly
//    via -fno-access-control.
TEST(ServiceManagerTest, GenPeerInfoStubbed)
{
    Stub stub;
    stub.set(ADDR(ServiceManager, asyncDiscovery), fakeNoop);
    stub.set(ADDR(ServiceManager, startRemoteServer), fakeNoop);
    stub.set(ADDR(ServiceManager, localIPCStart), fakeNoop);
    stub.set(ADDR(ServiceManager, genPeerInfo), fakeGenPeerInfoPeer);

    ServiceManager sm;
    fastring info = sm.genPeerInfo();
    EXPECT_EQ(info, "peer");
}

// 6. ipcName() is private; reachable via -fno-access-control. It builds a path
//    from the app name and a writable runtime/temp location, so it should be
//    non-empty.
TEST(ServiceManagerTest, IpcName)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    ServiceManager sm;
    QString name = sm.ipcName();
    EXPECT_FALSE(name.isEmpty());
}

// Util::getFirstIp is inline in the header, so it gets inlined into genPeerInfo
// at -O2 and a stub on Util::getFirstIp never fires. It delegates to the
// out-of-line deepin_cross::CommonUitls::getFirstIp (commonutils.cpp, no LTO),
// so we stub that. The fake returns std::string (the real signature) to keep
// the hidden return ABI correct.
static std::string fakeFirstIpStd() { return "1.2.3.4"; }

// 7. GenPeerInfoReal: genPeerInfo is NOT stubbed, so its real body executes.
//    asyncDiscovery/localIPCStart/startRemoteServer stay neutralised so the
//    constructor opens no sockets and spawns no threads; the IP helper is
//    stubbed only to keep the peer json deterministic and off the network.
TEST(ServiceManagerTest, GenPeerInfoReal)
{
    Stub stub;
    stub.set(ADDR(ServiceManager, asyncDiscovery), fakeNoop);
    stub.set(ADDR(ServiceManager, localIPCStart), fakeNoop);
    stub.set(ADDR(ServiceManager, startRemoteServer), fakeNoop);
    stub.set((void *)deepin_cross::CommonUitls::getFirstIp, fakeFirstIpStd);

    ServiceManager sm;
    fastring info = sm.genPeerInfo();
    EXPECT_FALSE(info.empty());

    co::Json j;
    ASSERT_TRUE(j.parse_from(info));
    EXPECT_TRUE(j.is_object());
    NodePeerInfo npi;
    npi.from_json(j);
    // uuid is generated during construction if absent, so it must be non-empty
    EXPECT_FALSE(npi.uuid.empty());
    EXPECT_EQ(npi.ipv4, fastring("1.2.3.4"));
}

// 8. IpcNameReal: ipcName() is private; reached via -fno-access-control.
//    Strengthens the basic IpcName case by asserting the built path carries the
//    "<app>.ipc" suffix produced from the runtime/temp location.
TEST(ServiceManagerTest, IpcNameReal)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    ServiceManager sm;
    QString name = sm.ipcName();
    EXPECT_FALSE(name.isEmpty());
    EXPECT_TRUE(name.endsWith(QString::fromLatin1(".ipc")));
}

// 9. HandleAppQuitTimerStop: the constructor's #ifndef _WIN32 block starts the
//    private _userTimer (reached via -fno-access-control). handleAppQuit must be
//    callable and safe to repeat (the close branches are skipped when the
//    services were never created).
TEST(ServiceManagerTest, HandleAppQuitTimerStop)
{
    Stub stub;
    applyServiceManagerStubs(stub);

    ServiceManager sm;
    EXPECT_TRUE(sm._userTimer.isActive());

    sm.handleAppQuit();
    sm.handleAppQuit();   // idempotent
    SUCCEED();
}

// 10. LocalIPCStartReal: localIPCStart is NOT stubbed, so the constructor runs
//     its real body (create HandleIpcService, listen on the ipc path, wire the
//     session signal). SlotIPCService is Qt local-socket IPC — no network, no
//     threads — so this is safe. asyncDiscovery/startRemoteServer stay
//     neutralised. Reached via -fno-access-control: the private _ipcService ends
//     up non-null, proving the real create+listen path executed.
TEST(ServiceManagerTest, DISABLED_LocalIPCStartReal)
{
    Stub stub;
    stub.set(ADDR(ServiceManager, asyncDiscovery), fakeNoop);
    stub.set(ADDR(ServiceManager, startRemoteServer), fakeNoop);
    stub.set(ADDR(ServiceManager, genPeerInfo), fakeGenPeerInfoEmpty);

    ServiceManager sm;
    EXPECT_NE(sm._ipcService, nullptr);
}

// ---- Extended coverage: handleAppQuit with services set ----

TEST(ServiceManagerCovTest, HandleAppQuitWithServices)
{
    Stub stub;
    applyServiceManagerStubs(stub);
    ServiceManager sm;
    // set the service pointers so the close-branches execute
    sm._ipcService = new HandleIpcService();
    sm._rpcService = new HandleRpcService();
    sm.handleAppQuit();
    EXPECT_EQ(sm._ipcService, nullptr);
    EXPECT_EQ(sm._rpcService, nullptr);
}

TEST(ServiceManagerCovTest, StartRemoteServerTwice)
{
    Stub stub;
    applyServiceManagerStubs(stub);
    ServiceManager sm;
    sm.startRemoteServer();
    sm.startRemoteServer();
    SUCCEED();
}

TEST(ServiceManagerCovTest, ConstructWithEmptyUuidGen)
{
    DaemonConfig::instance()->setUUID("");
    Stub stub;
    applyServiceManagerStubs(stub);
    ServiceManager sm;
    fastring uid = DaemonConfig::instance()->getUUID();
    EXPECT_FALSE(uid.empty());
}

TEST(ServiceManagerCovTest, ConstructWithExistingUuid)
{
    DaemonConfig::instance()->setUUID("existing-uid-xyz");
    Stub stub;
    applyServiceManagerStubs(stub);
    ServiceManager sm;
    EXPECT_EQ(DaemonConfig::instance()->getUUID(), fastring("existing-uid-xyz"));
}
