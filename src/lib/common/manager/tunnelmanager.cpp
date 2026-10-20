// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tunnelmanager.h"

#include "common/log.h"
#include "tunnelproto.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>
#include <QtEndian>

#include <cmath>

namespace {
QString makeToken()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces)
            + QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool parseStreamId(const QJsonObject &object, quint32 *id)
{
    const double value = object.value(QStringLiteral("stream")).toDouble();
    // Reject zero, fractions and out-of-range values before the cast: a
    // float-to-integer overflow is undefined behavior.
    if (!std::isfinite(value) || value < 1.0 || value > 4294967295.0
        || value != std::floor(value))
        return false;

    *id = static_cast<quint32>(value);
    return true;
}
}

TunnelManager::TunnelManager(QObject *parent)
    : QObject(parent)
{
}

TunnelManager::~TunnelManager()
{
    stopAll();
}

void TunnelManager::setSendHandler(SendHandler handler)
{
    _send = std::move(handler);
}

bool TunnelManager::registerService(const QString &serviceId, const QString &host, quint16 port)
{
    if (serviceId.isEmpty() || serviceId.size() > kMaxServiceIdLength
        || host.isEmpty() || port == 0)
        return false;
    _services.insert(serviceId, Service { host, port });
    return true;
}

void TunnelManager::unregisterService(const QString &serviceId)
{
    _services.remove(serviceId);
}

void TunnelManager::clearServices()
{
    _services.clear();
}

void TunnelManager::setPeer(const QString &peerIp)
{
    if (_peerIp != peerIp)
        _peerRemote.clear();
    _peerIp = peerIp;
}

quint16 TunnelManager::startLocalForward(const QString &serviceId)
{
    if (serviceId.isEmpty() || serviceId.size() > kMaxServiceIdLength
        || _peerIp.isEmpty()) {
        WLOG << "TunnelManager: cannot start forward, service or peer is empty";
        emit localForwardFailed(serviceId, "no peer");
        return 0;
    }

    auto existing = _forwards.find(serviceId);
    if (existing != _forwards.end()) {
        if (existing->server && existing->server->isListening())
            return existing->port;
        // A stale entry without a live listener: drop it and start over.
        stopLocalForward(serviceId);
    }

    if (_forwards.size() >= kMaxForwards) {
        WLOG << "TunnelManager: too many local forwards, reject service:"
             << serviceId.toStdString();
        emit localForwardFailed(serviceId, "too many forwards");
        return 0;
    }

    auto server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, [this, serviceId] {
        acceptLocalConnection(serviceId);
    });
    if (!server->listen(QHostAddress::LocalHost, 0)) {
        const QString error = server->errorString();
        WLOG << "TunnelManager: listen failed:" << error.toStdString();
        server->deleteLater();
        emit localForwardFailed(serviceId, error);
        return 0;
    }

    const quint16 port = server->serverPort();
    _forwards.insert(serviceId, Forward { server, port });
    DLOG << "TunnelManager: forwarding service" << serviceId.toStdString()
         << "on 127.0.0.1:" << port;
    emit localForwardReady(serviceId, port);
    return port;
}

void TunnelManager::stopLocalForward(const QString &serviceId)
{
    auto it = _forwards.find(serviceId);
    if (it == _forwards.end())
        return;

    QTcpServer *server = it->server;
    _forwards.erase(it);

    if (server) {
        server->disconnect(this);
        server->close();
        server->deleteLater();
    }

    // Only the streams belonging to this service are useless without its
    // listener. Other services keep forwarding.
    const auto ids = _streams.keys();
    for (auto id : ids) {
        Stream *stream = _streams.value(id);
        if (stream && !stream->dialer && stream->serviceId == serviceId)
            destroyStream(id, "forward stopped", true);
    }
}

void TunnelManager::stopLocalForward()
{
    const auto services = _forwards.keys();
    for (const auto &serviceId : services)
        stopLocalForward(serviceId);
}

void TunnelManager::stopAll()
{
    stopLocalForward();

    const auto ids = _streams.keys();
    for (auto id : ids)
        destroyStream(id, "stopped", false);

    _streams.clear();
}

