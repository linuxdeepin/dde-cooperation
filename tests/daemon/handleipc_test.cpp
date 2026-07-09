// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "service/ipc/handleipcservice.h"
#include "service/comshare.h"
#include "utils/config.h"
#include "common/constant.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/backend.h"
#include "ipc/bridge.h"
#include "service/discoveryjob.h"
#include "service/rpc/sendrpcservice.h"
#include "service/ipc/sendipcservice.h"

#include "stub.h"

#include <QSignalSpy>
#include <QString>
#include <QMap>

// ---------------------------------------------------------------------------
// HandleIpcService extends SlotIPCService (a QObject). Construct with
// `HandleIpcService svc;`. Most Q_INVOKABLE methods build a struct and call
// SendRpcService::instance()->doSendProtoMsg(...) which is an inline emitter
// of workDoSendProtoMsg (queued to the worker thread) — safe to call.
//
// A handful of methods reach DiscoveryJob, whose instance has null
// _announcer_p / _discoverer_p by default. Calling methods that dereference
// those pointers crashes, so we binary-stub them to no-ops / empty returns
// via the Stub class. The stubs match the exact member signatures.
//
// We also stub SendRpcWork::handleDoSendProtoMsg (the worker-thread slot).
// The real handler, when no RemoteServiceSender exists for the app, falls
// through to an else-branch that uses an uninitialized SendResult (a known
// source bug) and segfaults. The synchronous workDoSendProtoMsg signal
// emission from SendRpcService::doSendProtoMsg still fires and is what
// QSignalSpy observes, so coverage of the HandleIpcService callers is
// preserved while avoiding the worker-thread crash.
// ---------------------------------------------------------------------------

// Stub replacements for DiscoveryJob methods that dereference null pointers.
// Signatures must match the real member functions exactly.
static void stub_updateAnnouncApp(DiscoveryJob *, bool /*remove*/, const fastring /*info*/) {}
static void stub_updateAnnouncShare(DiscoveryJob *, const bool /*remove*/, const fastring /*connectIP*/) {}
static fastring stub_baseInfo(const DiscoveryJob *) { return fastring(); }
static void stub_searchDeviceByIp(DiscoveryJob *, const QString & /*ip*/, const bool /*remove*/) {}
static void stub_handleDoSendProtoMsg(SendRpcWork *, const uint32 /*type*/,
                                      const QString /*appName*/,
                                      const QString /*msg*/,
                                      const QByteArray /*data*/) {}

// RAII scope that patches the dangerous daemon methods for its lifetime.
struct DaemonStubScope {
    Stub stub;
    DaemonStubScope()
    {
        stub.set(ADDR(DiscoveryJob, updateAnnouncApp), stub_updateAnnouncApp);
        stub.set(ADDR(DiscoveryJob, updateAnnouncShare), stub_updateAnnouncShare);
        stub.set(ADDR(DiscoveryJob, baseInfo), stub_baseInfo);
        stub.set(ADDR(DiscoveryJob, searchDeviceByIp), stub_searchDeviceByIp);
        stub.set(ADDR(SendRpcWork, handleDoSendProtoMsg), stub_handleDoSendProtoMsg);
    }
};

// 1. BindSignal — bindSignal returns a non-empty 8-hex session string and
// records the mapping in the private _sessionIDs map.
TEST(HandleIpcServiceTest, BindSignal)
{
    HandleIpcService svc;
    QString sesid = svc.bindSignal("app1", "sig1");
    EXPECT_FALSE(sesid.isEmpty());
    // randstr with length 8 produces an 8-char hex string
    EXPECT_EQ(sesid.size(), 8);
    // -fno-access-control: _sessionIDs is private
    EXPECT_TRUE(svc._sessionIDs.contains("app1"));
    EXPECT_EQ(svc._sessionIDs.value("app1"), sesid);
}

// 2. RegisterDiscovery — builds an AppPeerInfo and forwards to
// handleNodeRegister, which touches DiscoveryJob::updateAnnouncApp (stubbed).
TEST(HandleIpcServiceTest, RegisterDiscovery)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    svc.registerDiscovery(false, "app1", "{\"appname\":\"app1\"}");
    SUCCEED();
}

// 3. GetDiscovery — calls DiscoveryJob::getNodes (empty) and baseInfo
// (stubbed to return empty). Result is a JSON string.
TEST(HandleIpcServiceTest, GetDiscovery)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QString result = svc.getDiscovery();
    EXPECT_FALSE(result.isEmpty());
}

