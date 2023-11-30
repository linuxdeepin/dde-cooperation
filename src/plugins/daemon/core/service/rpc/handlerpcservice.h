// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HANDLERPCSERVICE_H
#define HANDLERPCSERVICE_H

#include <QObject>
#include <QMap>
#include <QSharedPointer>
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
    void handleRemoteShareConnectReply(co::Json &info);
    void handleRemoteShareStart(co::Json &info);
    void handleRemoteShareStartRes(co::Json &info);
    void handleRemoteShareStop(co::Json &info);

private:
    void startRemoteServer(const quint16 port);

signals:
    void remoteRequestJob(const QString info);

private:
    QSharedPointer<RemoteServiceBinder> _rpc{ nullptr };
    QSharedPointer<RemoteServiceBinder> _rpc_trans{ nullptr };
    // port backend
    QMap<QString, QString> _sessionIDs;
};

#endif // HANDLERPCSERVICE_H
