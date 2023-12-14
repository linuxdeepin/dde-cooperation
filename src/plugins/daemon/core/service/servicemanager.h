// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QSharedPointer>
#include <QTimer>

#include "co/co.h"
#include "co/json.h"

class RemoteServiceSender;
class DiscoveryJob;
class HandleIpcService;
class HandleRpcService;
class HandleSendResultService;
class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    void startRemoteServer();
private Q_SLOTS:
    void checkSelfNetWork();

private:
    void localIPCStart();
    fastring genPeerInfo();
    void asyncDiscovery();
    void createBashAndRun();
private:
    HandleIpcService *_ipcService { nullptr };
    HandleRpcService *_rpcService { nullptr };
    QSharedPointer<HandleSendResultService> _logic;
    bool _network_ok { true };
    int _dis_counts { 0 };
    int _check_count { -1 };
    QTimer _net_check;
};

#endif // SERVICEMANAGER_H
