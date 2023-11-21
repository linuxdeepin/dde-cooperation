// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "handleipcservice.h"
#include "sendipcservice.h"
#include "service/rpc/sendrpcservice.h"
#include "service/share/sharecooperationservice.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/comstruct.h"
#include "ipc/backendservice.h"
#include "common/constant.h"
#include "common/commonstruct.h"
#include "service/comshare.h"
#include "service/discoveryjob.h"
#include "utils/config.h"
#include "service/jobmanager.h"
#include "protocol/version.h"

#include <QPointer>

HandleIpcService::HandleIpcService(QObject *parent)
    : QObject(parent)
{
    // init and start backend IPC
    ipcServiceStart();
}

HandleIpcService::~HandleIpcService()
{
}

void HandleIpcService::ipcServiceStart()
{
    createIpcBackend(UNI_IPC_BACKEND_PORT);
    createIpcBackend(UNI_IPC_BACKEND_COOPER_TRAN_PORT);
    createIpcBackend(UNI_IPC_BACKEND_DATA_TRAN_PORT);
}

void HandleIpcService::createIpcBackend(const quint16 port)
{
    if (_backendIpcServices.contains(port)) {
        ELOG << "this port has backend!!!!!! port = " << port;
        return;
    }
    QSharedPointer<BackendService> _backendIpcService(new BackendService);
    _backendIpcServices.insert(port, _backendIpcService);

    QPointer<HandleIpcService> self = this;
    UNIGO([self, _backendIpcService]() {
        while(!self.isNull()) {
            BridgeJsonData bridge;
            _backendIpcService->bridgeChan()->operator>>(bridge); //300ms超时
            if (!_backendIpcService->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

            LOG << "HandleIpcService get bridge json: " << bridge.type << " json:" << bridge.json;
            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                ELOG << "parse error from: " << bridge.json;
                continue;
            }
            self->handleAllMsg(_backendIpcService, bridge.type, json_obj);
        }
    });

    connect(this, &HandleIpcService::connectClosed, this, &HandleIpcService::handleConnectClosed);
    // start ipc services
    ipc::BackendImpl *backendimp = new ipc::BackendImpl();
    backendimp->setInterface(_backendIpcService.data());
    rpc::Server().add_service(backendimp, [this](int type, const fastring &ip, const uint16 port){
        Q_UNUSED(ip);
        if (type == 0)
            emit this->connectClosed(port);
    }).start("0.0.0.0", port, "/backend",
             QString::number(quintptr(_backendIpcService.data())).toStdString().c_str(), "");
}

void HandleIpcService::handleAllMsg(const QSharedPointer<BackendService> backend, const uint type, co::Json &msg)
{
    switch (type) {
    case IPC_PING:
    {
        BridgeJsonData res;
        res.type = IPC_PING;
        res.json = handlePing(msg).toStdString();

        backend->bridgeResult()->operator<<(res);
        break;
    }
    case MISC_MSG:
    {
        MiscJsonCall call;
        call.from_json(msg);
        SendRpcService::instance()->doSendProtoMsg(MISC, call.app.c_str(), call.json.c_str());
        break;
    }
    case BACK_TRY_CONNECT:
    {
        handleTryConnect(msg);
        break;
    }
    case BACK_TRY_TRANS_FILES:
    {
        ipc::TransFilesParam param;
        param.from_json(msg);
        QString session = QString(param.session.c_str());
        QString savedir = QString(param.savedir.c_str());
        QStringList paths;
        for (uint32 i = 0; i < param.paths.size(); i++) {
            paths << param.paths[i].c_str();
        }

        newTransSendJob(session, param.targetSession.c_str(), param.id, paths, param.sub, savedir);
        break;
    }
    case BACK_RESUME_JOB:
    case BACK_CANCEL_JOB:
    {
        bool ok = handleJobActions(type, msg);
        co::Json resjson = {
            { "result", ok },
            { "msg", msg.str() }
        };

        BridgeJsonData res;
        res.type = type;
        res.json = resjson.str();

        backend->bridgeResult()->operator<<(res);
        break;
    }
    case BACK_GET_DISCOVERY:
    {
        handleGetAllNodes(backend);
        break;
    }
    case BACK_GET_PEER:
    {
        break;
    }
    case BACK_FS_CREATE:
    {
        break;
    }
    case BACK_FS_DELETE:
    {
        break;
    }
    case BACK_FS_RENAME:
    {
        break;
    }
    case BACK_FS_PULL:
    {
        break;
    }
    case BACK_DISC_REGISTER:
    {
        handleNodeRegister(false, msg);
        break;
    }
    case BACK_DISC_UNREGISTER:
    {
        handleNodeRegister(true, msg);
        break;
    }
    case BACK_APPLY_TRANS_FILES:
    {
        handleBackApplyTransFiles(msg);
        break;
    }
    case BACK_SHARE_CONNECT:
    {
        // 发送连接请求到被控制端
        handleShareConnect(msg);
        break;
    }
    case BACK_SHARE_CONNECT_REPLY:
    {
        // 回复控制端接受控制还是拒绝控制
        handleShareConnectReply(msg);
        break;
    }
    case BACK_SHARE_START:
    {
        // 客户端发送配置文件到后端
        // 后端启动键鼠共享
        // 发送ip信息到被控制端告诉被控制端启动连接
        handleShareStart(msg);
        break;
    }
    default:
        break;
    }
}

