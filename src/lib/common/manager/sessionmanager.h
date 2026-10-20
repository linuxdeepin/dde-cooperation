// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "sessionproto.h"
#include "common/exportglobal.h"

#include <QObject>
#include <QByteArray>
#include <QHash>

#include <functional>

namespace proto { struct OriginMessage; }

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
    void updateLoginStatus(QString &ip, bool logined);

    void sessionListen(int port);
    bool sessionPing(QString ip, int port);
    int sessionConnect(QString ip, int port, QString password);
    void sessionDisconnect(QString ip);
    // useTunnel makes the file server listen on loopback only and announces a
    // temporary tunnel service instead of a directly reachable endpoint.
    void sendFiles(QString &ip, int port, QStringList paths, bool useTunnel = false);
    // jobid identifies the transfer worker; it defaults to ip and is kept
    // separate when the data connection itself is a loopback tunnel port.
    bool recvFiles(QString &ip, int port, QString &token, QStringList names,
                   const QString &jobid = QString());
    void cancelSyncFile(const QString &ip, const QString &reason = "");

    // Tunnel bridge used by transfer workers. An empty handler means the
    // tunnel is unavailable and transfers fall back to the direct endpoint.
    using TunnelRegisterHandler = std::function<bool(const QString &serviceId, quint16 port)>;
    using TunnelUnregisterHandler = std::function<void(const QString &serviceId)>;
    using TunnelForwardHandler = std::function<quint16(const QString &serviceId,
                                                       const QString &peerIp)>;
    using TunnelStopForwardHandler = std::function<void(const QString &serviceId)>;
    void setTunnelHandlers(TunnelRegisterHandler registerService,
                           TunnelUnregisterHandler unregisterService,
                           TunnelForwardHandler startForward,
                           TunnelStopForwardHandler stopForward);
    void clearTunnelHandlers();

    void sendRpcRequest(const QString &target, int type, const QString &reqJson);

    void sendTunnelMessage(const QString &target, int mask, const QByteArray &payload);

signals:
    void notifyCancelWeb();
    void notifyConnection(int result, QString reason);
    void notifyAsyncRpcResult(int32_t type, const QString response);

    // transfer status
    void notifyTransChanged(int status, const QString &path, quint64 size);

    // raw tunnel frame received from the peer
    void tunnelMessageReceived(int mask, const QByteArray &payload, const QString &remote);

public slots:
    void handleTransData(const QString endpoint, const QStringList nameVector,
                         bool tunnel = false, const QString &service = QString());
    void handleTransCount(const QString names, quint64 size);
    void handleCancelTrans(const QString jobid, const QString reason);
    void handleFileCounted(const QString ip, const QStringList paths, quint64 totalSize);
    void handleRpcResult(int32_t type, const QString &response);
    void handleTransException(const QString jobid, const QString reason);
    void handleTransFinish(const QString jobid);

private:
    std::shared_ptr<TransferWorker> createTransWorker(const QString &jobid);
    void releaseTransWorker(const QString &jobid);
    void releaseTunnelJob(const QString &jobid);
    void scheduleRpcRetry(const QString &target, const proto::OriginMessage &request, int remaining);

    struct TunnelJob {
        QString serviceId;
        bool sender { false };
    };

private:
    // session worker
    std::shared_ptr<SessionWorker> _session_worker { nullptr };

    // new thread to count all sub files size of directory
    std::shared_ptr<FileSizeCounter> _file_counter { nullptr };

    // map to manage multiple transfer workers by jobid (IP address), it will be release after stop.
    std::map<QString, std::shared_ptr<TransferWorker>> _trans_workers;

    QString _save_root = "";
    QString _save_dir = "";

    // jobid -> temporary tunnel service owned by that transfer
    QHash<QString, TunnelJob> _tunnel_jobs;
    TunnelRegisterHandler _tunnel_register;
    TunnelUnregisterHandler _tunnel_unregister;
    TunnelForwardHandler _tunnel_start_forward;
    TunnelStopForwardHandler _tunnel_stop_forward;
};

#endif // SESSIONMANAGER_H
