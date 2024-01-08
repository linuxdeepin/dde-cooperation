﻿// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
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

void SendRpcWork::handlePing(const QStringList apps)
{
    if (_stoped)
        return;
#if defined(WIN32)
    UNIGO([this, apps](){
#endif
    for (const auto &appName : apps) {
        if (_stoped)
            return;
        auto sender = this->rpcSender(appName);
        if (sender.isNull())
            continue;
        SendResult rs = sender->doSendProtoMsg(RPC_PING, "remote_ping", QByteArray());
        if ( !rs.data.empty() || rs.errorType < INVOKE_OK) {
            DLOG << "remote server no reply ping !!!!! " << appName.toStdString();
            SendStatus st;
            st.type = rs.errorType;
            st.msg = rs.data;
            co::Json req = st.as_json();
            req.add_member("api", "Frontend.notifySendStatus");
            SendIpcService::instance()->handleSendToClient(appName, req.str().c_str());
            SendRpcService::instance()->removePing(appName);
        }
    }

#if defined(WIN32)
    });
#endif
}

QSharedPointer<RemoteServiceSender> SendRpcWork::createRpcSender(const QString &appName,
                                                                 const QString &targetip, uint16_t port)
{
    if (_remotes.contains(targetip)) {
        _app_ips.remove(appName);
        _app_ips.insert(appName, targetip);
        return _remotes.value(targetip);
    }

    auto ip = _app_ips.value(appName);
    _app_ips.remove(appName);
    _app_ips.insert(appName, targetip);
    QSharedPointer<RemoteServiceSender> remote(new RemoteServiceSender(appName, targetip, port, false));
    _remotes.insert(targetip, remote);

    if (!ip.isEmpty() && _app_ips.keys(ip).isEmpty())
        _remotes.remove(ip);

    return remote;
}

QSharedPointer<RemoteServiceSender> SendRpcWork::rpcSender(const QString &appName)
{
    // 获取ip
    auto ip = _app_ips.value(appName);
    if (ip.isEmpty()) {
        ELOG << "has no ip, appname = " << appName.toStdString();
        return nullptr;
    }
    // 获取 remote
    auto remote = _remotes.value(ip);
    if (remote.isNull()) {
        remote.reset(new RemoteServiceSender(appName, ip, UNI_RPC_PORT_BASE, false));
        _remotes.insert(ip, remote);
    }
    return remote;
}

SendRpcService::SendRpcService(QObject *parent)
    : QObject(parent)
{

    initConnet();
}

void SendRpcService::initConnet()
{
    _ping_timer.setInterval(1000);
    _work.moveToThread(&_thread);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &SendRpcService::handleAboutQuit);

    connect(&_ping_timer, &QTimer::timeout, this, &SendRpcService::handleTimeOut);
    connect(this, &SendRpcService::startPingTimer, this, &SendRpcService::handleStartTimer, Qt::QueuedConnection);
    connect(this, &SendRpcService::stopPingTimer, this, &SendRpcService::handleStopTimer, Qt::QueuedConnection);

    connect(&_work, &SendRpcWork::sendToRpcResult, this, &SendRpcService::sendToRpcResult, Qt::QueuedConnection);
    connect(this, &SendRpcService::workCreateRpcSender, &_work, &SendRpcWork::handleCreateRpcSender, Qt::QueuedConnection);
    connect(this, &SendRpcService::workSetTargetAppName, &_work, &SendRpcWork::handleSetTargetAppName, Qt::QueuedConnection);
    connect(this, &SendRpcService::ping, &_work, &SendRpcWork::handlePing, Qt::QueuedConnection);
    connect(this, &SendRpcService::workDoSendProtoMsg, &_work, &SendRpcWork::handleDoSendProtoMsg, Qt::QueuedConnection);
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
    QReadLocker lk(&_ping_lock);
    emit ping(_ping_appname);
}

void SendRpcService::handleAboutQuit()
{
    _work.stop();
    _thread.quit();
    _thread.wait(3000);
    _thread.exit();
}
