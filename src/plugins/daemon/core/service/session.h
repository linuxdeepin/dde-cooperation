// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <co/co.h>

class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(QString name, QString session, int port, QObject *parent = nullptr);
    virtual ~Session();

    bool valid();

    QString getName();
    QString getSession();
    void addJob(int jobid);
    bool removeJob(int jobid);
    int hasJob(int jobid);
    co::pool* clientPool();

signals:

public slots:

private:
    QString _name;
    QString _sessionid;
    int _cb_port = 0;
    co::vector<int> _jobs;

    co::pool *_client_pool = nullptr;

    bool _pingOK = false;
};

#endif // SESSION_H