// 4. SaveAppConfig — persists via DaemonConfig and is readable back.
TEST(HandleIpcServiceTest, SaveAppConfig)
{
    HandleIpcService svc;
    svc.saveAppConfig("app", "k", "v");
    EXPECT_EQ(DaemonConfig::instance()->getAppConfig("app", "k").c_str(),
              std::string("v"));
}

// 5. SetAuthPassword — non-empty password is stored as the pin.
TEST(HandleIpcServiceTest, SetAuthPassword)
{
    HandleIpcService svc;
    svc.setAuthPassword("123456");
    EXPECT_EQ(DaemonConfig::instance()->getPin().c_str(),
              std::string("123456"));
}

// 6. SetAuthPasswordEmpty — empty password triggers refreshPin, yielding a
// 6-digit random pin.
TEST(HandleIpcServiceTest, SetAuthPasswordEmpty)
{
    HandleIpcService svc;
    svc.setAuthPassword("");
    EXPECT_EQ(DaemonConfig::instance()->getPin().size(), 6u);
}

// 7. GetAuthPassword — returns the current pin.
TEST(HandleIpcServiceTest, GetAuthPassword)
{
    HandleIpcService svc;
    DaemonConfig::instance()->setPin("abc123");
    QString pin = svc.getAuthPassword();
    EXPECT_EQ(pin.toStdString(), std::string("abc123"));
}

// 8. SendMiscMessage — builds MiscJsonCall and emits via SendRpcService.
TEST(HandleIpcServiceTest, SendMiscMessage)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.sendMiscMessage("app1", "{\"x\":1}");
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(1).toString(), QString("app1"));
}

// 9. DoTryConnect — builds UserLoginInfo, emits createRpcSender /
// setTargetAppName / doSendProtoMsg signals. No target name provided so
// targetAppname defaults to appname.
TEST(HandleIpcServiceTest, DoTryConnect)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spyCreate(SendRpcService::instance(),
                         &SendRpcService::workCreateRpcSender);
    QSignalSpy spySend(SendRpcService::instance(),
                       &SendRpcService::workDoSendProtoMsg);

    svc.doTryConnect("app1", "", "127.0.0.1", "pwd");

    EXPECT_EQ(spyCreate.count(), 1);
    EXPECT_EQ(spySend.count(), 1);
    // ip recorded internally
    EXPECT_EQ(svc._ips.value("app1"), QString("127.0.0.1"));
}

// 10. DoTryConnectWithTarget — target name supplied explicitly.
TEST(HandleIpcServiceTest, DoTryConnectWithTarget)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spySend(SendRpcService::instance(),
                       &SendRpcService::workDoSendProtoMsg);
    svc.doTryConnect("app1", "target", "127.0.0.1", "pwd");
    ASSERT_EQ(spySend.count(), 1);
    EXPECT_EQ(spySend.takeFirst().at(1).toString(), QString("app1"));
}

// 11. DoApplyTransfer — builds ApplyTransFiles with APPLY_TRANS_APPLY.
TEST(HandleIpcServiceTest, DoApplyTransfer)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doApplyTransfer("app1", "target", "machine");
    ASSERT_EQ(spy.count(), 1);
}

// 12. DoReplyTransfer — agree true → APPLY_TRANS_CONFIRM, false → REFUSED.
TEST(HandleIpcServiceTest, DoReplyTransferAgree)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doReplyTransfer("app1", "target", "machine", true);
    ASSERT_EQ(spy.count(), 1);
}

TEST(HandleIpcServiceTest, DoReplyTransferRefuse)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doReplyTransfer("app1", "target", "machine", false);
    ASSERT_EQ(spy.count(), 1);
    // refusing updates Comshare status to DISCONNECT
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// 13. DoApplyShare — emits createRpcSender + doSendProtoMsg and sets status
// to SHARE_CONNECT.
TEST(HandleIpcServiceTest, DoApplyShare)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doApplyShare("app1", "target", "127.0.0.1", "data");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_SHARE_CONNECT);
    EXPECT_EQ(svc._ips.value("app1"), QString("127.0.0.1"));
    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
}

