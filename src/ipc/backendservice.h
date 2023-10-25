// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BACKEND_SERVICE_H
#define BACKEND_SERVICE_H

#include <QObject>
#include <QDebug>

#include "bridge.h"
#include "proto/backend.h"
#include "co/co.h"

// must change the version if the IPC API changed.
#define BACKEND_PROTO_VERSION UNI_IPC_PROTO

class BackendService : public QObject
{
    Q_OBJECT
public:
    explicit BackendService(QObject *parent = nullptr);
    ~BackendService();

    co::chan<BridgeJsonData>* bridgeChan();
    co::chan<BridgeJsonData>* bridgeResult();

    fastring getSettingPin() const;
    void setSettingPin(fastring password);

    fastring getOneAppConfig(fastring &app, fastring &key) const;
    void setOneAppConfig(fastring &app, fastring &key, fastring &value);

private:
    co::chan<BridgeJsonData> *_bridge_chan = nullptr;
    co::chan<BridgeJsonData> *_bridge_result = nullptr;
};

namespace ipc {

class BackendImpl : public Backend
{
public:
    BackendImpl() = default;
    virtual ~BackendImpl() = default;

    void setInterface(BackendService *interface) {
        _interface = interface;
    }

    virtual void ping(co::Json& req, co::Json& res) override;

    virtual void getDiscovery(co::Json& req, co::Json& res) override;

    virtual void getPeerInfo(co::Json& req, co::Json& res) override;

    virtual void getPassword(co::Json& req, co::Json& res) override;

    virtual void setPassword(co::Json& req, co::Json& res) override;

    virtual void tryConnect(co::Json& req, co::Json& res) override;

    virtual void setAppConfig(co::Json& req, co::Json& res) override;

    virtual void getAppConfig(co::Json& req, co::Json& res) override;

    virtual void miscMessage(co::Json& req, co::Json& res) override;

    virtual void tryTransFiles(co::Json& req, co::Json& res) override;

    virtual void resumeTransJob(co::Json& req, co::Json& res) override;

    virtual void cancelTransJob(co::Json& req, co::Json& res) override;

    virtual void fsCreate(co::Json& req, co::Json& res) override;

    virtual void fsDelete(co::Json& req, co::Json& res) override;

    virtual void fsRename(co::Json& req, co::Json& res) override;

    virtual void fsPull(co::Json& req, co::Json& res) override;

private:
    BackendService *_interface;
};

}   // ipc

#endif   // BACKEND_SERVICE_H
