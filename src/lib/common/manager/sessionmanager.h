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
    void notifyTransData(const std::vector<std::string> nameVector);
    void notifyCancelWeb(const std::string jobid);
    void notifyConnection(int result, QString reason);
    void notifyDoResult(bool result, QString reason);

    // transfer status
    void notifyTransChanged(int status, const QString &path, quint64 size);

public slots:
    void handleTransData(const QString endpoint, const QStringList nameVector);
    void handleTransCount(const QString names, quint64 size);
    void handleFileCounted(const QString ip, const QStringList paths, quint64 totalSize);

private:
    void calculateTotalSize(const QStringList &paths, std::function<void(qint64)> onFinish);
    void calculateTotalSizeWithTimeout(const QStringList &paths, std::function<void(qint64)> onFinish);

private:
    std::shared_ptr<AsioService> asio_service { nullptr };

    // session and transfer worker
    std::shared_ptr<SessionWorker> _session_worker { nullptr };
    std::shared_ptr<TransferWorker> _trans_worker { nullptr };

    std::shared_ptr<FileSizeCounter> _file_counter { nullptr };

    bool _send_task { false };
    int _request_job_id;
    QString _save_dir = "";
};

#endif // SESSIONMANAGER_H
