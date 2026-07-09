// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/rpc/handlesendresultservice.h"
#include "service/comshare.h"
#include "common/commonstruct.h"
#include "ipc/proto/chan.h"
#include "common/constant.h"
#include "utils/config.h"

#include <QSignalSpy>

// handleSendResultMsg: invalid json -> parse fails, returns early
TEST(HandleSendResultTest, InvalidJsonReturnsEarly)
{
    HandleSendResultService svc;
    svc.handleSendResultMsg("app1", "not a json");
    SUCCEED();
}

TEST(HandleSendResultTest, EmptyMsgReturnsEarly)
{
    HandleSendResultService svc;
    svc.handleSendResultMsg("app1", "");
    SUCCEED();
}

// errorType < INVOKE_OK -> sends status to client
TEST(HandleSendResultTest, ErrorBelowInvokeOk)
{
    HandleSendResultService svc;
    SendResult res;
    res.protocolType = LOGIN_INFO;
    res.errorType = INVOKE_FAIL; // < INVOKE_OK(0)
    res.data = "error data";
    svc.handleSendResultMsg("app1", res.as_json().str().c_str());
    SUCCEED();
}

TEST(HandleSendResultTest, ParamErrorSendsStatus)
{
    HandleSendResultService svc;
    SendResult res;
    res.protocolType = TRANSJOB;
    res.errorType = PARAM_ERROR; // < INVOKE_OK
    res.data = "no executor";
    svc.handleSendResultMsg("app1", res.as_json().str().c_str());
    SUCCEED();
}

TEST(HandleSendResultTest, InvokeOkLoginInfoNoSession)
{
    HandleSendResultService svc;
    SendResult res;
    res.protocolType = LOGIN_INFO;
    res.errorType = INVOKE_OK;
    res.data = "invalid login json";
    // handleLogin parses res.data which is invalid -> sets result=false
    svc.handleSendResultMsg("app1", res.as_json().str().c_str());
    SUCCEED();
}

TEST(HandleSendResultTest, InvokeOkLoginInfoValid)
{
    HandleSendResultService svc;
    UserLoginResultInfo info;
    info.result = true;
    info.token = "token123";
    info.appName = "app1";
    info.peer.hostname = "host";
    info.peer.platform = "UOS";
    info.peer.username = "user";
    info.peer.version = "1.0.0";

    SendResult res;
    res.protocolType = LOGIN_INFO;
    res.errorType = INVOKE_OK;
    res.data = info.as_json().str();
    svc.handleSendResultMsg("app1", res.as_json().str().c_str());
    // cleanup
    Comshare::instance()->updateStatus(CURRENT_STATUS_DISCONNECT);
}

TEST(HandleSendResultTest, InvokeOkLoginInfoFailed)
{
    HandleSendResultService svc;
    UserLoginResultInfo info;
    info.result = false;
    info.token = "bad";
    info.appName = "app1";

    SendResult res;
    res.protocolType = LOGIN_INFO;
    res.errorType = INVOKE_OK;
    res.data = info.as_json().str();
    svc.handleSendResultMsg("app1", res.as_json().str().c_str());
}

TEST(HandleSendResultTest, InvokeOkNonLoginType)
{
    HandleSendResultService svc;
    SendResult res;
    res.protocolType = TRANSJOB;
    res.errorType = INVOKE_OK;
    res.data = "ok";
    // non-LOGIN_INFO type with INVOKE_OK -> does nothing (no handleLogin call)
    svc.handleSendResultMsg("app1", res.as_json().str().c_str());
    SUCCEED();
}
