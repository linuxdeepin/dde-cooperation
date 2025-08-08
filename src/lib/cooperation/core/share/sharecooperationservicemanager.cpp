// SPDX-FileCopyrightText: 2023-2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sharecooperationservicemanager.h"
#include "sharecooperationservice.h"
#include "common/log.h"

#include <QFile>
#include <QTimer>
#include <QStandardPaths>

ShareCooperationServiceManager::ShareCooperationServiceManager(QObject *parent)
    : QObject(parent)
{
    DLOG << "ShareCooperationServiceManager constructor";
    _client.reset(new ShareCooperationService);
    _client->setBarrierType(BarrierType::Client);
    _client->setBarrierProfile(barrierProfile());
    _server.reset(new ShareCooperationService);
    _server->setBarrierType(BarrierType::Server);
    _server->setBarrierProfile(barrierProfile());
    connect(this, &ShareCooperationServiceManager::startShareServer, this, &ShareCooperationServiceManager::handleStartShareSever, Qt::QueuedConnection);
    connect(this, &ShareCooperationServiceManager::stopShareServer, this, &ShareCooperationServiceManager::handleStopShareSever, Qt::QueuedConnection);
    DLOG << "ShareCooperationServiceManager initialized";
}

ShareCooperationServiceManager::~ShareCooperationServiceManager()
{
    DLOG << "ShareCooperationServiceManager destructor";
    stop();
}

ShareCooperationServiceManager *ShareCooperationServiceManager::instance()
{
    DLOG << "Getting ShareCooperationServiceManager instance";
    static ShareCooperationServiceManager in;
    return &in;
}

QSharedPointer<ShareCooperationService> ShareCooperationServiceManager::client()
{
    DLOG << "Getting client service";
    return _client;
}

QSharedPointer<ShareCooperationService> ShareCooperationServiceManager::server()
{
    DLOG << "Getting server service";
    return _server;
}

void ShareCooperationServiceManager::stop()
{
    _client->stopBarrier();
    _client->setClientTargetIp(""); // reset client by serverIp
    emit stopShareServer();
    DLOG << "Stop signal emitted";
}

bool ShareCooperationServiceManager::startServer(const QString &msg)
{
    DLOG << "Starting share server with message:" << msg.toStdString();
    emit startShareServer(msg);
    return true;
}

bool ShareCooperationServiceManager::stopServer()
{
    DLOG << "Stopping share server";
    emit stopShareServer();
    return true;
}

void ShareCooperationServiceManager::handleStartShareSever(const QString msg)
{
    if (_server.isNull()) {
        WLOG << "Cannot start share server - server is null";
        return;
    }

    DLOG << "Handling share server start with message:" << msg.toStdString();
    bool ok = _server->restartBarrier();
    emit startServerResult(ok, msg);
}

void ShareCooperationServiceManager::handleStopShareSever()
{
    if (_server.isNull()) {
        WLOG << "Cannot stop share server - server is null";
        return;
    }
    
    DLOG << "Stopping share server";
    _server->stopBarrier();
}

QString ShareCooperationServiceManager::barrierProfile()
{
    QString profileDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    DLOG << "Barrier profile directory:" << profileDir.toStdString();
    return profileDir;
}
