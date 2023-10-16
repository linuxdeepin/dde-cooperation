// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BACKEND_SERVICE_H
#define BACKEND_SERVICE_H

#include <QObject>
#include <QDebug>

#include "proto/backend.h"

class BackendService : public QObject
{
    Q_OBJECT
public:
    explicit BackendService(QObject *parent = nullptr);
    ~BackendService();

    fastring handlePing(const char *who, const char *version, int cbport);
    fastring getSettingPin() const;
    void setSettingPin(fastring password);
    void handleConnect(const char *session, const char *ip, const char *password);
    void handleSendFiles(QString session, int jobId, QStringList &paths, bool sub, QString savedir);

signals:
    void sigConnect(QString ip, QString name, QString pin);
    void sigSendFiles(QString session, int jobId, QStringList paths, bool sub, QString savedir);
    void sigSaveSession(QString who, QString session, int cbport);

public slots:

private:
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

    virtual void tryTargetSpace(co::Json& req, co::Json& res) override;

    virtual void tryApplist(co::Json& req, co::Json& res) override;

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
