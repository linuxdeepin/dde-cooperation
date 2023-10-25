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
    _client_pool = new co::Pool(
        [this]() { return (void*) new rpc::Client("127.0.0.1", _cb_port, false); },
        [](void* p) { delete (rpc::Client*) p; }
    );

    _jobs.clear();

    _pingOK = false;
    go([this]() {
        alive();
    });
}

Session::~Session()
{
    _jobs.reset();
    delete _client_pool;
}

bool Session::valid()
{
    return _pingOK;
}

bool Session::alive()
{
    fastring version(UNI_IPC_PROTO);
    fastring ses(_sessionid.toStdString());
    co::pool_guard<rpc::Client> c(_client_pool);
    co::Json req, res;
    //ping {result, msg}
    req = {
        { "session", ses },
        { "version", version },
    };
    req.add_member("api", "Frontend.ping");
    c->call(req, res);

    _pingOK = res.get("result").as_bool() && !res.get("msg").empty();
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

co::pool* Session::clientPool()
{
    return _client_pool;
}
