// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "handlerpcservice.h"
#include "service/ipc/sendipcservice.h"
#include "sendrpcservice.h"
#include "service/rpc/remoteservice.h"
#include "service/share/sharecooperationservice.h"
#include "common/constant.h"
#include "common/commonstruct.h"
#include "service/comshare.h"
#include "service/discoveryjob.h"
#include "ipc/proto/comstruct.h"
#include "ipc/bridge.h"
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
}

bool HandleRpcService::handleRemoteApplyTransFile(co::Json &info)
{
    ApplyTransFiles obj;
    obj.from_json(info);

    auto tmp = obj.tarAppname;
    obj.tarAppname = obj.appname;
    obj.appname = tmp;
    auto session = obj.appname;

    co::Json infojson;
    co::Json req;

    //notifyFileStatus {FileStatus}
    req = obj.as_json();
    req.add_member("api", "Frontend.applyTransFiles");
    SendIpcService::instance()->handleSendToClient(session.c_str(), req.str().c_str());
    if (obj.type != APPLY_TRANS_APPLY)
        SendRpcService::instance()->removePing(session.c_str());

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
    DLOG << "handleRemoteDisc: " << info.dbg();
    MiscJsonCall mis;
    mis.from_json(info);
    co::Json msg;
    msg.add_member("msg", mis.json.c_str());
    msg.add_member("api", "Frontend.cbMiscMessage");
    SendIpcService::instance()->handleSendToClient(mis.app.c_str(), msg.str().c_str());
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

    FileInfo fsinfo;
    fsinfo.job_id = res.job_id;
    fsinfo.file_id = fileid;
    fsinfo.name = filename;
    fsinfo.total_size = entry.size;
    fsinfo.current_size = 0;
    fsinfo.time_spended = -1;
    co::Json tm = fsinfo.as_json();
    if (type == FILE_B) {
        JobManager::instance()->handleFSInfo(tm, false);
    } else if (type == DIR) {
        JobManager::instance()->handleFSInfo(tm, true);
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

void HandleRpcService::handleRemoteShareConnect(co::Json &info)
{
    ShareConnectApply lo;
    lo.from_json(info);

    // 这是收到的appname是对方的appname
    SendRpcService::instance()->createRpcSender(lo.tarAppname.c_str(), lo.ip.c_str(),
                                                UNI_RPC_PORT_BASE);
    SendRpcService::instance()->setTargetAppName(lo.tarAppname.c_str(),
                                                 lo.appName.c_str());
    ShareEvents event;
    event.eventType = FRONT_SHARE_APPLY_CONNECT;
    event.data = info.str();
    co::Json req = event.as_json();
    req.add_member("api", "Frontend.shareEvents");
    SendIpcService::instance()->handleSendToClient(lo.tarAppname.c_str(), req.str().c_str());
}

void HandleRpcService::handleRemoteShareDisConnect(co::Json &info)
{
    // 发送给前端
    ShareDisConnect sd;
    sd.from_json(info);

    ShareEvents ev;
    ev.eventType = FRONT_SHARE_DISCONNECT;
    ev.data = info.str();
    co::Json req = ev.as_json();
    req.add_member("api", "Frontend.shareEvents");
    SendIpcService::instance()->handleSendToClient(sd.tarAppname.c_str(), req.str().c_str());
}

void HandleRpcService::handleRemoteShareConnectReply(co::Json &info)
{
    ShareConnectReply reply;
    reply.from_json(info);

    ShareEvents event;
    event.eventType = FRONT_SHARE_APPLY_CONNECT_REPLY;
    event.data = info.str();
    co::Json req = event.as_json();
    req.add_member("api", "Frontend.shareEvents");
    SendIpcService::instance()->handleSendToClient(reply.tarAppname.c_str(), req.str().c_str());
}

void HandleRpcService::handleRemoteShareStart(co::Json &info)
{
    ShareStart st;
    st.from_json(info);
    ShareEvents evs;
    evs.eventType = SHARE_START_RES;
    ShareEvents ev;
    ev.eventType = FRONT_SHARE_START_REPLY;

    ShareStartReply reply;
    reply.result = true;
    reply.isRemote = false;

    ShareStartRmoteReply rreply;
    rreply.result = true;
    rreply.tarAppname = st.appName;
    rreply.appName = st.tarAppname;

    // 获取其中的ip进行Barrier的client配置
    ShareCooperationService::instance()->setBarrierType(BarrierType::Client);
    if (!ShareCooperationService::instance()->setClientTargetIp(st.config.client_screen.c_str(), st.ip.c_str(), st.port)
            || !ShareCooperationService::instance()->restartBarrier()) {
        reply.result = false;
        reply.errorMsg = "init client config error or start error! param = " + info.str();
        rreply.result = false;
        rreply.errorMsg = "init client config error or start error! param = " + info.str();
    }
    evs.data = rreply.as_json().str();
    // 通知远程
    SendRpcService::instance()->doSendProtoMsg(SHARE_START_RES, st.tarAppname.c_str(),
                                               evs.as_json().str().c_str());
    ev.data = reply.as_json().str();
    auto req = ev.as_json();
    // 通知前端
    req.add_member("api", "Frontend.shareEvents");
    SendIpcService::instance()->handleSendToClient(st.tarAppname.c_str(), req.str().c_str());
}

void HandleRpcService::handleRemoteShareStartRes(co::Json &info)
{
    ShareStartRmoteReply rreply;
    rreply.from_json(info);
    ShareStartReply reply;
    reply.result = rreply.result;
    reply.isRemote = true;
    reply.errorMsg = rreply.errorMsg;
    ShareEvents evs;
    evs.eventType = SHARE_START_RES;
    // 通知前端
    auto req = evs.as_json();
    req.add_member("api", "Frontend.shareEvents");
    SendIpcService::instance()->handleSendToClient(rreply.tarAppname.c_str(), req.str().c_str());
}

void HandleRpcService::handleRemoteShareStop(co::Json &info)
{
    ShareStop st;
    st.from_json(info);
    // 停止自己的共享，并告诉前端
    ShareCooperationService::instance()->stopBarrier();
    ShareEvents event;
    event.eventType = FRONT_SHARE_STOP;
    event.data = info.str();
    co::Json req = event.as_json();
    req.add_member("api", "Frontend.shareEvents");
    SendIpcService::instance()->handleSendToClient(st.tarAppname.c_str(), req.str().c_str());
}

void HandleRpcService::handleRemoteDisConnectCb(co::Json &info)
{
    // 发送给前端
    ShareDisConnect sd;
    sd.from_json(info);

    co::Json req = info;
    req.add_member("api", "Frontend.cbDisConnect");
    SendIpcService::instance()->handleSendToClient(sd.tarAppname.c_str(), req.str().c_str());
    SendRpcService::instance()->removePing(sd.tarAppname.c_str());
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
            st.type = 0;
            st.status = REMOTE_CLIENT_OFFLINE;
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
            LOG << "ServiceManager get chan value: " << indata.type << " json:" << indata.json;
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
            case APPLY_SHARE_CONNECT:
            {
                // 被控制方收到共享连接申请
                OutData data;
                _outgo_chan << data;
                self->handleRemoteShareConnect(json_obj);
                break;
            }
            case APPLY_SHARE_DISCONNECT: {
                // 被控制方收到共享连接申请
                OutData data;
                _outgo_chan << data;
                self->handleRemoteShareDisConnect(json_obj);
                break;
            }
            case APPLY_SHARE_CONNECT_RES:
            {
                // 控制方收到被控制方申请共享连接的回复
                OutData data;
                _outgo_chan << data;
                self->handleRemoteShareConnectReply(json_obj);
                break;
            }
            case SHARE_START:
            {
                // 被控制方收到控制方的开始共享
                OutData data;
                _outgo_chan << data;
                self->handleRemoteShareStart(json_obj);
                break;
            }
            case SHARE_START_RES:
            {
                // 被控制方收到控制方的开始共享
                OutData data;
                _outgo_chan << data;
                self->handleRemoteShareStart(json_obj);
                break;
            }
            case SHARE_STOP:
            {
                // 被控制方收到控制方的开始共享
                OutData data;
                _outgo_chan << data;
                self->handleRemoteShareStop(json_obj);
                break;
            }
            case DISCONNECT_CB:
            {
                // 被控制方收到控制方的开始共享
                OutData data;
                _outgo_chan << data;
                self->handleRemoteDisConnectCb(json_obj);
                break;
            }
            default:{
                OutData data;
                _outgo_chan << data;
                break;
            }
            }
        }
    });
}

