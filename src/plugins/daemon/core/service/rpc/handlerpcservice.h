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

    void handleRpcLoginResult(bool result, const QString &appName, const QString &ip);
    bool handleRemoteApplyTransFile(co::Json &info);

signals:
    void remoteRequestJob(const QString info);

private:
    QSharedPointer<RemoteServiceBinder> _rpc{ nullptr };
    // port backend
    QMap<QString, QString> _sessionIDs;
};

#endif // HANDLERPCSERVICE_H
