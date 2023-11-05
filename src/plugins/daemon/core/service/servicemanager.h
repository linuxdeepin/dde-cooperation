// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <service/job/transferjob.h>
#include <service/job/communicationjob.h>
#include "co/co.h"
#include "co/json.h"

class RemoteServiceBinder;
class TransferJob;
class BackendService;
class DiscoveryJob;
class QSettings;
class Session;
class ServiceManager : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManager(QObject *parent = nullptr);
    ~ServiceManager();

    void startRemoteServer();

signals:
    void connectClosed(const QString &ip, const uint16 port);
    // 使用这个信号必须是不需要等待客户端返回值的。
    void sendToClient(const QString session, const QString &req);

public slots:
    void saveSession(QString who, QString session, int cbport);

    void newTransSendJob(QString session, const QString targetSession, int jobId, QStringList paths, bool sub, QString savedir);
    void notifyConnect(QString session, QString ip, QString password);

    bool doJobAction(uint32_t action, co::Json &jsonobj);
    void sendMiscMessage(fastring &appname, fastring &message);
    void forwardJsonMisc(fastring &appname, fastring &message);

    void handleLoginResult(bool result, QString who);
    void handleFileTransStatus(QString appname, int status, QString fileinfo);
    void handleJobTransStatus(QString appname, int jobid, int status, QString savedir);
    void handleNodeChanged(bool lost, QString info);

    void handleNodeRegister(bool unreg, const co::Json &info);
    void handleGetAllNodes();
    void handleBackApplyTransFiles(const co::Json &param);
    void handleConnectClosed(const QString &ip, const uint16 port);
    // 必须主线程处理向客户端发送
    void handleSendToClient(const QString session, const QString req);

private:
    void localIPCStart();

    bool handleRemoteRequestJob(fastring json);
    bool handleFSData(const co::Json &info, fastring buf);
    bool handleFSInfo(co::Json &info);
    bool handleCancelJob(co::Json &info);
    bool handleTransReport(co::Json &info);

    bool handleRemoteApplyTransFile(co::Json &info);

    QSharedPointer<Session> sessionById(QString &id);
    QSharedPointer<Session> sessionByName(const QString &name);
    fastring genPeerInfo();

    void asyncDiscovery();
private:
    RemoteServiceBinder *_rpcServiceBinder = nullptr;
    BackendService *_backendIpcService = nullptr;
    QMap<QString, QSharedPointer<CommunicationJob>> _applyjobs;

    QMap<int, TransferJob *> _transjob_sends;
    QMap<int, TransferJob *> _transjob_recvs;

    // record the send jobs which have error happend. that can be resumed by user.
    QMap<int, TransferJob *> _transjob_break;

    // record the frontend session
    QList<QSharedPointer<Session>> _sessions;

    bool _hasConnected = false;
    fastring _connected_target;
    co::mutex g_m;

    bool _this_destruct = false;
};

#endif // SERVICEMANAGER_H
