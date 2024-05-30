// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "sessionproto.h"
#include "common/exportglobal.h"

#include <QObject>

class AsioService;
class SessionWorker;
class TransferWorker;
class FileSizeCounter;
class EXPORT_API SessionManager : public QObject
{
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    void setSessionExtCallback(ExtenMessageHandler cb);
    void updatePin(QString code);
    void setStorageRoot(const QString &root);
    void updateSaveFolder(const QString &folder);

    void sessionListen(int port);
    bool sessionPing(QString ip, int port);
    bool sessionConnect(QString ip, int port, QString password);
    void sessionDisconnect(QString ip);
    void sendFiles(QString &ip, int port, QStringList paths);
    void recvFiles(QString &ip, int port, QString &token, QStringList names);
    void cancelSyncFile(QString &ip);

    QString sendRpcRequest(const QString &target, int type, const QString &reqJson);

signals:
    void notifyCancelWeb();
    void notifyConnection(int result, QString reason);
    void notifyDoResult(bool result, QString reason);

    // transfer status
    void notifyTransChanged(int status, const QString &path, quint64 size);

public slots:
    void handleTransData(const QString endpoint, const QStringList nameVector);
    void handleTransCount(const QString names, quint64 size);
    void handleCancelTrans(const QString jobid);
    void handleFileCounted(const QString ip, const QStringList paths, quint64 totalSize);

private:
    void createTransWorker();

private:
    // session worker
    std::shared_ptr<SessionWorker> _session_worker { nullptr };

    // new thread to count all sub files size of directory
    std::shared_ptr<FileSizeCounter> _file_counter { nullptr };

    // transfer worker, it will be release after stop.
    std::shared_ptr<TransferWorker> _trans_worker { nullptr };

    bool _send_task { false };
    int _request_job_id;
    QString _save_root = "";
    QString _save_dir = "";
};

#endif // SESSIONMANAGER_H