bool TunnelManager::isForwarding() const
{
    for (const auto &forward : _forwards) {
        if (forward.server && forward.server->isListening() && forward.port != 0)
            return true;
    }
    return false;
}

quint16 TunnelManager::localPort() const
{
    for (const auto &forward : _forwards) {
        if (forward.server && forward.server->isListening())
            return forward.port;
    }
    return 0;
}

quint16 TunnelManager::localPort(const QString &serviceId) const
{
    const auto it = _forwards.constFind(serviceId);
    if (it == _forwards.constEnd())
        return 0;
    return it->port;
}

quint32 TunnelManager::nextStreamId()
{
    // Stream ids are chosen by the initiating side but live in a namespace
    // shared by both peers. A counter starting at 1 on each side collides as
    // soon as both initiate a stream (barrier plus a reverse file transfer),
    // so pick a random id and skip ids already known locally.
    for (;;) {
        const quint32 id = QRandomGenerator::global()->generate();
        if (id != 0 && !_streams.contains(id))
            return id;
    }
}

void TunnelManager::acceptLocalConnection(const QString &serviceId)
{
    const auto forward = _forwards.constFind(serviceId);
    if (forward == _forwards.constEnd() || !forward->server)
        return;

    QTcpServer *server = forward->server;
    while (server->hasPendingConnections()) {
        QTcpSocket *socket = server->nextPendingConnection();
        if (!socket)
            break;

        const quint32 id = nextStreamId();

        const QString token = makeToken();
        Stream *stream = createStream(id, serviceId, token, false);
        stream->sock = socket;
        socket->setParent(this);
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        setupSocket(stream);
        startConnectTimeout(stream);

        if (!_send || _peerIp.isEmpty()) {
            destroyStream(id, "no session", false);
            continue;
        }

        _send(_peerIp, TUNNEL_OPEN, makeOpen(id, serviceId, token));
        DLOG << "TunnelManager: open stream" << id << "service" << serviceId.toStdString();
    }
}

void TunnelManager::setupSocket(Stream *stream)
{
    if (!stream || !stream->sock)
        return;

    const quint32 id = stream->id;
    QTcpSocket *socket = stream->sock;

    connect(socket, &QTcpSocket::readyRead, this, [this, id] { pump(id); });
    connect(socket, &QTcpSocket::bytesWritten, this, [this, id](qint64 bytes) {
        // Acknowledge only after the data has left Qt's write buffer. This is
        // what actually applies backpressure to the peer.
        if (bytes > 0)
            sendAck(id, static_cast<quint32>(bytes));
        flushPending(id);
    });
    connect(socket, &QTcpSocket::disconnected, this, [this, id] {
        destroyStream(id, "socket closed", true);
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this, id](QAbstractSocket::SocketError error) {
        Q_UNUSED(error)
        Stream *stream = findStream(id);
        if (!stream)
            return;

        if (!stream->ready && stream->dialer) {
            // The barrier server may still be starting up. Retry while the
            // overall connect timeout is running instead of failing at once.
            if (stream->dialAttempts < kDialAttempts) {
                stream->dialAttempts++;
                DLOG << "TunnelManager: retry dial for stream" << id
                     << "attempt" << stream->dialAttempts;
                QTimer::singleShot(kDialRetryMs, this, [this, id] { dialService(id); });
                return;
            }

            if (_send && !_peerIp.isEmpty())
                _send(_peerIp, TUNNEL_OPEN_ACK, makeOpenAck(id, false, stream->token, "connect failed"));
            destroyStream(id, "connect failed", false);
            return;
        }

        destroyStream(id, "socket error", true);
    });
}

void TunnelManager::startConnectTimeout(Stream *stream)
{
    if (!stream)
        return;

    const quint32 id = stream->id;
    stream->timeout = new QTimer(this);
    stream->timeout->setSingleShot(true);
    connect(stream->timeout, &QTimer::timeout, this, [this, id] {
        Stream *current = findStream(id);
        if (!current || current->ready)
            return;

        if (_send && !_peerIp.isEmpty()) {
            if (current->dialer)
                _send(_peerIp, TUNNEL_OPEN_ACK, makeOpenAck(id, false, current->token, "connect timeout"));
            else
                sendClose(id);
        }
        destroyStream(id, "connect timeout", false);
    });
    stream->timeout->start(kConnectTimeoutMs);
}

