// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HANDLERPCSERVICE_H
#define HANDLERPCSERVICE_H

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <QTimer>
#include <QReadWriteLock>

#include "co/co.h"
#include "co/json.h"

class RemoteServiceBinder;
class HandleRpcService : public QObject
{
    Q_OBJECT
public:
    explicit HandleRpcService(QObject *parent = nullptr);
    ~HandleRpcService();

    void startRemoteServer();

    void handleRpcLogin(bool result,const QString &targetAppname,
                        const QString &appName, const QString &ip);
    bool handleRemoteApplyTransFile(co::Json &info);
    bool handleRemoteLogin(co::Json &info);
    void handleRemoteDisc(co::Json &info);
    void handleRemoteFileInfo(co::Json &info);
    void handleRemoteFileBlock(co::Json &info, fastring data);
    void handleRemoteReport(co::Json &info);
    void handleRemoteJobCancel(co::Json &info);
    void handleTransJob(co::Json &info);
    void handleRemoteShareConnect(co::Json &info);
    void handleRemoteShareDisConnect(co::Json &info);
    void handleRemoteShareConnectReply(co::Json &info);
    void handleRemoteShareStart(co::Json &info);
    void handleRemoteShareStartRes(co::Json &info);
    void handleRemoteShareStop(co::Json &info);
    void handleRemoteDisConnectCb(co::Json &info);
    void handleRemotePing(const QString &info);

private:
    void startRemoteServer(const quint16 port);

private Q_SLOTS:
    void handleTimeOut();
    void handleStartTimer();

signals:
    void remoteRequestJob(const QString info);
    void startTimer();

private:
    QSharedPointer<RemoteServiceBinder> _rpc{ nullptr };
    QSharedPointer<RemoteServiceBinder> _rpc_trans{ nullptr };
    // port backend
    QMap<QString, QString> _sessionIDs;
    QTimer _timeOut;
    QReadWriteLock _lock;
    QMap<QString, int> _ping_lost_count;
    QMap<QString, bool> _startPing;
};

#endif // HANDLERPCSERVICE_H
