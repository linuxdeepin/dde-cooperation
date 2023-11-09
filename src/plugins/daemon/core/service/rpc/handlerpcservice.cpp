// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "handlerpcservice.h"
#include "service/ipc/sendipcservice.h"
#include "sendrpcservice.h"
#include "service/rpc/remoteservice.h"
#include "common/constant.h"
#include "service/comshare.h"
#include "service/discoveryjob.h"
#include "ipc/proto/comstruct.h"
#include "service/jobmanager.h"

#include "utils/cert.h"

#include <QPointer>
#include <QCoreApplication>

co::chan<IncomeData> _income_chan(10, 300);
co::chan<OutData> _outgo_chan(10, 10);
HandleRpcService::HandleRpcService(QObject *parent)
    : QObject(parent)
{
    _rpc.reset(new RemoteServiceBinder);
}

HandleRpcService::~HandleRpcService()
{
}

void HandleRpcService::startRemoteServer()
{
    if (_rpc.isNull())
        return;

    fastring key = Cert::instance()->writeKey();
    fastring crt = Cert::instance()->writeCrt();
    _rpc->startRpcListen(key.c_str(), crt.c_str(),
                         [](const int type, const fastring &ip, const uint16 port){
        if (type == 0) {
            SendStatus st;
            st.type = REMOTE_CLIENT_OFFLINE;
            st.msg = co::Json({{"ip", ip}, {"port", port}}).str();
            co::Json req = st.as_json();
            req.add_member("api", "Frontend.notifySendStatus");
            SendIpcService::instance()->handleSendToAllClient(req.str().c_str());
        }
    });
    Cert::instance()->removeFile(key);
    Cert::instance()->removeFile(crt);

    QPointer<HandleRpcService> self = this;
    UNIGO([self]() {
        // 这里已经是线程或者协程
        while (!self.isNull()) {
            IncomeData indata;
            _income_chan >> indata;
            if (!_income_chan.done()) {
                // timeout, next read
                continue;
            }
            // LOG << "ServiceManager get chan value: " << indata.type << " json:" << indata.json;
            co::Json json_obj = json::parse(indata.json);
            if (json_obj.is_null()) {
                ELOG << "parse error from: " << indata.json;
                continue;
            }
            switch (indata.type) {
            case IN_LOGIN_CONFIRM:
            {
                //TODO: notify user confirm login
                break;
            }
            case IN_LOGIN_RESULT:
            {
                UserLoginResult result;
                result.from_json(json_obj);
                QString appname(result.appname.c_str());
                self->handleRpcLoginResult(result.result, appname, result.ip.c_str());
                break;
            }
            case IN_TRANSJOB:
            {

                JobManager::instance()->handleRemoteRequestJob(json_obj.str().c_str());
                break;
            }
            case FS_DATA:
            {
                // must update the binrary data into struct object.
                JobManager::instance()->handleFSData(json_obj, indata.buf);
                break;
            }
            case FS_INFO:
            {
                JobManager::instance()->handleFSInfo(json_obj);
                break;
            }
            case TRANS_CANCEL:
            {
                JobManager::instance()->handleCancelJob(json_obj);
                break;
            }
            case FS_REPORT:
            {
                JobManager::instance()->handleTransReport(json_obj);
                break;
            }
            case TRANS_APPLY:
            {
                self->handleRemoteApplyTransFile(json_obj);
                break;
            }
            default:
                break;
            }
        }
    });
}

void HandleRpcService::handleRpcLoginResult(bool result, const QString &appName, const QString &ip)
{
    // todo拿到客户端ip，进行消息通讯
    if (result) {
        SendRpcService::instance()->createRpcSender(appName, ip, UNI_RPC_PORT_BASE);
    }

    UNIGO([result, appName]() {
        co::Json req;
        //cbConnect {GenericResult}
        req = {
            { "id", 0 },
            { "result", result ? 1 : 0 },
            { "msg", appName.toStdString() },
        };
        req.add_member("api", "Frontend.cbConnect");
        SendIpcService::instance()->handleSendToClient(appName, req.str().c_str());
    });
}

bool HandleRpcService::handleRemoteApplyTransFile(co::Json &info)
{
    ApplyTransFiles obj;
    obj.from_json(info);
    auto tmp = obj.tarSession;
    obj.tarSession = obj.session;
    obj.session = tmp;
    auto session = obj.session;
    if (obj.type == ApplyTransType::APPLY_TRANS_APPLY) {
        // 创建远程的rpcsender 如果有就直接
        SendRpcService::instance()->createRpcSender(session.c_str(), obj.selfIp.c_str(),
                                                    static_cast<uint16>(obj.selfPort));
        SendRpcService::instance()->setTargetAppName(session.c_str(), obj.tarSession.c_str());
    }
    UNIGO([session, obj]() {
        co::Json infojson;
        co::Json req;

        //notifyFileStatus {FileStatus}
        req = obj.as_json();
        req.add_member("api", "Frontend.applyTransFiles");
        SendIpcService::instance()->handleSendToClient(session.c_str(), req.str().c_str());
    });


    return true;
}

