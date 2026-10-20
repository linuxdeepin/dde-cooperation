// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKUTIL_P_H
#define NETWORKUTIL_P_H

#include "global_defines.h"
#include "discover/deviceinfo.h"

#include <QObject>

#include <atomic>

class SessionManager;
class TunnelManager;
namespace cooperation_core {

class NetworkUtil;
class NetworkUtilPrivate : public QObject
{
    friend class NetworkUtil;
    Q_OBJECT
public:
    explicit NetworkUtilPrivate(NetworkUtil *qq);
    ~NetworkUtilPrivate();

    QString getStorageFolder() const;

    QString getConfirmTargetAddress() const;

    // Select the tunnel peer when no cooperation is active. A different peer
    // already using the tunnel is never replaced, so an active barrier
    // session keeps working.
    bool ensureTunnelPeer(const QString &ip);

public Q_SLOTS:
    void handleConnectStatus(int result, QString reason);
    void handleTransChanged(int status, const QString &path, quint64 size);
    void handleAsyncRpcResult(int32_t type, const QString response);

private:
    NetworkUtil *q { nullptr };
    SessionManager *sessionManager { nullptr };
    TunnelManager *tunnelManager { nullptr };

    QString confirmTargetAddress {""};   // remote ip address
    QString storageRoot = {};   //storage dir config
    QString storageFolder = {};   //sub folder under storage dir config
    int servePort;
    // Transfer tunnel negotiation result for the current apply/send. The
    // session callbacks run on the ASIO thread, the actions using them on the
    // GUI thread.
    std::atomic<bool> transTunnelOffered { false };   // peer offers the tunnel
    std::atomic<bool> peerTransTunnel { false };      // peer can forward for us
};

}

#endif   // NETWORKUTIL_P_H
