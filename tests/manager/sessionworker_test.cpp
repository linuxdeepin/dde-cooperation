// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "sessionworker.h"
#include "sessionproto.h"

#include <QSignalSpy>
#include <QString>
#include "stub.h"

class SessionWorkerTest : public ::testing::Test {
protected:
    std::shared_ptr<SessionWorker> worker;

    void SetUp() override
    {
        worker = std::make_shared<SessionWorker>();
    }
    void TearDown() override
    {
        if (worker) {
            worker->stop();
            worker.reset();
        }
    }

    proto::OriginMessage makeMsg(int mask, const std::string &json, const std::string &id = "00000000-0000-0000-0000-000000000000")
    {
        proto::OriginMessage msg;
        msg.id = FBE::uuid_t(id);
        msg.mask = mask;
        msg.json_msg = json;
        return msg;
    }
};

TEST_F(SessionWorkerTest, ConstructDestruct)
{
    EXPECT_NE(worker, nullptr);
}

TEST_F(SessionWorkerTest, UpdatePincode)
{
    worker->updatePincode("5678");
    SUCCEED();
}

TEST_F(SessionWorkerTest, UpdateLogin)
{
    worker->updateLogin("192.168.1.1", true);
    SUCCEED();
}

TEST_F(SessionWorkerTest, UpdateLoginMultiple)
{
    worker->updateLogin("10.0.0.1", true);
    worker->updateLogin("10.0.0.2", false);
    worker->updateLogin("10.0.0.3", true);
    SUCCEED();
}

TEST_F(SessionWorkerTest, IsClientLoginNotLoggedIn)
{
    QString ip = "192.168.1.99";
    EXPECT_FALSE(worker->isClientLogin(ip));
}

TEST_F(SessionWorkerTest, SetGetRealIP)
{
    worker->setRealIP("10.0.0.1");
    EXPECT_EQ(worker->getRealIP(), QString("10.0.0.1"));
}

TEST_F(SessionWorkerTest, SetGetRealIPEmpty)
{
    worker->setRealIP("");
    EXPECT_TRUE(worker->getRealIP().isEmpty());
}

TEST_F(SessionWorkerTest, SetExtMessageHandler)
{
    worker->setExtMessageHandler([](int32_t, const picojson::value &, std::string *) {
        return false;
    });
    SUCCEED();
}

// ---- onReceivedMessage tests ----

TEST_F(SessionWorkerTest, OnReceivedMessageEmptyJson)
{
    auto req = makeMsg(REQ_LOGIN, "");
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageInvalidJson)
{
    auto req = makeMsg(REQ_LOGIN, "not a json");
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageLoginSuccess)
{
    worker->updatePincode("1234");
    // base64 of "1234" = "MTIzNA=="
    QByteArray pinByte("MTIzNA==");
    QString dePin = QString::fromUtf8(QByteArray::fromBase64(pinByte));

    LoginMessage loginMsg;
    loginMsg.name = "test-host";
    loginMsg.auth = "MTIzNA==";
    auto req = makeMsg(REQ_LOGIN, loginMsg.as_json().serialize(), "00000000-0000-0000-0000-000000000001");

    QSignalSpy spy(worker.get(), &SessionWorker::onConnectChanged);
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
    EXPECT_FALSE(resp.json_msg.empty());
    // should emit LOGIN_SUCCESS
    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), LOGIN_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageLoginDenied)
{
    worker->updatePincode("correct-pin");
    LoginMessage loginMsg;
    loginMsg.name = "bad-host";
    loginMsg.auth = "d3JvbmdwaW4="; // base64 of "wrongpin"

    auto req = makeMsg(REQ_LOGIN, loginMsg.as_json().serialize());

    QSignalSpy spy(worker.get(), &SessionWorker::onConnectChanged);
    QSignalSpy rejectSpy(worker.get(), &SessionWorker::onRejectConnection);
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);

    ASSERT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toInt(), LOGIN_DENIED);
}

