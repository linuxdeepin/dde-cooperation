// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "session.h"
#include "common/constant.h"
#include "co/all.h"

Session::Session(QString name, QString session, int port, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _sessionid(session)
    , _cb_port(port)
{
    // initialize the proto client
    coClient = std::shared_ptr<rpc::Client>(new rpc::Client("127.0.0.1", _cb_port, false));

    _jobs.clear();

    _pingOK = false;
    _initPing = false;
}

Session::~Session()
{
    _jobs.reset();
    coClient->close();
}

bool Session::valid()
{
    if (!_initPing) {
        std::thread coThread([this]() {
            alive();
        });
        coThread.join();
    }
    return _pingOK;
}

bool Session::alive()
{
    fastring version(UNI_IPC_PROTO);
    fastring ses(_sessionid.toStdString());

    co::Json req, res;
    //ping {result, msg}
    req = {
        { "session", ses },
        { "version", version },
    };
    req.add_member("api", "Frontend.ping");
    client()->call(req, res);

    _pingOK = res.get("result").as_bool() && !res.get("msg").empty();
    _initPing = true;
    return _pingOK;
}

QString Session::getName()
{
    return _name;
}

QString Session::getSession()
{
    return _sessionid;
}

void Session::addJob(int jobid)
{
    _jobs.push_back(jobid);
}

bool Session::removeJob(int jobid)
{
    int i = hasJob(jobid);
    if (i >= 0) {
        _jobs.remove(i);
        return true;
    }
    return false;
}

int Session::hasJob(int jobid)
{
    for (size_t i = 0; i < _jobs.size(); ++i) {
        if (_jobs[i] == jobid) {
            return i;
        }
    }
    return -1;
}

rpc::Client* Session::client()
{
    return coClient.get();
}
