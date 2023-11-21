// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "handlesendresultservice.h"
#include "common/commonstruct.h"
#include "common/constant.h"
#include "service/ipc/sendipcservice.h"
#include "ipc/proto/chan.h"
#include "service/comshare.h"
#include "utils/config.h"
#include "service/rpc/sendrpcservice.h"

#include "co/co.h"

HandleSendResultService::HandleSendResultService(QObject *parent) : QObject (parent)
{

}

void HandleSendResultService::handleSendResultMsg(const QString appName, const QString msg)
{
    SendResult res;
    co::Json res_json;
    if (!res_json.parse_from(msg.toStdString().c_str())) {
        ELOG << "handleSendResultMsg parse SendResult error!!!!!!!!";
        return;
    }
    res.from_json(res_json);

    if (res.errorType < INVOKE_OK) {
        SendStatus st;
        st.type = res.errorType;
        st.msg = msg.toStdString();
        co::Json req = st.as_json();
        req.add_member("api", "Frontend.notifySendStatus");
        SendIpcService::instance()->handleSendToAllClient(req.str().c_str());
        return;
    }
    if (res.protocolType == IN_LOGIN_INFO) {
        handleLogin(appName, res.data.c_str());
    }
}

void HandleSendResultService::handleLogin(const QString &appName, const QString &msg)
{
    co::Json res_json;
    if (!res_json.parse_from(msg.toStdString())) {
        ELOG << "handleLogin parse SendResult error!!!!!!!!";
        return;
    }
    UserLoginResultInfo res;
    res.from_json(res_json);
    if (res.result) {
        // TODO: save the target peer info into target's map
        fastring token = res.token;
        PeerInfo target_info = res.peer;
        // login successful
        DaemonConfig::instance()->saveAuthed(token);
        SendRpcService::instance()->addPing(res.appName.c_str());
    }

    co::Json req;
    //cbConnect {GenericResult}
    req = {
        { "id", 0 },
        { "result", res.result ? 1 : 0 },
        { "msg", res.appName },
        { "isself", true},
    };
    req.add_member("api", "Frontend.cbConnect");
    SendIpcService::instance()->handleSendToClient(appName, req.str().c_str());
}
