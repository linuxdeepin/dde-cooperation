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
    RemoteServiceBinder *_rpcServiceBinder = nullptr;
    CommonService *_ipcCommonService = nullptr;
    FSService *_ipcFsService = nullptr;

    QMap<int, TransferJob *> _transjob_sends;
    QMap<int, TransferJob *> _transjob_recvs;

    bool _hasConnected = false;
};

#endif // SERVICEMANAGER_H
