// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FRONTEND_SERVICE_H
#define FRONTEND_SERVICE_H

#include <QObject>
#include <QDebug>

#include "proto/frontend.h"

class FrontendService : public QObject
{
    Q_OBJECT
public:
    explicit FrontendService(QObject *parent = nullptr);
    ~FrontendService();

    void handlePing(QString sessionid);
    void handleConnectstatus(int result, QString msg);
    void handleTransJobstatus(int id, int result, QString msg);
    void handleFileTransstatus(QString statusstr);
    void handlePeerChanges(bool find, fastring peerinfo);

signals:
    void sigSession(QString sessionid);
    void sigConnectStatus(int result, QString msg);
    void sigTransJobtatus(int id, int result, QString msg);
    void sigFileTransStatus(QString statusstr);
    void sigPeerChanged(bool find, QString peerinfo);

public slots:

private:
    int _job_id = 0;
};

namespace ipc {

class FrontendImpl : public Frontend
{
public:
    FrontendImpl() = default;
    virtual ~FrontendImpl() = default;

    void setInterface(FrontendService *interface) {
        _interface = interface;
    }

    virtual void ping(co::Json& req, co::Json& res) override;

    virtual void cbPeerInfo(co::Json& req, co::Json& res) override;

    virtual void cbConnect(co::Json& req, co::Json& res) override;

    virtual void cbTargetSpace(co::Json& req, co::Json& res) override;

    virtual void cbApplist(co::Json& req, co::Json& res) override;

    virtual void cbMiscMessage(co::Json& req, co::Json& res) override;

    virtual void cbTransStatus(co::Json& req, co::Json& res) override;

    virtual void cbFsPull(co::Json& req, co::Json& res) override;

    virtual void cbFsAction(co::Json& req, co::Json& res) override;

    virtual void notifyFileStatus(co::Json& req, co::Json& res) override;

private:
    FrontendService *_interface;
};

}   // ipc

#endif   // FRONTEND_SERVICE_H