void TunnelManager::handleOpen(const QString &remote, const QByteArray &payload)
{
    quint32 id = 0;
    QString serviceId;
    QString token;
    if (!parseOpen(payload, &id, &serviceId, &token))
        return;

    if (!_send) {
        WLOG << "TunnelManager: open ignored, session sender is not set";
        return;
    }

    if (_peerIp.isEmpty() || remote != _peerIp) {
        WLOG << "TunnelManager: reject open from unexpected peer:" << remote.toStdString()
             << "expected:" << _peerIp.toStdString();
        if (_send && !remote.isEmpty())
            _send(remote, TUNNEL_OPEN_ACK, makeOpenAck(id, false, token, "unexpected peer"));
        return;
    }

    auto service = _services.constFind(serviceId);
    if (service == _services.constEnd()) {
        WLOG << "TunnelManager: reject unknown service:" << serviceId.toStdString();
        _send(_peerIp, TUNNEL_OPEN_ACK, makeOpenAck(id, false, token, "unknown service"));
        return;
    }

    if (_streams.contains(id)) {
        WLOG << "TunnelManager: duplicate stream id:" << id;
        _send(_peerIp, TUNNEL_OPEN_ACK, makeOpenAck(id, false, token, "duplicate stream"));
        return;
    }

    Stream *stream = createStream(id, serviceId, token, true);
    stream->sock = new QTcpSocket(this);
    stream->sock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    setupSocket(stream);
    startConnectTimeout(stream);

    connect(stream->sock, &QTcpSocket::connected, this, [this, id] {
        Stream *current = findStream(id);
        if (!current)
            return;

        current->ready = true;
        if (current->timeout) {
            current->timeout->stop();
            current->timeout->deleteLater();
            current->timeout = nullptr;
        }

        _send(_peerIp, TUNNEL_OPEN_ACK, makeOpenAck(id, true, current->token));
        flushPending(id);

        DLOG << "TunnelManager: stream" << id << "connected to service"
             << current->serviceId.toStdString();
        emit streamOpened(id, current->serviceId);
        pump(id);
    });

    DLOG << "TunnelManager: dial service" << serviceId.toStdString()
         << "for stream" << id;
    stream->sock->connectToHost(service->host, service->port);
}

void TunnelManager::dialService(quint32 id)
{
    Stream *stream = findStream(id);
    if (!stream || stream->ready || !stream->sock)
        return;

    const auto service = _services.constFind(stream->serviceId);
    if (service == _services.constEnd()) {
        if (_send && !_peerIp.isEmpty())
            _send(_peerIp, TUNNEL_OPEN_ACK, makeOpenAck(id, false, stream->token, "service removed"));
        destroyStream(id, "service removed", false);
        return;
    }

    stream->sock->abort();
    stream->sock->connectToHost(service->host, service->port);
}

void TunnelManager::handleOpenAck(const QString &remote, const QByteArray &payload)
{
    quint32 id = 0;
    bool ok = false;
    QString token;
    QString reason;
    if (!parseOpenAck(payload, &id, &ok, &token, &reason))
        return;

    Stream *stream = findStream(id);
    if (!stream || stream->dialer)
        return;

    // The token authenticates the answer even when the peer's source address
    // differs from the advertised one (NAT).
    if (token != stream->token) {
        WLOG << "TunnelManager: stream" << id << "token mismatch, ignore";
        return;
    }

    if (!ok) {
        WLOG << "TunnelManager: stream" << id << "rejected:"
             << reason.toStdString();
        destroyStream(id, reason.isEmpty() ? "rejected" : reason, false);
        return;
    }

    // Remember the actual socket address. Behind NAT it differs from the
    // advertised peer address used as the send target.
    _peerRemote = remote;

    stream->ready = true;
    if (stream->timeout) {
        stream->timeout->stop();
        stream->timeout->deleteLater();
        stream->timeout = nullptr;
    }

    DLOG << "TunnelManager: stream" << id << "opened";
    emit streamOpened(id, stream->serviceId);
    flushPending(id);
    pump(id);
}