// 14. DoDisconnectShare — updates status to DISCONNECT, touches
// DiscoveryJob::updateAnnouncShare (stubbed), emits doSendProtoMsg.
TEST(HandleIpcServiceTest, DoDisconnectShare)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doDisconnectShare("app1", "target", "msg");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// 15. DoReplyShare — SHARE_CONNECT_REFUSE resets status; CONFIRM does not.
TEST(HandleIpcServiceTest, DoReplyShareRefuse)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    Comshare::instance()->updateStatus(CURRENT_STATUS_SHARE_CONNECT);
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doReplyShare("app1", "target", SHARE_CONNECT_REFUSE);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceTest, DoReplyShareConfirm)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    Comshare::instance()->updateStatus(CURRENT_STATUS_SHARE_CONNECT);
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doReplyShare("app1", "target", SHARE_CONNECT_COMFIRM);
    ASSERT_EQ(spy.count(), 1);
    // confirm does not change status here
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_SHARE_CONNECT);
    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
}

// 16. DoStartShare — sets status to SHARE_START, emits doSendProtoMsg.
TEST(HandleIpcServiceTest, DoStartShare)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doStartShare("app1", "screen");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_SHARE_START);
    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
}

// 17. DoStopShare — emits doSendProtoMsg and resets status. Both flag
// variants exercise the same path.
TEST(HandleIpcServiceTest, DoStopShareAll)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doStopShare("app1", "target", SHARE_STOP_ALL);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceTest, DoStopShareClient)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doStopShare("app1", "target", SHARE_STOP_CLIENT);
    ASSERT_EQ(spy.count(), 1);
}

// 18. DoDisconnectCallback — emits doSendProtoMsg, removes ping, resets
// status.
TEST(HandleIpcServiceTest, DoDisconnectCallback)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doDisconnectCallback("app1");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// 19. DoCancelShareApply — resets status, emits doSendProtoMsg.
TEST(HandleIpcServiceTest, DoCancelShareApply)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.doCancelShareApply("app1");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// 20. DoAsyncSearch — calls DiscoveryJob::searchDeviceByIp (stubbed).
TEST(HandleIpcServiceTest, DoAsyncSearch)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    svc.doAsyncSearch("1.2.3.4", false);
    SUCCEED();
}

TEST(HandleIpcServiceTest, DoAsyncSearchRemove)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    svc.doAsyncSearch("1.2.3.4", true);
    SUCCEED();
}

// 21. UpdateCooperationStatus — forwards to Comshare::updateStatus.
TEST(HandleIpcServiceTest, UpdateCooperationStatus)
{
    HandleIpcService svc;
    svc.updateCooperationStatus(CURRENT_STATUS_SHARE_START);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_SHARE_START);
    // reset
    svc.updateCooperationStatus(CURRENT_STATUS_DISCONNECT);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// 22. GetCurrentCooperationStatus — true only when status is SHARE_START.
TEST(HandleIpcServiceTest, GetCurrentCooperationStatus)
{
    HandleIpcService svc;
    Comshare::instance()->updateStatus(CURRENT_STATUS_SHARE_START);
    EXPECT_TRUE(svc.getCurrentCooperationStatus());

    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
    EXPECT_FALSE(svc.getCurrentCooperationStatus());
}

// 23. HandleSessionSignal — invokeMethod with an unregistered signal name
// fails silently (QueuedConnection); the call must not crash.
TEST(HandleIpcServiceTest, HandleSessionSignal)
{
    HandleIpcService svc;
    svc.handleSessionSignal("nonexistentSignal", 0, "msg");
    SUCCEED();
}

// 24. HandleNodeRegister — unreg=false path: only calls
// DiscoveryJob::updateAnnouncApp (stubbed). unreg=true path additionally
// calls SendRpcService::removePing + SendIpcService::handleRemoveSessionByAppName.
TEST(HandleIpcServiceTest, HandleNodeRegister)
{
    DaemonStubScope dstub;
    HandleIpcService svc;

    AppPeerInfo appPeer;
    appPeer.appname = "app1";
    appPeer.json = "{}";

    svc.handleNodeRegister(false, appPeer.as_json());
    SUCCEED();
}

TEST(HandleIpcServiceTest, HandleNodeRegisterUnreg)
{
    DaemonStubScope dstub;
    HandleIpcService svc;

    AppPeerInfo appPeer;
    appPeer.appname = "app2";
    appPeer.json = "{}";

    svc.handleNodeRegister(true, appPeer.as_json());
    SUCCEED();
}