QString HandleIpcService::handlePing(const co::Json &msg)
{
    //check session or gen new one
    ipc::PingBackParam param;
    param.from_json(msg);

    fastring my_ver(BACKEND_PROTO_VERSION);
    if (my_ver.compare(param.version) != 0) {
        DLOG << param.version << " =version not match= " << my_ver;
        return QString();
    }
    QString appName = param.who.c_str();
    // gen new one
    QString sesid = co::randstr(appName.toStdString().c_str(), 8).c_str(); // 长度为8的16进制字符串
    _sessionIDs.insert(appName, sesid);
    // 创建新的sessionid
    SendIpcService::instance()->handleSaveSession(appName, sesid, static_cast<uint16>(param.cb_port));
    return sesid;
}

void HandleIpcService::newTransSendJob(QString session, const QString targetSession, int32 jobId, QStringList paths, bool sub, QString savedir)
{
    auto s = _sessionIDs.key(session);
    if (s.isEmpty()) {
        DLOG << "this session is invalid." << session.toStdString();
        return;
    }

    int32 id = jobId;
    fastring who = s.toStdString();
    fastring savepath = savedir.toStdString();

    co::Json pathjson;
    for (QString path : paths) {
        fastring filepath = path.toStdString();
        pathjson.push_back(filepath);
    }
    FileTransJob job;
    job.app_who = who;
    job.targetAppname = targetSession.toStdString();
    job.job_id = id;
    job.save_path = savepath;
    job.include_hidden = false;
    job.sub = sub;
    job.write = false;
    job.path = pathjson.str();
    job.ip = _ips.value(who.c_str()).toStdString();
    if (job.ip.empty()) {
        ELOG << "create trans file job to remote ip is empty, appName = " << who;
    }

    SendIpcService::instance()->handleAddJob(s, jobId);
    JobManager::instance()->handleRemoteRequestJob(job.as_json().str().c_str());
}

void HandleIpcService::handleNodeRegister(bool unreg, const co::Json &info)
{
    AppPeerInfo appPeer;
    appPeer.from_json(info);
    if (unreg) {
        fastring appname = appPeer.appname;
        // 移除ping
        SendRpcService::instance()->removePing(appname.c_str());
        SendIpcService::instance()->removeSessionByAppName(appname.c_str());
    }
    DiscoveryJob::instance()->updateAnnouncApp(unreg, info.as_string());
}

void HandleIpcService::handleGetAllNodes(const QSharedPointer<BackendService> _backendIpcService)
{
    auto nodes = DiscoveryJob::instance()->getNodes();
    NodeList nodeInfos;
    for (const auto &node : nodes) {
        co::Json nodejs;
        nodejs.parse_from(node);
        NodeInfo info;
        info.from_json(nodejs);
        nodeInfos.peers.push_back(info);
    }
    BridgeJsonData res;
    res.type = BACK_GET_DISCOVERY;
    res.json = nodeInfos.as_json().str();

    _backendIpcService->bridgeResult()->operator<<(res);
}

void HandleIpcService::handleBackApplyTransFiles(co::Json param)
{
     // 远程发送
    ApplyTransFiles info;
    info.from_json(param);
    info.selfIp = Util::getFirstIp();
    info.selfPort = UNI_RPC_PORT_BASE;
    SendRpcService::instance()->doSendProtoMsg(TRANS_APPLY,info.appname.c_str(), info.as_json().str().c_str());
}

