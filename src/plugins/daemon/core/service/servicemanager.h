// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <service/ipc/commonservice.h>
#include <service/ipc/fsservice.h>
#include <service/transferjob.h>
#include "co/co.h"

class RemoteServiceBinder;
class TransferJob;
class CommonService;
class FSService;
class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    void startRemoteServer();

signals:

public slots:
    void newTransJob(int jobId, QStringList paths, QString savedir);

    void notifyConnect(QString ip, QString name, QString password);

private:
    bool handleRemoteRequestJob(co::Json &info);
    bool handleFSData(co::Json &info, fastring buf);
    bool handleFSInfo(co::Json &info);
    bool handleCancelJob(co::Json &info);
    bool handleTransReport(co::Json &info);

private:
    RemoteServiceBinder *_rpcServiceBinder = nullptr;
    CommonService *_ipcCommonService = nullptr;
    FSService *_ipcFsService = nullptr;

    QMap<int, TransferJob *> _transjob_sends;
    QMap<int, TransferJob *> _transjob_recvs;

    // record the send jobs which have error happend. that can be resumed by user.
    QMap<int, TransferJob *> _transjob_break;

    bool _hasConnected = false;
    fastring _connected_target;
    co::mutex g_m;
};

#endif // SERVICEMANAGER_H