// Additional coverage: handleBackApplyTransFiles builds ApplyTransFiles from
// the json and emits doSendProtoMsg.
TEST(HandleIpcServiceTest, HandleBackApplyTransFiles)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ApplyTransFiles info;
    info.appname = "app1";
    info.tarAppname = "target";
    info.machineName = "machine";
    info.type = APPLY_TRANS_APPLY;
    co::Json param = info.as_json();

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleBackApplyTransFiles(param);
    ASSERT_EQ(spy.count(), 1);
}

// handleTryConnect parses ConnectParam and emits signals.
TEST(HandleIpcServiceTest, HandleTryConnect)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ipc::ConnectParam param;
    param.appName = "app1";
    param.targetAppname = "target";
    param.host = "127.0.0.1";
    param.password = "pwd";

    QSignalSpy spyCreate(SendRpcService::instance(),
                         &SendRpcService::workCreateRpcSender);
    QSignalSpy spySend(SendRpcService::instance(),
                       &SendRpcService::workDoSendProtoMsg);
    svc.handleTryConnect(param.as_json());
    EXPECT_EQ(spyCreate.count(), 1);
    EXPECT_EQ(spySend.count(), 1);
    EXPECT_EQ(svc._ips.value("app1"), QString("127.0.0.1"));
}

// handleShareConnect parses ShareConnectApply, records ip, emits signals and
// sets status to SHARE_CONNECT.
TEST(HandleIpcServiceTest, HandleShareConnect)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectApply param;
    param.appName = "app1";
    param.tarAppname = "target";
    param.tarIp = "127.0.0.1";

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleShareConnect(param.as_json());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(svc._ips.value("app1"), QString("127.0.0.1"));
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_SHARE_CONNECT);
    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
}

// handleShareDisConnect resets status, touches updateAnnouncShare (stubbed),
// emits doSendProtoMsg.
TEST(HandleIpcServiceTest, HandleShareDisConnect)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareDisConnect info;
    info.appName = "app1";
    info.tarAppname = "";

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleShareDisConnect(info.as_json());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// handleShareConnectReply — refuse resets status.
TEST(HandleIpcServiceTest, HandleShareConnectReplyRefuse)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectReply reply;
    reply.appName = "app1";
    reply.reply = SHARE_CONNECT_REFUSE;

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleShareConnectReply(reply.as_json());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// handleShareConnectDisApply resets status, emits doSendProtoMsg.
TEST(HandleIpcServiceTest, HandleShareConnectDisApply)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectDisApply reply;
    reply.appName = "app1";

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleShareConnectDisApply(reply.as_json());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// handleShareStop emits doSendProtoMsg and resets status.
TEST(HandleIpcServiceTest, HandleShareStop)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareStop st;
    st.appName = "app1";
    st.tarAppname = "target";

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleShareStop(st.as_json());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// handleDisConnectCb emits doSendProtoMsg, removes ping, resets status.
TEST(HandleIpcServiceTest, HandleDisConnectCb)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareDisConnect info;
    info.appName = "app1";
    info.tarAppname = "";

    QSignalSpy spy(SendRpcService::instance(),
                   &SendRpcService::workDoSendProtoMsg);
    svc.handleDisConnectCb(info.as_json());
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(Comshare::instance()->currentStatus(),
              CURRENT_STATUS_DISCONNECT);
}

// ---- Extended coverage: doTransferFile / doOperateJob / newTransSendJob ----

TEST(HandleIpcServiceCovTest, DoTransferFileInvalidSession)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    // no bound session -> newTransSendJob returns early
    svc.doTransferFile("invalidsession", "target", 1, QStringList{"/tmp/a"}, false, "/save");
    SUCCEED();
}

TEST(HandleIpcServiceCovTest, DoTransferFileValidSession)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    // bind a session first
    svc.bindSignal("transapp", "sigTrans");
    // _sessionIDs now has transapp; doTransferFile uses the session id
    // get the session id via _sessionIDs
    QString sesid = svc._sessionIDs.value("transapp");
    svc.doTransferFile(sesid, "target", 2, QStringList{"/tmp/x"}, false, "/save");
    SUCCEED();
}

