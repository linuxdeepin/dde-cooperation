// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sendrpcservice.h"
#include "common/constant.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/backend.h"
#include "ipc/bridge.h"
#include "service/comshare.h"
#include "service/ipc/sendipcservice.h"
#include "utils/utils.h"
#include "service/jobmanager.h"

#include <QCoreApplication>

SendRpcWork::~SendRpcWork()
{

}

SendRpcWork::SendRpcWork(QObject *parent) : QObject (parent)
{
}

void SendRpcWork::handleDoLogin(const QString appName, const QString targetIp, const quint16 port, const QString username, const QString pincode)
{
    auto sender = createRpcSender(appName, targetIp, port);
    sender->createExecutor();
    auto result = sender->doLogin(appName, username.toStdString().c_str(), pincode.toStdString().c_str());
    LoginResultStruct ru;
    ru.from_json(result);
    if (ru.result) {
        handleAddPing(appName);
    }
    co::Json req;
    //cbConnect {GenericResult}
    req = {
        { "id", 0 },
        { "result", ru.result ? 1 : 0 },
        { "msg", ru.appName },
    };
    req.add_member("api", "Frontend.cbConnect");
    SendIpcService::instance()->handleSendToClient(ru.appName.c_str(), req.str().c_str());
}

void SendRpcWork::handleDoQuery(const QString appName)
{
    auto sender = this->rpcSender(appName);
    if (sender.isNull())
        return;
    return sender->doQuery(appName);
}

void SendRpcWork::handleDoMisc(const QString appName, const QByteArray msg)
{
    auto sender = this->rpcSender(appName);
    if (sender.isNull())
        return;
    sender->doMisc(appName.toStdString().c_str(), msg);
}

void SendRpcWork::handleDoTransfileJob(const QString appName, int id, const QString obpath, bool hidden, bool recursive, bool recv)
{
    auto sender = this->rpcSender(appName);
    int result = PARAM_ERROR;
    if (!sender.isNull())
        result = sender->doTransfileJob(appName.toStdString().c_str(), id, obpath.toStdString().c_str(), hidden, recursive, recv);
    // 停止当前job
    if (result < INVOKE_OK) {
        ipc::TransJobParam parm;
        parm.session = appName.toStdString();
        parm.is_remote = false;
        parm.job_id = id;
        JobManager::instance()->doJobAction(BACK_CANCEL_JOB, parm.as_json());
    }
    // todo 发送信号给其他使用模块
}

void SendRpcWork::handleDoSendFileInfo(const QString appName, int jobid, int fileid, const QString subdir, const QString filepath)
{
    auto sender = this->rpcSender(appName);
    int result = PARAM_ERROR;
    if (!sender.isNull())
        result = sender->doSendFileInfo(appName.toStdString().c_str(), jobid, fileid,
                                        subdir.toStdString().c_str(), filepath.toStdString().c_str());

    // 停止当前job
    if (result < INVOKE_OK) {
        ipc::TransJobParam parm;
        parm.session = appName.toStdString();
        parm.is_remote = false;
        parm.job_id = jobid;
        JobManager::instance()->doJobAction(BACK_CANCEL_JOB, parm.as_json());
    }
    // todo 发送信号给其他使用模块
}

void SendRpcWork::handleDoSendFileBlock(const QString appName, FileTransBlock fileblock)
{
    auto sender = this->rpcSender(appName);
    int result = PARAM_ERROR;
    if (!sender.isNull())
        result = sender->doSendFileBlock(appName.toStdString().c_str(), fileblock);
    // 停止当前job
    if (result < INVOKE_OK) {
        ipc::TransJobParam parm;
        parm.session = appName.toStdString();
        parm.is_remote = false;
        parm.job_id = fileblock.job_id();
        JobManager::instance()->doJobAction(BACK_CANCEL_JOB, parm.as_json());
    }
    // todo 发送信号给其他使用模块
}

void SendRpcWork::handleDoUpdateTrans(const QString appName, const int id, FileTransUpdate update)
{
    auto sender = this->rpcSender(appName);
    int result = PARAM_ERROR;
    if (!sender.isNull())
        result = sender->doUpdateTrans(appName.toStdString().c_str(), update);
    // 停止当前job
    if (result < INVOKE_OK) {
        ipc::TransJobParam parm;
        parm.session = appName.toStdString();
        parm.is_remote = false;
        parm.job_id = id;
        JobManager::instance()->doJobAction(BACK_CANCEL_JOB, parm.as_json());
    }
    // todo 发送信号给其他使用模块
}

void SendRpcWork::handleDoSendApplyTransFiles(const QString param)
{
    ApplyTransFiles info;
    // 远程处理远端
    co::Json paramj;
    int result = PARAM_ERROR;
    if (paramj.parse_from(param.toStdString())) {
        info.from_json(paramj);
        auto sender = this->rpcSender(info.session.c_str());
        if (!sender.isNull()) {
            info.selfIp = Util::getFirstIp();
            info.selfPort = UNI_RPC_PORT_BASE;
            sender->createExecutor();
            result = sender->doSendApplyTransFiles(info.session.c_str(), info.as_json().str().c_str());
        }
    } else {
        ELOG << "json parse from param error! function = handleDoSendApplyTransFiles , param = " << param.toStdString();
    }

    // todo 发送信号给其他使用模块
    SendStatus st;
    st.type = APPLY_TRANS_FILE;
    st.status = result;
    auto req = st.as_json();
    req.add_member("api", "Frontend.notifySendStatus");
    SendIpcService::instance()->handleSendToClient(info.session.c_str(), req.str().c_str());
}

