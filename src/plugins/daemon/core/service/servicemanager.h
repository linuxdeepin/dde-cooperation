// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <service/transferjob.h>
#include "co/co.h"
#include "co/json.h"

class RemoteServiceBinder;
class TransferJob;
class BackendService;
class Session;
class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    void startRemoteServer();

signals:

public slots:
    void saveSession(QString who, QString session, int cbport);

    void newTransSendJob(QString session, int jobId, QStringList paths, bool sub, QString savedir);
    void notifyConnect(QString ip, QString name, QString password);
    void handleLoginResult(bool result, QString who);

private:
    bool handleRemoteRequestJob(co::Json &info);
    bool handleFSData(co::Json &info, fastring buf);
    bool handleFSInfo(co::Json &info);
    bool handleCancelJob(co::Json &info);
    bool handleTransReport(co::Json &info);

    Session* sessionById(QString &id);

private:
    RemoteServiceBinder *_rpcServiceBinder = nullptr;
    BackendService *_backendIpcService = nullptr;

    QMap<int, TransferJob *> _transjob_sends;
    QMap<int, TransferJob *> _transjob_recvs;

    // record the send jobs which have error happend. that can be resumed by user.
    QMap<int, TransferJob *> _transjob_break;

    // record the frontend session
    co::vector<Session*> _sessions;

    bool _hasConnected = false;
    fastring _connected_target;
    co::mutex g_m;
};

#endif // SERVICEMANAGER_H
