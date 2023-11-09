// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>

#include "co/co.h"
#include "co/json.h"

class RemoteServiceBinder;
class DiscoveryJob;
class HandleIpcService;
class HandleRpcService;
class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    void startRemoteServer();

private:
    void localIPCStart();
    fastring genPeerInfo();
    void asyncDiscovery();
private:
    HandleIpcService *_ipcService { nullptr };
    HandleRpcService *_rpcService { nullptr };
};

#endif // SERVICEMANAGER_H
