// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SESSION_H
#define SESSION_H

#include <QObject>
#include <co/co.h>
#include <co/rpc.h>

class Session : public QObject
{
    Q_OBJECT
public:
    explicit Session(QString name, QString session, uint16 port, QObject *parent = nullptr);
    virtual ~Session();

    bool valid();
    bool alive();

    QString getName();
    QString getSession();
    void addJob(int jobid);
    bool removeJob(int jobid);
    int hasJob(int jobid);
    rpc::Client* client();
    void call(const json::Json& req, json::Json& res);
    uint16 port() const;

signals:

public slots:

private:
    QString _name;
    QString _sessionid;
    uint16 _cb_port = 0;
    co::vector<int> _jobs;

    std::shared_ptr<rpc::Client> coClient { nullptr };

    bool _initPing = false;
    bool _pingOK = false;
};

#endif // SESSION_H
