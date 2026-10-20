// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TUNNELMANAGER_H
#define TUNNELMANAGER_H

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>

#include <functional>

class QTcpServer;
class QTcpSocket;
class QTimer;

// Generic reverse stream tunnel built on the existing cooperation session.
//
// The peer that can only be reached in one direction (the "dialer") exposes a
// loopback port locally. Every accepted connection is announced to the other
// side, which dials a registered local service and pipes the bytes back.
//
// The tunnel never accepts a peer supplied host/port. Only services registered
// through registerService() can be opened.
class TunnelManager : public QObject
{
    Q_OBJECT
public:
    using SendHandler = std::function<void(const QString &target, int mask, const QByteArray &payload)>;

    explicit TunnelManager(QObject *parent = nullptr);
    ~TunnelManager() override;

    void setSendHandler(SendHandler handler);

    // A side: local services that the peer is allowed to open.
    bool registerService(const QString &serviceId, const QString &host, quint16 port);
    void unregisterService(const QString &serviceId);
    void clearServices();

    // Only this peer may open streams. Call before startLocalForward().
    void setPeer(const QString &peerIp);
    QString peer() const { return _peerIp; }
    // Actual socket address learned from a successful OPEN_ACK. It may differ
    // from the advertised peer address behind NAT.
    QString peerRemote() const { return _peerRemote; }

    // B side: listen on 127.0.0.1 and forward every accepted connection to the
    // peer's service. Returns the bound port, or 0 on failure.
    // One forward per service, several services can be active at the same
    // time (barrier plus concurrent file transfers).
    quint16 startLocalForward(const QString &serviceId);
    void stopLocalForward(const QString &serviceId);
    void stopLocalForward();
    void stopAll();

    bool isForwarding() const;
    quint16 localPort() const;
    quint16 localPort(const QString &serviceId) const;

signals:
    void localForwardReady(const QString &serviceId, quint16 port);
    void localForwardFailed(const QString &serviceId, const QString &reason);
    void streamOpened(quint32 streamId, const QString &serviceId);
    void streamClosed(quint32 streamId, const QString &reason);

public slots:
    void onTunnelMessage(int mask, const QByteArray &payload, const QString &remote);
    void onPeerDisconnected();

private:
    struct Stream {
        quint32 id { 0 };
        QString serviceId;
        QString token;
        QTcpSocket *sock { nullptr };
        QByteArray pending;
        qint64 unacked { 0 };
        int dialAttempts { 0 };
        bool ready { false };
        bool dialer { false };
        QTimer *timeout { nullptr };
    };
    struct Service {
        QString host;
        quint16 port { 0 };
    };
    struct Forward {
        QTcpServer *server { nullptr };
        quint16 port { 0 };
    };

    void handleOpen(const QString &remote, const QByteArray &payload);
    void handleOpenAck(const QString &remote, const QByteArray &payload);
    void handleData(const QByteArray &payload);
    void handleAck(const QByteArray &payload);
    void handleClose(const QByteArray &payload);

    Stream *findStream(quint32 id) const;
    Stream *createStream(quint32 id, const QString &serviceId, const QString &token, bool dialer);
    void destroyStream(quint32 id, const QString &reason, bool notifyPeer);

    void acceptLocalConnection(const QString &serviceId);
    void setupSocket(Stream *stream);
    void startConnectTimeout(Stream *stream);
    void pump(quint32 id);
    void flushPending(quint32 id);
    void sendData(quint32 id, const QByteArray &data);
    void sendAck(quint32 id, quint32 consumed);
    void sendClose(quint32 id);
    void dialService(quint32 id);

    static QByteArray makeOpen(quint32 id, const QString &service, const QString &token);
    static QByteArray makeOpenAck(quint32 id, bool ok, const QString &token, const QString &reason = QString());
    static bool parseOpen(const QByteArray &payload, quint32 *id, QString *service, QString *token);
    static bool parseOpenAck(const QByteArray &payload, quint32 *id, bool *ok, QString *token, QString *reason);
    static void appendU32(QByteArray &out, quint32 value);
    static quint32 readU32(const QByteArray &data, int offset, bool *ok = nullptr);

    static constexpr int kChunkSize = 16 * 1024;
    static constexpr int kWindowSize = 256 * 1024;
    static constexpr int kWindowLowWater = 64 * 1024;
    static constexpr int kMaxPending = 1024 * 1024;
    static constexpr int kConnectTimeoutMs = 5000;
    static constexpr int kDialAttempts = 4;
    static constexpr int kDialRetryMs = 300;
    static constexpr int kMaxForwards = 16;
    static constexpr int kMaxServiceIdLength = 128;

    SendHandler _send;
    QHash<QString, Service> _services;
    QHash<QString, Forward> _forwards;
    QHash<quint32, Stream *> _streams;
    QString _peerIp;
    QString _peerRemote;
    quint32 _nextStreamId { 1 };
};

#endif   // TUNNELMANAGER_H
