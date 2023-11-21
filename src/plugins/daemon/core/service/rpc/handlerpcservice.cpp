// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "handlerpcservice.h"
#include "service/ipc/sendipcservice.h"
#include "sendrpcservice.h"
#include "service/rpc/remoteservice.h"
#include "common/constant.h"
#include "common/comonstruct.h"
#include "service/comshare.h"
#include "service/discoveryjob.h"
#include "ipc/proto/comstruct.h"
#include "service/jobmanager.h"
#include "protocol/version.h"
#include "utils/config.h"
#include "service/fsadapter.h"

#include "utils/cert.h"

#include <QPointer>
#include <QCoreApplication>

co::chan<IncomeData> _income_chan(10, 300);
co::chan<OutData> _outgo_chan(10, 10);
HandleRpcService::HandleRpcService(QObject *parent)
    : QObject(parent)
{
    _rpc.reset(new RemoteServiceBinder);
    _rpc_trans.reset(new RemoteServiceBinder);
}

HandleRpcService::~HandleRpcService()
{
}

void HandleRpcService::startRemoteServer()
{
    startRemoteServer(UNI_RPC_PORT_BASE);
    startRemoteServer(UNI_RPC_PORT_TRANS);
}

void HandleRpcService::handleRpcLogin(bool result, const QString &targetAppname,
                                      const QString &appName, const QString &ip)
{
    // todo拿到客户端ip，进行消息通讯
    if (result) {
        SendRpcService::instance()->createRpcSender(appName, ip, UNI_RPC_PORT_BASE);
        SendRpcService::instance()->setTargetAppName(appName, targetAppname);
    }

    UNIGO([result, appName]() {
        co::Json req;
        //cbConnect {GenericResult}
        req = {
            { "id", 0 },
            { "result", result ? 1 : 0 },
            { "msg", appName.toStdString() },
            { "isself", false},
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

bool HandleRpcService::handleRemoteLogin(co::Json &info)
{
    UserLoginInfo lo;
    lo.from_json(info);
    UserLoginResultInfo lores;

    std::string version = lo.version.c_str();
    if (version.compare(UNIAPI_VERSION) != 0) {
        // Notification not match version
        lores.result = false;
        lores.token = "Invalid version";
    } else {
        bool authOK = false;

        fastring pwd = lo.auth;
        if (pwd.empty()) {
            // TODO: 无认证，用户确认
            if (DaemonConfig::instance()->needConfirm()) {
                // 目前还没有处理
            } else {
                authOK = true;
            }
        } else {
            fastring pass = Util::decodeBase64(pwd.c_str());
            //            LOG << "pass= " << pass << " getPin=" << DaemonConfig::instance()->getPin();
            authOK = DaemonConfig::instance()->getPin().compare(pass) == 0;
        }

        if (!authOK) {
            lores.result = false;
            lores.token = "Invalid auth code";
        } else {
            DaemonConfig::instance()->saveRemoteSession(lo.session_id);

            //TODO: generate auth token
            fastring auth_token = "thatsgood";
            DaemonConfig::instance()->setTargetName(lo.my_name.c_str());   // save the login name
            fastring plattsr;
            if (WINDOWS == Util::getOSType()) {
                plattsr = "Windows";
            } else {
                //TODO: other OS
                plattsr = "UOS";
            }

            lores.peer.version = version;
            lores.peer.hostname = Util::getHostname();
            lores.peer.platform = plattsr.c_str();
            lores.peer.username = Util::getUsername();
            lores.token = auth_token;
            lores.appName = lo.selfappName;
            lores.result = authOK;
        }

        UserLoginResult result;
        result.appname = lo.appName;
        result.uuid = lo.my_uid;
        result.ip = lo.ip;
        result.result = authOK;
        QString appname(result.appname.c_str());
        handleRpcLogin(result.result, lo.selfappName.c_str(), appname, result.ip.c_str());
    }
    OutData data;
    data.type = OUT_LOGIN;
    data.json = lores.as_json().str();
    _outgo_chan << data;

    return true;
}

void HandleRpcService::handleRemoteDisc(co::Json &info)
{
    MiscInfo mis;
    mis.from_json(info);
    co::Json msg{"msg", mis.json};
    msg.add_member("api", "Frontend::cbMiscMessage");
    SendIpcService::instance()->sendToClient(mis.appName.c_str(), msg.str().c_str());
}

void HandleRpcService::handleRemoteFileInfo(co::Json &info)
{
    FileTransCreate res;
    res.from_json(info);
    int32 fileid = res.file_id;

    FileEntry entry = res.entry;

    FileType type = static_cast<FileType>(entry.filetype);
    fastring filename;
    if (res.sub_dir.empty()) {
        filename = entry.name;
    } else {
        filename = res.sub_dir + "/" + entry.name;
    }

    if (type == FILE_B) {
        FileInfo info;
        info.job_id = res.job_id;
        info.file_id = fileid;
        info.name = filename;
        info.total_size = entry.size;
        info.current_size = 0;
        info.time_spended = -1;
        co::Json tm = info.as_json();
        JobManager::instance()->handleFSInfo(tm);
    }

    bool exist = JobManager::instance()->handleCreateFile(res.job_id, filename.c_str(), type == DIR);

    FileTransResponse reply;
    OutData data;

    reply.id = fileid;
    reply.name = (filename.c_str());
    reply.result = (exist ? OK : IO_ERROR);
    data.json = reply.as_json().str();
    _outgo_chan << data;

}

void HandleRpcService::handleRemoteFileBlock(co::Json &info, fastring data)
{
    FileTransResponse reply;
    auto res = JobManager::instance()->handleFSData(info, data, &reply);

    OutData out;
    reply.result = (res ? OK : IO_ERROR);
    out.json = reply.as_json().str();
    _outgo_chan << out;
}

void HandleRpcService::handleRemoteReport(co::Json &info)
{
    FileTransResponse reply;
    reply.result = OK;
    OutData out;

    JobManager::instance()->handleTransReport(info, &reply);
    out.json = reply.as_json().str();
    _outgo_chan << out;
}

void HandleRpcService::handleRemoteJobCancel(co::Json &info)
{
    FileTransResponse reply;
    reply.result = OK;
    OutData out;

    JobManager::instance()->handleCancelJob(info, &reply);
    out.json = reply.as_json().str();
    _outgo_chan << out;
}

void HandleRpcService::handleTransJob(co::Json &info)
{
    auto res = JobManager::instance()->handleRemoteRequestJob(info.str().c_str());
    OutData data;
    data.type = OUT_TRANSJOB;
    data.json = co::Json({"result", res}).str();
    _outgo_chan << data;
}

void HandleRpcService::startRemoteServer(const quint16 port)
{
    if (_rpc.isNull() && port != UNI_RPC_PORT_TRANS)
        return;
    if (_rpc_trans.isNull() && port == UNI_RPC_PORT_TRANS)
        return;
    auto rpc = port != UNI_RPC_PORT_TRANS ? _rpc : _rpc_trans;
    fastring key = Cert::instance()->writeKey();
    fastring crt = Cert::instance()->writeCrt();
    auto callback = [](const int type, const fastring &ip, const uint16 port){
        if (type == 0) {
            SendStatus st;
            st.type = REMOTE_CLIENT_OFFLINE;
            st.msg = co::Json({{"ip", ip}, {"port", port}}).str();
            co::Json req = st.as_json();
            req.add_member("api", "Frontend.notifySendStatus");
            SendIpcService::instance()->handleSendToAllClient(req.str().c_str());
        }
    };
    if (port == UNI_RPC_PORT_TRANS) {
        rpc->startRpcListen(key.c_str(), crt.c_str(), port, callback);
    } else {
        rpc->startRpcListen(key.c_str(), crt.c_str(), port);
    }
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
            case IN_LOGIN_INFO:
            {
                self->handleRemoteLogin(json_obj);
                break;
            }
            case IN_LOGIN_CONFIRM:
            {
                //TODO: notify user confirm login
                break;
            }
            case IN_LOGIN_RESULT:// 服务器端回复登陆结果
            {
                break;
            }
            case IN_TRANSJOB:
            {
                self->handleTransJob(json_obj);
                break;
            }
            case FS_DATA:
            {
                // must update the binrary data into struct object.
                self->handleRemoteFileBlock(json_obj, indata.buf);
                break;
            }
            case FS_INFO:
            {
                self->handleRemoteFileInfo(json_obj);

                break;
            }
            case TRANS_CANCEL:
            {
                self->handleRemoteJobCancel(json_obj);
                break;
            }
            case FS_REPORT:
            {
                self->handleRemoteReport(json_obj);
                break;
            }
            case TRANS_APPLY:
            {
                OutData data;
                _outgo_chan << data;
                self->handleRemoteApplyTransFile(json_obj);
                break;
            }
            case MISC:
            {
                OutData data;
                _outgo_chan << data;
                self->handleRemoteDisc(json_obj);
                break;
            }
            case RPC_PING:
            {
                OutData data;
                data.json = "remote_ping";
                _outgo_chan << data;
                break;
            }
            default:
                break;
            }
        }
    });
}