void HandleIpcService::handleConnectClosed(const quint16 port)
{
    // 不延时，还是可以ping通，资源还没有回收
    QTimer::singleShot(1000, this, [port]{
        SendIpcService::instance()->handleConnectClosed(port);
    });
}

void HandleIpcService::handleTryConnect(co::Json json)
{
    ipc::ConnectParam param;
    param.from_json(json);
    QString appName(param.appName.c_str());
    QString ip(param.host.c_str());
    _ips.remove(appName);
    _ips.insert(appName, ip);

    QString pass(param.password.c_str());
    QString targetAppname = param.targetAppname.empty() ? appName : param.targetAppname.c_str();
    UserLoginInfo login;

    // 使用base64加密auth
    login.name = param.appName;
    login.auth = Util::encodeBase64(param.password.c_str());

    std::string uuid = Util::genUUID();
    login.my_uid = uuid;
    login.my_name = Util::getHostname();
    login.selfappName = param.appName;
    login.appName = targetAppname.toStdString();

    login.session_id = uuid;
    login.version = UNIAPI_VERSION;
    login.ip = Util::getFirstIp();
    LOG << " rcv client connet to " << ip.toStdString() << appName.toStdString();
    // 创建远程发送的work
    SendRpcService::instance()->createRpcSender(param.appName.c_str(), ip, UNI_RPC_PORT_BASE);
    SendRpcService::instance()->setTargetAppName(appName, targetAppname);
    SendRpcService::instance()->doSendProtoMsg(IN_LOGIN_INFO, appName, login.as_json().str().c_str());

}

bool HandleIpcService::handleJobActions(const uint type, co::Json &msg)
{
    ipc::TransJobParam param;
    param.from_json(msg);
    int jobid = param.job_id;
    uint action_type;
    QString appName(param.appname.c_str());
    if (BACK_RESUME_JOB == type) {
        action_type = TRANS_RESUME;
    } else if (BACK_CANCEL_JOB == type) {
        action_type = TRANS_CANCEL;
    } else {
        action_type = TRANS_PAUSE;
        DLOG << "unsupport job action: PAUSE.";
    }

    FileTransJobAction action;
    action.job_id = (jobid);
    action.appname = (param.appname);
    action.type = (type);

    //LOG << " send job action to " << appName.toStdString() <<  " type: " << action_type;
    SendRpcService::instance()->doSendProtoMsg(action_type, appName, action.as_json().str().c_str(), QByteArray());

    return JobManager::instance()->doJobAction(type, jobid);
}

void HandleIpcService::handleShareStart(co::Json json)
{
    ShareStart st;
    st.from_json(json);

    // 读取相应的配置配置Barrier
    ShareCooperationService::instance()->setBarrierType(BarrierType::Server);
    // 自己启动
    ShareCooperationService::instance()->startBarrier();
    // 通知远端启动客户端连接到这里的batter服务器
    // 发送本机ip过去
    st.ip = Util::getFirstIp();
    SendRpcService::instance()->doSendProtoMsg(SHARE_START, st.appName.c_str(),
                                               st.as_json().str().c_str());
}

void HandleIpcService::handleShareConnect(co::Json json)
{
    ShareConnectApply param;
    param.from_json(json);
    QString appName(param.appName.c_str());
    QString targetIp(param.tarIp.c_str());
    _ips.remove(appName);
    _ips.insert(appName, targetIp);
    QString targetAppname = param.tarAppname.empty() ? appName : param.tarAppname.c_str();

    param.ip = Util::getFirstIp();
    LOG << " rcv client connet to " << targetIp.toStdString() << appName.toStdString();
    // 创建远程发送的work
    SendRpcService::instance()->createRpcSender(appName, targetIp, UNI_RPC_PORT_BASE);
    // 发送给被控制端请求共享连接
    SendRpcService::instance()->doSendProtoMsg(APPLY_SHARE_CONNECT, appName, param.as_json().str().c_str());
}

void HandleIpcService::handleShareConnectReply(co::Json json)
{
    ShareConnectReply reply;
    reply.from_json(json);
    // 回复控制端连接结果
    SendRpcService::instance()->doSendProtoMsg(APPLY_SHARE_CONNECT_RES,
                                               reply.appName.c_str(), json.str().c_str());
}