void TunnelManager::handleData(const QByteArray &payload)
{
    if (payload.size() < 4)
        return;

    bool ok = false;
    const quint32 id = readU32(payload, 0, &ok);
    if (!ok)
        return;

    Stream *stream = findStream(id);
    if (!stream)
        return;

    const QByteArray chunk = payload.mid(4);
    if (chunk.isEmpty())
        return;

    // Always queue first: a partial socket write must not let a later chunk
    // overtake the bytes still waiting in pending.
    if (stream->pending.size() + chunk.size() > kMaxPending) {
        WLOG << "TunnelManager: stream" << id << "pending overflow";
        destroyStream(id, "pending overflow", true);
        return;
    }

    stream->pending.append(chunk);
    flushPending(id);
}

void TunnelManager::handleAck(const QByteArray &payload)
{
    if (payload.size() < 8)
        return;

    bool ok = false;
    const quint32 id = readU32(payload, 0, &ok);
    if (!ok)
        return;
    const quint32 consumed = readU32(payload, 4, &ok);
    if (!ok)
        return;

    Stream *stream = findStream(id);
    if (!stream)
        return;

    stream->unacked -= consumed;
    if (stream->unacked < 0)
        stream->unacked = 0;

    if (stream->unacked < kWindowLowWater)
        pump(id);
}

void TunnelManager::handleClose(const QByteArray &payload)
{
    if (payload.size() < 4)
        return;

    bool ok = false;
    const quint32 id = readU32(payload, 0, &ok);
    if (!ok)
        return;

    destroyStream(id, "peer closed", false);
}

void TunnelManager::onTunnelMessage(int mask, const QByteArray &payload, const QString &remote)
{
    if (payload.size() > TUNNEL_MAX_FRAME_SIZE) {
        WLOG << "TunnelManager: drop oversized frame, mask:" << mask
             << "size:" << payload.size();
        return;
    }

    const bool peerMatches = remote == _peerIp
            || (!_peerRemote.isEmpty() && remote == _peerRemote);
    if (_peerIp.isEmpty() || !peerMatches) {
        // The first answer from a NATed peer arrives before we know its socket
        // address. The random per-stream token authenticates it.
        if (mask == TUNNEL_OPEN_ACK) {
            handleOpenAck(remote, payload);
            return;
        }
        if (mask == TUNNEL_OPEN)
            WLOG << "TunnelManager: drop open from:" << remote.toStdString()
                 << "expected:" << _peerIp.toStdString();
        return;
    }

    switch (mask) {
    case TUNNEL_OPEN:
        handleOpen(remote, payload);
        break;
    case TUNNEL_OPEN_ACK:
        handleOpenAck(remote, payload);
        break;
    case TUNNEL_DATA:
        handleData(payload);
        break;
    case TUNNEL_ACK:
        handleAck(payload);
        break;
    case TUNNEL_CLOSE:
        handleClose(payload);
        break;
    default:
        break;
    }
}

void TunnelManager::onPeerDisconnected()
{
    const auto ids = _streams.keys();
    for (auto id : ids)
        destroyStream(id, "peer disconnected", false);

    stopLocalForward();
    _peerIp.clear();
    _peerRemote.clear();
}

TunnelManager::Stream *TunnelManager::findStream(quint32 id) const
{
    return _streams.value(id, nullptr);
}

TunnelManager::Stream *TunnelManager::createStream(quint32 id, const QString &serviceId,
                                                   const QString &token, bool dialer)
{
    Stream *stream = new Stream;
    stream->id = id;
    stream->serviceId = serviceId;
    stream->token = token;
    stream->dialer = dialer;
    _streams.insert(id, stream);
    return stream;
}

void TunnelManager::destroyStream(quint32 id, const QString &reason, bool notifyPeer)
{
    Stream *stream = _streams.take(id);
    if (!stream)
        return;

    if (notifyPeer && _send && !_peerIp.isEmpty())
        sendClose(id);

    if (stream->timeout) {
        stream->timeout->stop();
        stream->timeout->deleteLater();
        stream->timeout = nullptr;
    }

    if (stream->sock) {
        stream->sock->disconnect(this);
        stream->sock->abort();
        stream->sock->deleteLater();
        stream->sock = nullptr;
    }

    DLOG << "TunnelManager: stream" << id << "closed:"
         << reason.toStdString();
    delete stream;
    emit streamClosed(id, reason);
}

