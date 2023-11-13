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
    initConnect();
}

void SendRpcWork::initConnect()
{
    connect(this, &SendRpcWork::workDoLogin, this, &SendRpcWork::handleDoLogin, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoQuery, this, &SendRpcWork::handleDoQuery, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoMisc, this, &SendRpcWork::handleDoMisc, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoTransfileJob, this, &SendRpcWork::handleDoTransfileJob, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoSendFileInfo, this, &SendRpcWork::handleDoSendFileInfo, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoSendFileBlock, this, &SendRpcWork::handleDoSendFileBlock, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoUpdateTrans, this, &SendRpcWork::handleDoUpdateTrans, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoSendApplyTransFiles, this, &SendRpcWork::handleDoSendApplyTransFiles, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workCreateRpcSender, this, &SendRpcWork::handleCreateRpcSender, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workSetTargetAppName, this, &SendRpcWork::handleSetTargetAppName, Qt::QueuedConnection);
    connect(this, &SendRpcWork::ping, this, &SendRpcWork::handlePing, Qt::QueuedConnection);
}

void SendRpcWork::handleDoLogin(const QString appName, const QString targetIp, const quint16 port, const QString username, const QString pincode)
{
    auto sender = createRpcSender(appName, targetIp, port);
    sender->createExecutor();
    auto result = sender->doLogin(appName, username.toStdString().c_str(), pincode.toStdString().c_str());
    LoginResultStruct ru;
    ru.from_json(result);
    if (ru.result) {
        SendRpcService::instance()->addPing(appName);
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
    _remote.reset(new RemoteServiceSender(appName, targetip, port));
    return _remote;
}

QSharedPointer<RemoteServiceSender> SendRpcWork::rpcSender(const QString &appName)
{
    if (!_remote.isNull())
        return _remote;

    ELOG << "has not remote sender, appname = " << appName.toStdString();
    return nullptr;
}

QString SendRpcWork::targetIp() const
{
    if (_remote)
        return _remote->remoteIP();
    ELOG << "targetIp do not create RemoteServiceSender !!!!!!";
    return "";
}

quint16 SendRpcWork::targetPort() const
{
    if (_remote)
        return _remote->remotePort();
    ELOG << "targetPort do not create RemoteServiceSender !!!!!!";
    return 0;
}

SendRpcService::SendRpcService(QObject *parent)
    : QObject(parent)
{

    initConnet();
}

QSharedPointer<SendRpcService::ThreadInfo> SendRpcService::getWork(const QString &appName)
{
    QReadLocker lk(&_lock);
    return  _worksMap.value(appName);
}

void SendRpcService::initConnet()
{
    connect(this, &SendRpcService::createSenderWork, this, &SendRpcService::createRpcSenderWork, Qt::QueuedConnection);
    connect(&_ping_timer, &QTimer::timeout, this, &SendRpcService::handleTimeOut);
    connect(this, &SendRpcService::startPingTimer, this, &SendRpcService::handleStartTimer, Qt::QueuedConnection);
    connect(this, &SendRpcService::stopPingTimer, this, &SendRpcService::handleStopTimer, Qt::QueuedConnection);
}

SendRpcService::~SendRpcService()
{
}

SendRpcService *SendRpcService::instance()
{
    static SendRpcService service;
    return &service;
}

void SendRpcService::removePing(const QString &appName)
{
    QWriteLocker lk(&_ping_lock);
    _ping_appname.removeOne(appName);
    if (_ping_appname.isEmpty())
        emit stopPingTimer();
}

void SendRpcService::addPing(const QString &appName)
{
    QWriteLocker lk(&_ping_lock);
    if (!_ping_appname.contains(appName))
        _ping_appname.append(appName);
    if (!_ping_timer.isActive())
        emit startPingTimer();
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

void SendRpcService::handleTimeOut()
{
    QReadLocker lk(&_lock);
    QList<QString> ips;
    for (const auto &w : _worksMap.keys()) {
        if (_ping_appname.contains(w) && !ips.contains(w)) {
            ips.append(w);
            emit _worksMap.value(w)->_work.ping();
        }
    }
}

void SendRpcService::createRpcSenderWork(const QString appName, const QString targetip, quint16 port)
{
    QSharedPointer<ThreadInfo> work{nullptr};

    {
        QWriteLocker lk(&_lock);
        QList<QSharedPointer<ThreadInfo>> works;
        for (const auto &w : _works) {
            if (w->_work.targetIp() == targetip && w->_work.targetPort() == port) {
                _worksMap.remove(appName);
                _worksMap.insert(appName, w);
                return;
            }
        }
        _worksMap.remove(appName);
        work.reset(new ThreadInfo);
        _worksMap.insert(appName, work);
    }
    work->_work.createRpcSender(appName, targetip, port);
    work->_work.moveToThread(&work->_thread);
    work->_thread.start();

}
