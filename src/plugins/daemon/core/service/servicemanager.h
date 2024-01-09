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
    void checkSelfKill();

private:
    void localIPCStart();
    fastring genPeerInfo();
    void asyncDiscovery();
    bool createKillScript(const QString &filename);
    void createBashAndRun();
    void checkNetPort();
private:
    HandleIpcService *_ipcService { nullptr };
    HandleRpcService *_rpcService { nullptr };
    QSharedPointer<HandleSendResultService> _logic;
    QTimer _kill_check;
    const QString _killScript { "/tmp/cooperation-kill.sh" };
};

#endif // SERVICEMANAGER_H
