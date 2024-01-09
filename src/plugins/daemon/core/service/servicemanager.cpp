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
#include "service/share/sharecooperationservicemanager.h"
#include "jobmanager.h"

#include "utils/config.h"
#include "utils/cert.h"
#include "common/commonutils.h"

#include <QCoreApplication>


ServiceManager::ServiceManager(QObject *parent) : QObject(parent)
{
#if !defined(WIN32)
    // check network port
    checkNetPort();
#endif
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
    QTimer::singleShot(2000, this, []{
        SendIpcService::instance()->handlebackendOnline();
    });
    _logic.reset(new HandleSendResultService);
    // init sender
    SendIpcService::instance();
    SendRpcService::instance();
    JobManager::instance();
    ShareCooperationServiceManager::instance();
    connect(SendRpcService::instance(), &SendRpcService::sendToRpcResult,
            _logic.data(), &HandleSendResultService::handleSendResultMsg, Qt::QueuedConnection);
    connect(ShareCooperationServiceManager::instance(), &ShareCooperationServiceManager::startServerResult,
            _ipcService, &HandleIpcService::handleShareServerStart, Qt::QueuedConnection);

#if !defined(WIN32)
    _kill_check.setInterval(3000);
    connect(&_kill_check, &QTimer::timeout, this, &ServiceManager::checkSelfKill);
    _kill_check.start();
#endif
}

ServiceManager::~ServiceManager()
{
    if (_ipcService) {
        _ipcService->deleteLater();
        _ipcService = nullptr;
    }

    if (_rpcService) {
        _rpcService->deleteLater();
        _rpcService = nullptr;
    }

    DiscoveryJob::instance()->stopAnnouncer();
    DiscoveryJob::instance()->stopDiscoverer();
}

void ServiceManager::startRemoteServer()
{
    if (_rpcService != nullptr)
        return;
    _rpcService = new HandleRpcService;
    _rpcService->startRemoteServer();
}

void ServiceManager::checkSelfKill()
{
    QFile file(_killScript);
    if (file.exists()) {
        DLOG << "=== other user needs this, stop me! ===";
        QProcess::startDetached(_killScript);
        file.remove();
    }
}

void ServiceManager::localIPCStart()
{
    if (_ipcService != nullptr)
        return;
    _ipcService = new HandleIpcService;
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
        { "share_connect_ip", Util::getFirstIp() },
        { "port", UNI_RPC_PORT_BASE },
        { "os_type", Util::getOSType() },
        { "mode_type", mode },
    };

    return info.str();
}

void ServiceManager::asyncDiscovery()
{
    connect(DiscoveryJob::instance(), &DiscoveryJob::sigNodeChanged, SendIpcService::instance(),
            &SendIpcService::nodeChanged, Qt::QueuedConnection);
    UNIGO([]() {
        DiscoveryJob::instance()->discovererRun();
    });
    UNIGO([this]() {
        fastring baseinfo = genPeerInfo();
        DiscoveryJob::instance()->announcerRun(baseinfo);
    });
}

bool ServiceManager::createKillScript(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QFileDevice::OpenModeFlag::Truncate | QFileDevice::OpenModeFlag::WriteOnly)) {
        ELOG << "open server config error, path = " << filename.toStdString() << ", case : "
             << file.errorString().toStdString();
        return false;
    }

    QTextStream outStream(&file);
    outStream << "#!/bin/sh" << endl << endl;
    outStream << "pidof cooperation-daemon | xargs kill -9;" << endl;
    outStream << "sleep 10;" << endl;
    outStream << "pidof cooperation-daemon | xargs kill -9;" << endl;

    outStream.flush();
    file.flush();
    file.close();
    QProcess::execute("chmod 0777 " + filename);

    return true;
}

void ServiceManager::createBashAndRun()
{
    if (createKillScript(_killScript)) {
        QProcess::startDetached(_killScript);
    }
}

void ServiceManager::checkNetPort()
{
    QFile file(_killScript);

    bool inUse = deepin_cross::CommonUitls::isPortInUse(UNI_RPC_PORT_BASE);
    if (inUse) {
        DLOG << "=== network port is using, restart! ===";
        if (!file.exists()) {
            createKillScript(_killScript);
        }
    } else {
        if (file.exists()) {
            file.remove();
        }
    }
}