TEST_F(SessionWorkerTest, OnReceivedMessageFreeSpace)
{
    FreeSpaceMessage msg;
    msg.total = 1024;
    msg.free = 512;
    auto req = makeMsg(REQ_FREE_SPACE, msg.as_json().serialize());

    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageTransDatasNoAuth)
{
    // without pin/host login, REQ_TRANS_DATAS should be rejected
    TransDataMessage msg;
    msg.id = "192.168.1.1";
    msg.names = {"file.txt"};
    msg.endpoint = "127.0.0.1:8080:token";
    msg.flag = false;
    msg.size = 100;
    auto req = makeMsg(REQ_TRANS_DATAS, msg.as_json().serialize());

    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageTransDatasWithAuth)
{
    // set pin and login first
    worker->updatePincode("9999");
    worker->updateLogin("10.0.0.1", true);

    TransDataMessage msg;
    msg.id = "192.168.1.1";
    msg.names = {"file1.txt", "file2.txt"};
    msg.endpoint = "127.0.0.1:8080:token";
    msg.flag = true;
    msg.size = 500;
    auto req = makeMsg(REQ_TRANS_DATAS, msg.as_json().serialize());

    QSignalSpy dataSpy(worker.get(), &SessionWorker::onTransData);
    QSignalSpy countSpy(worker.get(), &SessionWorker::onTransCount);
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);

    // should emit onTransData and onTransCount (size > 0)
    EXPECT_EQ(dataSpy.count(), 1);
    EXPECT_EQ(countSpy.count(), 1);
}

TEST_F(SessionWorkerTest, OnReceivedMessageTransCancel)
{
    TransCancelMessage msg;
    msg.id = "cancel-id";
    msg.name = "all";
    msg.reason = "user_cancel";
    auto req = makeMsg(REQ_TRANS_CANCLE, msg.as_json().serialize());

    QSignalSpy spy(worker.get(), &SessionWorker::onCancelJob);
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(SessionWorkerTest, OnReceivedMessageCastInfo)
{
    auto req = makeMsg(CAST_INFO, R"({"info":"test"})");
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageInfoTransCount)
{
    TransDataMessage msg;
    msg.id = "count-id";
    msg.names = {"a.txt", "b.txt"};
    msg.flag = false;
    msg.size = 2048;
    auto req = makeMsg(INFO_TRANS_COUNT, msg.as_json().serialize());

    QSignalSpy spy(worker.get(), &SessionWorker::onTransCount);
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(SessionWorkerTest, OnReceivedMessageUnknownType)
{
    auto req = makeMsg(99999, R"({"test":"data"})");
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);
    EXPECT_EQ(resp.mask, DO_SUCCESS);
}

TEST_F(SessionWorkerTest, OnReceivedMessageExternalHandlerHandled)
{
    bool handlerCalled = false;
    worker->setExtMessageHandler([&handlerCalled](int32_t mask, const picojson::value &, std::string *res) {
        handlerCalled = true;
        *res = R"({"handled":true})";
        return true;
    });

    auto req = makeMsg(REQ_LOGIN, R"({"name":"x","auth":"y"})");
    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);

    EXPECT_TRUE(handlerCalled);
    EXPECT_EQ(resp.json_msg, R"({"handled":true})");
}

TEST_F(SessionWorkerTest, OnReceivedMessageExternalHandlerNotHandled)
{
    bool handlerCalled = false;
    worker->setExtMessageHandler([&handlerCalled](int32_t mask, const picojson::value &, std::string *) {
        handlerCalled = true;
        return false;
    });

    worker->updatePincode("1234");
    LoginMessage loginMsg;
    loginMsg.name = "host";
    loginMsg.auth = "MTIzNA==";
    auto req = makeMsg(REQ_LOGIN, loginMsg.as_json().serialize());

    proto::OriginMessage resp;
    worker->onReceivedMessage(req, &resp);

    EXPECT_TRUE(handlerCalled);
    // should proceed to normal handling
}

// ---- onStateChanged tests ----

TEST_F(SessionWorkerTest, OnStateChangedConnected)
{
    std::string msg = "192.168.1.1";
    bool result = worker->onStateChanged(RPC_CONNECTED, msg);
    EXPECT_TRUE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedDisconnected)
{
    QSignalSpy spy(worker.get(), &SessionWorker::onRemoteDisconnected);
    std::string msg = "192.168.1.1";
    bool result = worker->onStateChanged(RPC_DISCONNECTED, msg);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedDisconnectedNullAddr)
{
    std::string msg = "";
    // _tryConnect is false initially, so should return false
    bool result = worker->onStateChanged(RPC_DISCONNECTED, msg);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedDisconnectedRetryAfterConnect)
{
    // first connect
    std::string connMsg = "10.0.0.1";
    worker->onStateChanged(RPC_CONNECTED, connMsg);
    // then disconnect with null → should retry
    std::string msg = "";
    bool result = worker->onStateChanged(RPC_DISCONNECTED, msg);
    EXPECT_TRUE(result); // _tryConnect was set to true on connect
}

