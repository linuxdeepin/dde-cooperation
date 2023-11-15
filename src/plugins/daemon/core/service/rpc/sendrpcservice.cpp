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
#include "common/comonstruct.h"

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
    connect(this, &SendRpcWork::workCreateRpcSender, this, &SendRpcWork::handleCreateRpcSender, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workSetTargetAppName, this, &SendRpcWork::handleSetTargetAppName, Qt::QueuedConnection);
    connect(this, &SendRpcWork::ping, this, &SendRpcWork::handlePing, Qt::QueuedConnection);
    connect(this, &SendRpcWork::workDoSendProtoMsg, this, &SendRpcWork::handleDoSendProtoMsg, Qt::QueuedConnection);
}

void SendRpcWork::handleCreateRpcSender(const QString appName, const QString targetip, quint16 port)
{
    if (_stoped)
        return;
    createRpcSender(appName, targetip, port);
}

void SendRpcWork::handleSetTargetAppName(const QString appName, const QString targetAppName)
{
    if (_stoped)
        return;
    auto sender = this->rpcSender(appName);
    if (sender.isNull())
        return;
    sender->setTargetAppName(targetAppName);
}

void SendRpcWork::handleDoSendProtoMsg(const uint32 type, const QString appName, const QString msg, const QByteArray data)
{
    if (_stoped)
        return;
    auto sender = this->rpcSender(appName);
#if defined(WIN32)
    UNIGO([sender, type, appName, msg, data](){
#endif
    SendResult res;
    if (!sender.isNull()) {
        // 创建exector
        if (type == IN_LOGIN_INFO)
            sender->createExecutor();
        if (type == TRANS_APPLY) {
            co::Json param;
            param.parse_from(msg.toStdString());
            ApplyTransFiles info;
            info.from_json(param);
            if (info.type != APPLY_TRANS_APPLY) {
                info.session = appName.toStdString();

                QString tar = sender->targetAppname();
                info.tarSession = tar.isEmpty() ?
                            appName.toStdString() : tar.toStdString();
            }
            res = sender->doSendProtoMsg(type, info.as_json().str().c_str(), data);
        } else {
            res = sender->doSendProtoMsg(type, msg, data);
        }

    } else {
        SendResult res;
        res.protocolType = type;
        res.errorType = PARAM_ERROR;
        res.data = "There is no remote sender!!!!!!";
    }
    if (_stoped)
        return;
    // todo 发送信号给其他使用模块
    emit sendToRpcResult(appName, res.as_json().str().c_str());
#if defined(WIN32)
    });
#endif
}

void SendRpcWork::handlePing()
{
    if (_stoped)
        return;
#if defined(WIN32)
    UNIGO([this](){
#endif
    for (auto appName = _ping_appname.begin();  appName != _ping_appname.end();) {
        auto sender = this->rpcSender((*appName));
        if (_stoped)
            return;
        SendResult rs = sender->doSendProtoMsg(RPC_PING, "remote_ping", QByteArray());
        if ( !rs.data.empty() || rs.errorType < INVOKE_OK) {
            DLOG << "remote server no reply ping !!!!! " << (*appName).toStdString();
            SendStatus st;
            st.type = rs.errorType;
            st.msg = rs.data;
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
#if defined(WIN32)
    });
#endif
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
    if (appName.isEmpty()) {
        ELOG << "createRpcSenderWork the app name is null!!!!!!!!";
        return;
    }
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
    connect(&work->_work, &SendRpcWork::sendToRpcResult, this, &SendRpcService::sendToRpcResult, Qt::QueuedConnection);
    work->_work.createRpcSender(appName, targetip, port);
    work->_work.moveToThread(&work->_thread);
    work->_thread.start();

}
