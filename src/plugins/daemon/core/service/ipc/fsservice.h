// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FS_SERVICE_H
#define FS_SERVICE_H

#include <QObject>
#include <QDebug>

#include "ipc/fs.h"

class FSService : public QObject
{
    Q_OBJECT
public:
    explicit FSService(QObject *parent = nullptr);
    ~FSService();

    void handleSendFiles(QStringList &paths);

signals:
    void sigSendFiles(int jobId, QStringList paths, QString savedir);

public slots:

private:
    int _job_id = 0;
};

namespace ipc {

class FSImpl : public FS
{
public:
    FSImpl() = default;
    virtual ~FSImpl() = default;

    void setInterface(FSService *interface) {
        _interface = interface;
    }

    virtual void compatible(co::Json &req, co::Json &res) override;

    virtual void readDir(co::Json &req, co::Json &res) override;

    virtual void removeDir(co::Json &req, co::Json &res) override;

    virtual void create(co::Json &req, co::Json &res) override;

    virtual void rename(co::Json &req, co::Json &res) override;

    virtual void removeFiles(co::Json &req, co::Json &res) override;

    virtual void sendFiles(co::Json &req, co::Json &res) override;

    virtual void receiveFiles(co::Json &req, co::Json &res) override;

    virtual void resumeJob(co::Json &req, co::Json &res) override;

    virtual void cancelJob(co::Json &req, co::Json &res) override;

    virtual void fsReport(co::Json &req, co::Json &res) override;

private:
    FSService *_interface;
};

}   // ipc

#endif   // FS_SERVICE_H
