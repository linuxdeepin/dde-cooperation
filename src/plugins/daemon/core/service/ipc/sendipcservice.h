// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENDIPCSERVICE_H
#define SENDIPCSERVICE_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QSharedPointer>
#include <QThread>

#include "co/json.h"
#include "ipc/proto/chan.h"

class BackendService;
class Session;

class SendIpcWork :public QObject {
    Q_OBJECT
    friend class SendIpcService;

public:
    ~SendIpcWork();
private slots:
    void handleSaveSession(const QString appName, const QString sessionID, const quint16 cbport);
    void handleConnectClosed(const quint16 port);
    void handleRemoveSessionByAppName(const QString appName);
    void handleRemoveSessionBySessionID(const QString sessionID);
    void handleSendToClient(const QString appName, const QString req);
    void handleAddJob(const QString appName, const int jobID);
    void handleRemoveJob(const QString appName, const int jobID);
    void handleSendToAllClient(const QString req);
    void handleNodeChanged(bool found, QString info);
    void handlebackendOnline();
    void handlePing();

private:
    explicit SendIpcWork(QObject *parent = nullptr);
    void stop();
    void handleStopShareConnect(const QString &info, const QSharedPointer<Session> s);

private:
    // record the frontend session
    QMap<QString, QSharedPointer<Session>> _sessions;
    std::atomic_bool _stoped{ false };
};

class SendIpcService : public QObject
{
    Q_OBJECT
public:
    ~SendIpcService();
    static SendIpcService *instance();

signals:
    void connectClosed(const quint16 port);
    // 使用这个信号必须是不需要等待客户端返回值的。
    void sendToClient(const QString appName, const QString req);
    void removeSessionByAppName(const QString appName);
    void removeSessionBySessionID(const QString sessionID);
    void saveSession(const QString appName, const QString sessionID, const quint16 cbport);
    void addJob(const QString appName, const int jobId);
    void removeJob(const QString appName, const int jobId);
    void sendToAllClient(const QString req);
    void nodeChanged(bool found, QString info);
    void backendOnline();
    void pingFront();

    void startOfflineTimer();
    void stopOfflineTimer();

public slots:
    void handleSaveSession(const QString appName, const QString sessionID, const quint16 cbport);
    void handleConnectClosed(const uint16 port);
    void handleRemoveSessionByAppName(const QString appName);
    void handleRemoveSessionBySessionID(const QString sessionID);
    void handleSendToClient(const QString appName, const QString req);
    void handleAboutToQuit();
    void handleAddJob(const QString appName, const int jobId);
    void handleRemoveJob(const QString appName, const int jobID) { emit removeJob(appName, jobID); }
    void handleSendToAllClient(const QString req) { emit sendToAllClient(req); }
    void handlebackendOnline() { emit backendOnline(); }

    //缓存远端离线消息，可能因某次失败而触发，若3秒内有正常则应取消此消息到前端
    void preprocessOfflineStatus(const QString appName, int32 type, const fastring msg);
    void cancelOfflineStatus(const QString appName);

    void handleStartOfflineTimer();
    void handleStopOfflineTimer();

private:
    explicit SendIpcService( QObject *parent = nullptr);
    void initConnect();

private:
    QThread thread;
    QSharedPointer<SendIpcWork> work;
    QTimer _ping;
    QMap<QString, SendStatus> _offline_status;
    QTimer _cacheTimer;
};

#endif // SENDIPCSERVICE_H