void SendRpcWork::handleCreateRpcSender(const QString appName, const QString targetip, quint16 port)
{
    createRpcSender(appName, targetip, port);
}

void SendRpcWork::handleSetTargetAppName(const QString appName, const QString targetAppName)
{
    auto sender = this->rpcSender(appName);
    if (sender.isNull())
        return;
    sender->setTargetAppName(targetAppName);
}

void SendRpcWork::handleAddPing(const QString appName)
{
    if (_ping_appname.contains(appName))
        return;
    _ping_appname.append(appName);
    emit startPingTimer();
}

void SendRpcWork::handleRemovePing(const QString appName)
{
    _ping_appname.removeOne(appName);
}

void SendRpcWork::handlePing()
{
    for (auto appName = _ping_appname.begin();  appName != _ping_appname.end();) {
        auto sender = this->rpcSender((*appName));
        QString res = sender->doMisc((*appName).toStdString().c_str(), "remote_ping");
        if ( res.isEmpty()) {
            DLOG << "remote server no reply ping !!!!! " << (*appName).toStdString();
            SendStatus st;
            st.type = REMOTE_CLIENT_OFFLINE;
            st.msg = co::Json({{"ip", sender->remoteIP().toStdString()}, {"port", sender->remotePort()}}).str();
            co::Json req = st.as_json();
            req.add_member("api", "Frontend.notifySendStatus");
            SendIpcService::instance()->handleSendToClient((*appName), req.str().c_str());
            appName = _ping_appname.erase(appName);
        } else {
            appName++;
        }
    }

    if (_ping_appname.isEmpty())
        emit stopPingTimer();
}

QSharedPointer<RemoteServiceSender> SendRpcWork::createRpcSender(const QString &appName, const QString &targetip, uint16_t port)
{
    auto sender = _remotes.value(appName);
    if (sender.isNull()) {
        sender.reset(new RemoteServiceSender(appName, targetip, port));
        _remotes.insert(appName, sender);
        return sender;
    }

    return sender;
}

QSharedPointer<RemoteServiceSender> SendRpcWork::rpcSender(const QString &appName)
{
    if (_remotes.contains(appName))
        return _remotes.value(appName);

    ELOG << "has not remote sender, appname = " << appName.toStdString();
    return nullptr;
}

SendRpcService::SendRpcService(QObject *parent)
    : QObject(parent)
{
    _ping_timer.setInterval(1000);
    connect(&_ping_timer, &QTimer::timeout, this, &SendRpcService::handleTimeOut);
    initConnet();
}

void SendRpcService::initConnet()
{
    if (_work.isNull())
        _work.reset(new SendRpcWork);
    _work->moveToThread(&_thread);
    connect(this, &SendRpcService::workDoLogin, _work.data(), &SendRpcWork::handleDoLogin, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoQuery, _work.data(), &SendRpcWork::handleDoQuery, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoMisc, _work.data(), &SendRpcWork::handleDoMisc, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoTransfileJob, _work.data(), &SendRpcWork::handleDoTransfileJob, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoSendFileInfo, _work.data(), &SendRpcWork::handleDoSendFileInfo, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoSendFileBlock, _work.data(), &SendRpcWork::handleDoSendFileBlock, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoUpdateTrans, _work.data(), &SendRpcWork::handleDoUpdateTrans, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoSendApplyTransFiles, _work.data(), &SendRpcWork::handleDoSendApplyTransFiles, Qt::QueuedConnection);
    connect(this, &SendRpcService::workCreateRpcSender, _work.data(), &SendRpcWork::handleCreateRpcSender, Qt::QueuedConnection);
    connect(this, &SendRpcService::workSetTargetAppName, _work.data(), &SendRpcWork::handleSetTargetAppName, Qt::QueuedConnection);
    connect(this, &SendRpcService::workRemovePing, _work.data(), &SendRpcWork::handleRemovePing, Qt::QueuedConnection);
    connect(this, &SendRpcService::workAddPing, _work.data(), &SendRpcWork::handleAddPing, Qt::QueuedConnection);
    connect(this, &SendRpcService::ping, _work.data(), &SendRpcWork::handlePing, Qt::QueuedConnection);

    connect(_work.data(), &SendRpcWork::startPingTimer, this, &SendRpcService::handleStartTimer, Qt::QueuedConnection);
    connect(_work.data(), &SendRpcWork::stopPingTimer, this, &SendRpcService::handleStopTimer, Qt::QueuedConnection);

    _thread.start();
}

SendRpcService::~SendRpcService()
{
}

SendRpcService *SendRpcService::instance()
{
    static SendRpcService service;
    return &service;
}

void SendRpcService::handleStartTimer()
{
    Q_ASSERT(qApp->thread() == QThread::currentThread());
    if (!_ping_timer.isActive())
        _ping_timer.start();
}

void SendRpcService::handleStopTimer()
{
    Q_ASSERT(qApp->thread() == QThread::currentThread());
    _ping_timer.stop();
}
