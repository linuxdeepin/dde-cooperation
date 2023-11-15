// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENDRPCSERVICE_H
#define SENDRPCSERVICE_H

#include "service/rpc/remoteservice.h"

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QReadLocker>
#include <QThread>
#include <QSharedPointer>

#include "co/json.h"

class SendRpcWork : public QObject {
    Q_OBJECT
    friend class SendRpcService;
public:
    ~SendRpcWork();
    void stop() {_stoped = true;}

Q_SIGNALS:
    void startPingTimer();
    void stopPingTimer();

    void workCreateRpcSender(const QString appName,
                         const QString targetip, quint16 port);
    void workSetTargetAppName(const QString appName, const QString targetAppName);
    void workRemovePing(const QString appName);
    void workDoSendProtoMsg(const uint32 type, const QString AppName,
                            const QString msg, const QByteArray data);

    void ping();

    void sendToRpcResult(const QString appName, const QString msg);

private:
    explicit SendRpcWork( QObject *parent = nullptr);
    void initConnect();

private slots:
    void handleCreateRpcSender(const QString appName,
                         const QString targetip, quint16 port);
    void handleSetTargetAppName(const QString appName, const QString targetAppName);
    void handleDoSendProtoMsg(const uint32 type, const QString appName,
                              const QString msg, const QByteArray data);

    void handlePing();

private:
    QSharedPointer<RemoteServiceSender> createRpcSender(const QString &appName,
                                                        const QString &targetip, uint16_t port);
    QSharedPointer<RemoteServiceSender> rpcSender(const QString &appName);
    QString targetIp() const;
    quint16 targetPort() const;

private:
    QSharedPointer<RemoteServiceSender> _remote;
    QTimer _ping_timer;
    QStringList _ping_appname;
    std::atomic_bool _stoped{false};
};

class SendRpcService : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void startPingTimer();
    void stopPingTimer();
    void createSenderWork(const QString appName,
                          const QString targetip, quint16 port);
    void sendToRpcResult(const QString appName, const QString msg);
public:
    struct ThreadInfo{
        SendRpcWork _work;
        QThread _thread;
        ~ThreadInfo() {
            _work.stop();
            _thread.quit();
            _thread.wait(3000);
        }
    };

public slots:
    void handleStartTimer();
    void handleStopTimer();
    void handleTimeOut();

    void createRpcSenderWork(const QString appName,
                             const QString targetip, quint16 port);

public:
    ~SendRpcService() override;
    static SendRpcService *instance();

    void createRpcSender(const QString &appName,
                         const QString &targetip, quint16 port) {
        createRpcSenderWork(appName, targetip, port);
    }


    void setTargetAppName(const QString &appName, const QString &targetAppName) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workSetTargetAppName(appName, targetAppName);
    }
    void doSendProtoMsg(const uint32 type, const QString &appName,
                        const QString &msg, const QByteArray &data = QByteArray()) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoSendProtoMsg(type, appName, msg, data);
    }

    void removePing(const QString &appName);

    void addPing(const QString &appName);

private:
    explicit SendRpcService( QObject *parent = nullptr);
    QSharedPointer<ThreadInfo> getWork(const QString &appName);

    void initConnet();

private:
    QReadWriteLock _lock;
    QMap<QString, QSharedPointer<ThreadInfo>> _worksMap;
    QList<QSharedPointer<ThreadInfo>> _works;
    QReadWriteLock _ping_lock;
    QStringList _ping_appname;
    QTimer _ping_timer;
};

#endif // SENDRPCSERVICE_H
