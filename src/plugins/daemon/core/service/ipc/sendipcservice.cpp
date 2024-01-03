// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sendipcservice.h"
#include "ipc/backendservice.h"
#include "service/comshare.h"
#include "session.h"
#include "service/discoveryjob.h"
#include "service/share/sharecooperationservicemanager.h"
#include "service/rpc/sendrpcservice.h"
#include "common/constant.h"
#include "ipc/proto/comstruct.h"

#include <QCoreApplication>
#include <QThread>

SendIpcWork::SendIpcWork(QObject *parent) : QObject(parent)
{
}

void SendIpcWork::stop()
{
    _stoped = true;
}

void SendIpcWork::handleStopShareConnect(const QString &info, const QSharedPointer<Session> s)
{
    fastring nodeinfo(info.toStdString());
    co::Json js;
    if (!js.parse_from(nodeinfo)) {
        ELOG << "parse node info to json error, info = " << nodeinfo;
        return;
    }

    NodeInfo _nodeinfo;
    _nodeinfo.from_json(js);
    auto _base = DiscoveryJob::instance()->baseInfo();
    co::Json _base_json;
    if (!_base_json.parse_from(_base)) {
        ELOG << "parse base info to json error, base info = " << _base;
        return;
    }

    NodePeerInfo _info;
    _info.from_json(_base_json);
    if (_info.share_connect_ip.empty() || _info.share_connect_ip != _nodeinfo.os.share_connect_ip ||
            nodeinfo.contains("\"appname\":\"dde-cooperation\""))
        return;

    // 清理连接并停止共享
    ShareCooperationServiceManager::instance()->stop();
    _info.share_connect_ip = "";
    DiscoveryJob::instance()->updateAnnouncBase(_info.as_json().str());
    // 向前段发送断开连接信号
    ShareEvents ev;
    ev.eventType = FRONT_SHARE_DISCONNECT;
    ShareDisConnect disInfo;
    disInfo.appName = "dde-cooperation";
    disInfo.tarAppname = "dde-cooperation";
    ev.data = disInfo.as_json().str();
    co::Json req = ev.as_json(), res;
    req.add_member("api", "Frontend.shareEvents");
    if (s) {
        s->call(req,res);
    } else {
        SendIpcService::instance()->handleSendToClient("dde-cooperation", req.str().c_str());
    }
}

SendIpcWork::~SendIpcWork()
{

}

void SendIpcWork::handleSaveSession(const QString appName, const QString sessionID, const uint16 cbport)
{
    if (_stoped)
        return;
    QSharedPointer<Session> s(new Session(appName, sessionID, static_cast<uint16>(cbport)));
    _sessions.remove(appName);
    _sessions.insert(appName, s);
}

void SendIpcWork::handleConnectClosed(const quint16 port)
{
    if (_stoped)
        return;

    for (auto s = _sessions.begin(); s != _sessions.end();) {
        if (s.value()->port() == port && !s.value()->alive()) {
            s = _sessions.erase(s);
        } else {
            s++;
        }
    }
}

void SendIpcWork::handleRemoveSessionByAppName(const QString appName)
{
    if (_stoped)
        return;
    _sessions.remove(appName);
}

void SendIpcWork::handleRemoveSessionBySessionID(const QString sessionID)
{
    if (_stoped)
        return;
    for (auto s = _sessions.begin(); s != _sessions.end();) {
        if (s.value()->getSession() == sessionID) {
            s = _sessions.erase(s);
        } else {
            s++;
        }
    }
}

void SendIpcWork::handleSendToClient(const QString appName, const QString req)
{
    if (_stoped)
        return;
    auto s = _sessions.value(appName);
    if (s.isNull()) {
        ELOG << "no session to send, appname = " << appName.toStdString() << "  ,  req = " << req.toStdString();
        return;
    }
    co::Json reqj, res;
    reqj.parse_from(req.toStdString());
    s->call(reqj, res);
}

void SendIpcWork::handleAddJob(const QString appName, const int jobID)
{
    if (_stoped)
        return;
    auto s = _sessions.value(appName);
    if (s.isNull()) {
        ELOG << "no session to add job, appname = " << appName.toStdString() << "  ,  jobID = " << jobID;
        return;
    }
    s->addJob(jobID);
}

void SendIpcWork::handleRemoveJob(const QString appName, const int jobID)
{
    if (_stoped)
        return;
    auto s = _sessions.value(appName);
    if (s.isNull()) {
        ELOG << "no session to remove job, appname = " << appName.toStdString() << "  ,  jobID = " << jobID;
        return;
    }
    s->removeJob(jobID);
}

void SendIpcWork::handleSendToAllClient(const QString req)
{
    if (_stoped)
        return;
    for (const auto &appName : _sessions.keys())
        handleSendToClient(appName, req);
}

