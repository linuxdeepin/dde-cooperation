// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "servicemanager.h"
#include "discoveryjob.h"
#include "service/ipc/handleipcservice.h"
#include "service/rpc/handlerpcservice.h"
#include "service/ipc/sendipcservice.h"
#include "service/rpc/sendrpcservice.h"
#include "service/rpc/handlesendresultservice.h"
#include "jobmanager.h"

#include "utils/config.h"
#include "utils/cert.h"
#include "common/commonutils.h"

#include <QCoreApplication>
#include <QStandardPaths>

ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
    DLOG << "ServiceManager initializing";
    // init and start backend IPC
    localIPCStart();

    // init the pin code: no setting then refresh as random
    DaemonConfig::instance()->initPin();

    // init the host uuid. no setting then gen a random
    fastring hostid = DaemonConfig::instance()->getUUID();
    if (hostid.empty()) {
        hostid = Util::genUUID();
        DaemonConfig::instance()->setUUID(hostid.c_str());
    }
    asyncDiscovery();
    // QTimer::singleShot(2000, this, []{
    //     SendIpcService::instance()->handlebackendOnline();
    // });
    _logic.reset(new HandleSendResultService);
    // init sender
    SendIpcService::instance();
    SendRpcService::instance();
    JobManager::instance();

    connect(SendRpcService::instance(), &SendRpcService::sendToRpcResult,
            _logic.data(), &HandleSendResultService::handleSendResultMsg, Qt::QueuedConnection);

#ifndef _WIN32
    _userTimer.setInterval(500);
    connect(&_userTimer, &QTimer::timeout, this, [this](){
        QString curUser = QDir::home().dirName();
        auto active = qApp->property(KEY_CURRENT_ACTIVE_USER).toString();
        if (!active.isEmpty() && curUser != active && !curUser.startsWith(active + "@")) {
            WLOG << "User session mismatch"
                                       << "active:" << active.toStdString()
                                       << "current:" << curUser.toStdString();
            _userTimer.stop();
            qApp->exit(0);
        }
    });
    _userTimer.start(2000);
#endif

    connect(qApp, &QCoreApplication::aboutToQuit, this, &ServiceManager::handleAppQuit);
}

ServiceManager::~ServiceManager()
{
}

void ServiceManager::startRemoteServer()
{
    if (_rpcService != nullptr) {
        WLOG << "RPC service already initialized";
        return;
    }
    DLOG << "Starting RPC service";
    _rpcService = new HandleRpcService;
    _rpcService->startRemoteServer();
}


QString ServiceManager::ipcName()
{
    QString key = qAppName() + ".ipc";
    // create ipc socket under user's tmp
    QString userKey = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation), key);
    if (userKey.isEmpty()) {
        userKey = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), key);
    }
    return userKey;
}

void ServiceManager::localIPCStart()
{
    if (_ipcService != nullptr) {
        WLOG << "IPC service already initialized";
        return;
    }
    DLOG << "Starting IPC service";
    _ipcService = new HandleIpcService;
    bool ret = _ipcService->listen(ipcName());
    if (!ret) {
        ELOG << "Failed to start IPC service";
    } else {
        DLOG << "IPC service started successfully";
    }

    connect(SendIpcService::instance(), &SendIpcService::sessionSignal,
            _ipcService, &HandleIpcService::handleSessionSignal, Qt::QueuedConnection);
}

fastring ServiceManager::genPeerInfo()
{
    fastring nick = DaemonConfig::instance()->getNickName();
    int mode = DaemonConfig::instance()->getMode();
    co::Json info = {
        { "proto_version", UNI_RPC_PROTO },
        { "uuid", DaemonConfig::instance()->getUUID() },
        { "nickname", nick },
        { "username", Util::getUsername() },
        { "hostname", Util::getHostname() },
        { "ipv4", Util::getFirstIp() },
        { "share_connect_ip", "" },
        { "port", UNI_RPC_PORT_BASE },
        { "os_type", Util::getOSType() },
        { "mode_type", mode },
    };

    return info.str();
}

void ServiceManager::asyncDiscovery()
{
    connect(DiscoveryJob::instance(), &DiscoveryJob::sigNodeChanged, SendIpcService::instance(),
            &SendIpcService::handleNodeChanged, Qt::QueuedConnection);

    QUNIGO([]() {
        DiscoveryJob::instance()->discovererRun();
    });
    QUNIGO([this]() {
        fastring baseinfo = genPeerInfo();
        DiscoveryJob::instance()->announcerRun(baseinfo);
    });
}

void ServiceManager::handleAppQuit()
{
    DLOG << "ServiceManager shutting down";
    if (_ipcService) {
        DLOG << "Closing IPC service";
        _ipcService->close();
        _ipcService->deleteLater();
        _ipcService = nullptr;
    }

    if (_rpcService) {
        DLOG << "Closing RPC service";
        _rpcService->deleteLater();
        _rpcService = nullptr;
    }

    DLOG << "Stopping discovery services";
    DiscoveryJob::instance()->stopAnnouncer();
    DiscoveryJob::instance()->stopDiscoverer();

    DLOG << "ServiceManager shutdown complete";
    // _exit(0);
}
