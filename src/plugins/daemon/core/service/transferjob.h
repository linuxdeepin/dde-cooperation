// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERJOB_H
#define TRANSFERJOB_H

#include <QObject>
#include <service/rpc/remoteservice.h>

class TransferJob : public QObject
{
    Q_OBJECT

    typedef int (*PushfileJobFunc)(int id, const char *);

public:
    explicit TransferJob(QObject *parent = nullptr);
    void initJob(int id, QStringList paths, bool sub, QString savedir);

    void startJob(bool push, RemoteServiceBinder *binder);
    void stop();
    bool finished();

    bool isRunning() {
        return !_stoped;
    }

signals:

public slots:

private:
    void trySend(const char *path, int id, RemoteServiceBinder *binder);

    bool _inited = false;
    bool _stoped = true;
    bool _finished = false;

    int _id;
    QStringList _paths;
    bool _sub;
    QString _savedir;
};

#endif // TRANSFERJOB_H