// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

//#include "sessionworker.h"
//#include "transferworker.h"
#include "sessionproto.h"

#include <QObject>

class AsioService;
class SessionWorker;
class TransferWorker;
class SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    void setSessionExtCallback(ExtenMessageHandler cb);
    void updatePin(QString code);

    void sessionListen(int port);
    bool sessionPing(QString ip, int port);
    bool sessionConnect(QString ip, int port, QString password);
    void sessionDisconnect(QString ip);
    void sendFiles(QString &ip, int port, QStringList paths);
    void recvFiles(QString &ip, int port, QString &token, QStringList names);
    void cancelSyncFile(QString &ip);

    QString sendRpcRequest(const QString &target, int type, const QString &reqJson);

signals:
    void notifyTransData(const std::vector<std::string> nameVector);
    void notifyCancelWeb(const std::string jobid);
    void notifyConnection(int result, QString reason);
    void notifyDoResult(bool result, QString reason);

public slots:
    void handleTransData(const QString endpoint, const QStringList nameVector);

private:
    std::shared_ptr<AsioService> asio_service { nullptr };

    // session and transfer worker
    std::shared_ptr<SessionWorker> _session_worker { nullptr };
    std::shared_ptr<TransferWorker> _trans_worker { nullptr };

    bool _send_task { false };
    int _request_job_id;
    QString _save_dir = "";
};

#endif // SESSIONMANAGER_H