void TunnelManager::pump(quint32 id)
{
    Stream *stream = findStream(id);
    if (!stream || !stream->ready || !stream->sock)
        return;

    while (stream->unacked < kWindowSize && stream->sock->bytesAvailable() > 0) {
        const qint64 want = qMin<qint64>(kChunkSize, kWindowSize - stream->unacked);
        const QByteArray chunk = stream->sock->read(want);
        if (chunk.isEmpty())
            break;

        sendData(id, chunk);
        stream->unacked += chunk.size();
    }
}

void TunnelManager::sendData(quint32 id, const QByteArray &data)
{
    if (!_send || _peerIp.isEmpty())
        return;

    QByteArray frame;
    frame.reserve(4 + data.size());
    appendU32(frame, id);
    frame.append(data);
    _send(_peerIp, TUNNEL_DATA, frame);
}

void TunnelManager::sendAck(quint32 id, quint32 consumed)
{
    if (!_send || _peerIp.isEmpty())
        return;

    QByteArray frame;
    appendU32(frame, id);
    appendU32(frame, consumed);
    _send(_peerIp, TUNNEL_ACK, frame);
}

void TunnelManager::sendClose(quint32 id)
{
    if (!_send || _peerIp.isEmpty())
        return;

    QByteArray frame;
    appendU32(frame, id);
    _send(_peerIp, TUNNEL_CLOSE, frame);
}

void TunnelManager::flushPending(quint32 id)
{
    Stream *stream = findStream(id);
    if (!stream || !stream->ready || !stream->sock || stream->pending.isEmpty())
        return;
    if (stream->sock->state() != QAbstractSocket::ConnectedState)
        return;

    while (!stream->pending.isEmpty()) {
        const qint64 written = stream->sock->write(stream->pending);
        if (written < 0) {
            destroyStream(id, "write failed", true);
            return;
        }
        if (written == 0)
            break;

        // The ACK is sent from the bytesWritten handler once Qt has actually
        // drained these bytes to the local socket.
        stream->pending.remove(0, static_cast<int>(written));
    }
}

QByteArray TunnelManager::makeOpen(quint32 id, const QString &service, const QString &token)
{
    QJsonObject object;
    object.insert(QStringLiteral("stream"), static_cast<double>(id));
    object.insert(QStringLiteral("service"), service);
    object.insert(QStringLiteral("token"), token);
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QByteArray TunnelManager::makeOpenAck(quint32 id, bool ok, const QString &token, const QString &reason)
{
    QJsonObject object;
    object.insert(QStringLiteral("stream"), static_cast<double>(id));
    object.insert(QStringLiteral("ok"), ok);
    object.insert(QStringLiteral("token"), token);
    if (!reason.isEmpty())
        object.insert(QStringLiteral("reason"), reason);
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

bool TunnelManager::parseOpen(const QByteArray &payload, quint32 *id, QString *service, QString *token)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;

    const QJsonObject object = document.object();
    if (!parseStreamId(object, id))
        return false;
    *service = object.value(QStringLiteral("service")).toString();
    *token = object.value(QStringLiteral("token")).toString();
    return !service->isEmpty() && !token->isEmpty();
}

bool TunnelManager::parseOpenAck(const QByteArray &payload, quint32 *id, bool *ok,
                                 QString *token, QString *reason)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return false;

    const QJsonObject object = document.object();
    if (!parseStreamId(object, id))
        return false;
    *ok = object.value(QStringLiteral("ok")).toBool();
    *token = object.value(QStringLiteral("token")).toString();
    *reason = object.value(QStringLiteral("reason")).toString();
    return true;
}

void TunnelManager::appendU32(QByteArray &out, quint32 value)
{
    uchar buffer[4];
    qToBigEndian<quint32>(value, buffer);
    out.append(reinterpret_cast<const char *>(buffer), 4);
}

quint32 TunnelManager::readU32(const QByteArray &data, int offset, bool *ok)
{
    if (offset < 0 || offset + 4 > data.size()) {
        if (ok)
            *ok = false;
        return 0;
    }

    if (ok)
        *ok = true;
    return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + offset));
}