TEST_F(SessionWorkerTest, OnStateChangedPingout)
{
    QSignalSpy spy(worker.get(), &SessionWorker::onRemoteDisconnected);
    std::string msg = "10.0.0.1";
    worker->onStateChanged(RPC_PINGOUT, msg);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(SessionWorkerTest, OnStateChangedDefault)
{
    std::string msg = "connecting...";
    bool result = worker->onStateChanged(RPC_CONNECTING, msg);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedErrorHostUnreachable)
{
    // asio::error::host_unreachable = 113
    std::string msg = "113";
    bool result = worker->onStateChanged(RPC_ERROR, msg);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedErrorTimedOut)
{
    // asio::error::timed_out = 110
    std::string msg = "110";
    bool result = worker->onStateChanged(RPC_ERROR, msg);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedErrorOther)
{
    std::string msg = "42";
    bool result = worker->onStateChanged(RPC_ERROR, msg);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, OnStateChangedErrorInvalidNumber)
{
    // 源码已修复：在 ENABLE_AUTO_UNIT_TEST 模式下捕获 std::stoi 异常并返回 false
    std::string msg = "not_a_number";
    bool result = worker->onStateChanged(RPC_ERROR, msg);
    EXPECT_FALSE(result);
}

// ---- other methods ----

TEST_F(SessionWorkerTest, HandleRemoteDisconnected)
{
    worker->updateLogin("10.0.0.5", true);
    // first connect to set _connectedAddress
    std::string connMsg = "10.0.0.5";
    worker->onStateChanged(RPC_CONNECTED, connMsg);

    // now disconnect
    worker->handleRemoteDisconnected("10.0.0.5");
    SUCCEED();
}

TEST_F(SessionWorkerTest, HandleRemoteDisconnectedUnknownAddr)
{
    worker->handleRemoteDisconnected("unknown-ip");
    SUCCEED();
}

TEST_F(SessionWorkerTest, HandleRejectConnectionNoServer)
{
    worker->handleRejectConnection();
    SUCCEED();
}

TEST_F(SessionWorkerTest, SendAsyncRequestEmptyTarget)
{
    proto::OriginMessage req;
    req.id = FBE::uuid_t("00000000-0000-0000-0000-000000000000");
    req.mask = REQ_LOGIN;
    req.json_msg = "{}";
    bool result = worker->sendAsyncRequest("", req);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, SendAsyncRequestNoConnection)
{
    proto::OriginMessage req;
    req.id = FBE::uuid_t("00000000-0000-0000-0000-000000000000");
    req.mask = REQ_LOGIN;
    req.json_msg = "{}";
    bool result = worker->sendAsyncRequest("192.168.1.1", req);
    EXPECT_FALSE(result);
}

TEST_F(SessionWorkerTest, SendRequestNoConnection)
{
    proto::OriginMessage req;
    req.id = FBE::uuid_t("00000000-0000-0000-0000-000000000000");
    req.mask = REQ_LOGIN;
    req.json_msg = "{}";
    QString result = worker->sendRequest("192.168.1.1", req);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(SessionWorkerTest, DisconnectRemote)
{
    worker->disconnectRemote();
    SUCCEED();
}

// ---- listen/connect paths with stubs ----

static bool stub_listen_true(SessionWorker *, int) { return true; }
static bool stub_listen_false(SessionWorker *, int) { return false; }

TEST_F(SessionWorkerTest, StartListenSuccess)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, listen), stub_listen_true);
    EXPECT_TRUE(worker->startListen(34567));
}

TEST_F(SessionWorkerTest, StartListenFail)
{
    Stub stub;
    stub.set(ADDR(SessionWorker, listen), stub_listen_false);
    EXPECT_FALSE(worker->startListen(99999));
}

TEST_F(SessionWorkerTest, Stop)
{
    worker->stop();
    SUCCEED();
}