TEST(HandleIpcServiceCovTest, DoOperateJob)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    bool ret = svc.doOperateJob(TRANS_CANCEL, 5, "opapp");
    // doJobAction on nonexistent job returns false
    EXPECT_FALSE(ret);
}

TEST(HandleIpcServiceCovTest, HandleJobActionsResume)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ipc::TransJobParam param;
    param.appname = "jaapp";
    param.job_id = 7;
    param.session = "s";
    co::Json json = param.as_json();
    bool ret = svc.handleJobActions(BACK_RESUME_JOB, json);
    EXPECT_FALSE(ret);
}

TEST(HandleIpcServiceCovTest, HandleJobActionsCancel)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ipc::TransJobParam param;
    param.appname = "jaapp2";
    param.job_id = 8;
    co::Json json = param.as_json();
    bool ret = svc.handleJobActions(BACK_CANCEL_JOB, json);
    EXPECT_FALSE(ret);
}

TEST(HandleIpcServiceCovTest, HandleJobActionsPause)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ipc::TransJobParam param;
    param.appname = "jaapp3";
    param.job_id = 9;
    co::Json json = param.as_json();
    // unknown type -> TRANS_PAUSE path
    bool ret = svc.handleJobActions(9999, json);
    EXPECT_FALSE(ret);
}

TEST(HandleIpcServiceCovTest, HandleBackApplyTransFiles)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ApplyTransFiles info;
    info.appname = "baapp";
    info.tarAppname = "batarget";
    info.type = APPLY_TRANS_CONFIRM;
    co::Json json = info.as_json();
    svc.handleBackApplyTransFiles(json);
    SUCCEED();
}

TEST(HandleIpcServiceCovTest, HandleSearchDevice)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    SearchDevice de;
    de.ip = "1.2.3.4";
    de.remove = false;
    co::Json json = de.as_json();
    svc.handleSearchDevice(json);
    // async (QUNIGO) - just exercise the parse path
    SUCCEED();
}

TEST(HandleIpcServiceCovTest, HandleShareConnectFromRpc)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectApply param;
    param.appName = "scapp";
    param.tarAppname = "sctarget";
    param.tarIp = "1.2.3.4";
    param.ip = "1.2.3.4";
    co::Json json = param.as_json();
    svc.handleShareConnect(json);
    EXPECT_EQ(Comshare::instance()->currentStatus(), CURRENT_STATUS_SHARE_CONNECT);
    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceCovTest, HandleShareDisConnectFromRpc)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareDisConnect info;
    info.appName = "sdapp";
    info.tarAppname = "";
    co::Json json = info.as_json();
    svc.handleShareDisConnect(json);
    EXPECT_EQ(Comshare::instance()->currentStatus(), CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceCovTest, HandleShareConnectReplyConfirm)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectReply reply;
    reply.appName = "rcapp";
    reply.reply = SHARE_CONNECT_COMFIRM;
    co::Json json = reply.as_json();
    svc.handleShareConnectReply(json);
    SUCCEED();
}

TEST(HandleIpcServiceCovTest, HandleShareConnectReplyRefuse)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectReply reply;
    reply.appName = "rcapp2";
    reply.reply = SHARE_CONNECT_REFUSE;
    co::Json json = reply.as_json();
    svc.handleShareConnectReply(json);
    EXPECT_EQ(Comshare::instance()->currentStatus(), CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceCovTest, HandleShareStopFromRpc)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareStop st;
    st.appName = "ssapp";
    st.tarAppname = "sstarget";
    co::Json json = st.as_json();
    svc.handleShareStop(json);
    EXPECT_EQ(Comshare::instance()->currentStatus(), CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceCovTest, HandleDisConnectCbFromRpc)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareDisConnect info;
    info.appName = "dcapp";
    info.tarAppname = "";
    co::Json json = info.as_json();
    svc.handleDisConnectCb(json);
    EXPECT_EQ(Comshare::instance()->currentStatus(), CURRENT_STATUS_DISCONNECT);
}

TEST(HandleIpcServiceCovTest, HandleShareConnectDisApply)
{
    DaemonStubScope dstub;
    HandleIpcService svc;
    ShareConnectDisApply reply;
    reply.appName = "daapp";
    co::Json json = reply.as_json();
    svc.handleShareConnectDisApply(json);
    EXPECT_EQ(Comshare::instance()->currentStatus(), CURRENT_STATUS_DISCONNECT);
}
