// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FRONTEND_SERVICE_H
#define FRONTEND_SERVICE_H

#include <QObject>
#include <QDebug>

#include "bridge.h"
#include "proto/frontend.h"
#include "co/co.h"

// must change the version if the IPC API changed.
#define FRONTEND_PROTO_VERSION UNI_IPC_PROTO

class FrontendService : public QObject
{
    Q_OBJECT
public:
    explicit FrontendService(QObject *parent = nullptr);
    ~FrontendService();

    co::chan<BridgeJsonData>* bridgeChan();
    co::chan<BridgeJsonData>* bridgeResult();

private:
    co::chan<BridgeJsonData> *_bridge_chan = nullptr;
    co::chan<BridgeJsonData> *_bridge_result = nullptr;
};

namespace ipc {

class FrontendImpl : public Frontend
{
public:
    FrontendImpl() = default;
    virtual ~FrontendImpl() override = default;

    void setInterface(FrontendService *interface) {
        _interface = interface;
    }

    virtual void ping(co::Json& req, co::Json& res) override;

    virtual void cbPeerInfo(co::Json& req, co::Json& res) override;

    virtual void cbConnect(co::Json& req, co::Json& res) override;

    virtual void cbMiscMessage(co::Json& req, co::Json& res) override;

    virtual void cbTransStatus(co::Json& req, co::Json& res) override;

    virtual void cbFsPull(co::Json& req, co::Json& res) override;

    virtual void cbFsAction(co::Json& req, co::Json& res) override;

    virtual void notifyFileStatus(co::Json& req, co::Json& res) override;

    virtual void applyTransFiles(co::Json& req, co::Json& res) override;

    virtual void notifySendStatus(co::Json& req, co::Json& res) override;

    virtual void backendServerOnline(co::Json& req, co::Json& res) override;

    virtual void shareEvents(co::Json& req, co::Json& res) override;

    virtual void cbDisConnect(co::Json& req, co::Json& res) override;

    virtual void searchDeviceRes(co::Json& req, co::Json& res) override;

private:
    FrontendService *_interface;
};

}   // ipc

#endif   // FRONTEND_SERVICE_H