void SendIpcWork::handleNodeChanged(bool found, QString info)
{
    // notify to all frontend sessions
    for (auto i = _sessions.begin(); i != _sessions.end();) {
        QSharedPointer<Session> s = *i;
        if (s->alive()) {
            // fastring session_id(s->getSession().toStdString());
            fastring nodeinfo(info.toStdString());
            if (!found && s->getName() == "dde-cooperation")
                handleStopShareConnect(info, s);


            co::Json req, res;
            //cbPeerInfo {GenericResult}
            req = {
                { "id", 0 },
                { "result", found ? 1 : 0 },
                { "msg", nodeinfo },
            };

            req.add_member("api", "Frontend.cbPeerInfo");
            s->call(req, res);
            ++i;
        } else {
            SendRpcService::instance()->removePing(s->getName());
            // the frontend is offline
            i = _sessions.erase(i);

            //remove the frontend app register info
            fastring name = s->getName().toStdString();
            DiscoveryJob::instance()->removeAppbyName(name);
        }
    }
}

void SendIpcWork::handlebackendOnline()
{
    QList<uint16> ports{UNI_IPC_FRONTEND_PORT, UNI_IPC_FRONTEND_COOPERATION_PORT,
                UNI_IPC_FRONTEND_TRANSFER_PORT, UNI_IPC_BACKEND_COOPER_PLUGIN_PORT};
    for (const auto &session : _sessions) {
        ports.removeOne(session->port());
    }
    for (const auto &port : ports) {
        Session s("backendServerOnline", "backendServerOnline", port);
        if (s.alive()) {
            co::Json req, res;
            //cbPeerInfo {GenericResult}
            req.add_member("api", "Frontend.backendServerOnline");
            s.call(req, res);
        }
    }
}

void SendIpcWork::handlePing()
{
    for (auto i = _sessions.begin(); i != _sessions.end();) {
        QSharedPointer<Session> s = *i;
        if (!s->alive()) {

            SendRpcService::instance()->removePing(s->getName());
            // the frontend is offline
            i = _sessions.erase(i);

            //remove the frontend app register info
            fastring name = s->getName().toStdString();
            DiscoveryJob::instance()->removeAppbyName(name);
        } else {
            i++;
        }
    }
}

SendIpcService::SendIpcService(QObject *parent)
    : QObject(parent)
{
    work.reset(new SendIpcWork);
    work->moveToThread(&thread);

    initConnect();
    thread.start();
    _ping.setInterval(1000);
    _ping.start();
}

SendIpcService::~SendIpcService()
{
    handleAboutToQuit();
}

SendIpcService *SendIpcService::instance()
{
    static SendIpcService service;
    return &service;
}

void SendIpcService::handleSaveSession(const QString appName, const QString session, const quint16 cbport)
{
    emit saveSession(appName, session, cbport);
}

void SendIpcService::handleConnectClosed(const quint16 port)
{
    emit connectClosed(port);
}

void SendIpcService::handleRemoveSessionByAppName(const QString appName)
{
    emit removeSessionByAppName(appName);
}

void SendIpcService::handleRemoveSessionBySessionID(const QString sessionID)
{
    emit removeSessionBySessionID(sessionID);
}

void SendIpcService::handleSendToClient(const QString appName, const QString req)
{
    emit sendToClient(appName, req);
}

void SendIpcService::handleAboutToQuit()
{
    work->stop();
    thread.quit();
    thread.wait(3000);
}

void SendIpcService::handleAddJob(const QString appName, const int jobId)
{
    emit addJob(appName , jobId);
}

void SendIpcService::initConnect()
{
    connect(qApp, &QCoreApplication::aboutToQuit, this, &SendIpcService::handleAboutToQuit, Qt::DirectConnection);
    connect(&_ping, &QTimer::timeout, this, &SendIpcService::pingFront, Qt::QueuedConnection);

    connect(this, &SendIpcService::connectClosed, work.data(), &SendIpcWork::handleConnectClosed,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::saveSession, work.data(), &SendIpcWork::handleSaveSession,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::removeSessionByAppName, work.data(), &SendIpcWork::handleRemoveSessionByAppName,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::removeSessionBySessionID, work.data(), &SendIpcWork::handleRemoveSessionBySessionID,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::sendToClient, work.data(), &SendIpcWork::handleSendToClient,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::sendToAllClient, work.data(), &SendIpcWork::handleSendToAllClient,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::addJob, work.data(), &SendIpcWork::handleAddJob,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::nodeChanged, work.data(), &SendIpcWork::handleNodeChanged,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::backendOnline, work.data(), &SendIpcWork::handlebackendOnline,
            Qt::QueuedConnection);
    connect(this, &SendIpcService::pingFront, work.data(), &SendIpcWork::handlePing,
            Qt::QueuedConnection);
}
