// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "session.h"
#include "common/constant.h"
#include "co/all.h"

Session::Session(QString name, QString session, uint16 port, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _sessionid(session)
    , _cb_port(port)
{
    _jobs.clear();

    coClient.reset( new rpc::Client("127.0.0.1", _cb_port, false));
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
        // thread lamada crash on windows, use corroutine
        alive();
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
    call(req, res);

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
        _jobs.remove(static_cast<size_t>(i));
        return true;
    }
    return false;
}

int Session::hasJob(int jobid)
{
    for (size_t i = 0; i < _jobs.size(); ++i) {
        if (_jobs[i] == jobid) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

rpc::Client* Session::client()
{
    return coClient.get();
}

void Session::call(const json::Json &req, json::Json &res)
{
    if (!req.str().contains("Frontend.ping"))
        DLOG_IF(TEST_LOGOUT) << "Send To Client  : session = " << _name.toStdString() << ", port = " << _cb_port
             << " \n req : " << req;
    coClient.reset( new rpc::Client("127.0.0.1", _cb_port, false));
#if defined(WIN32)
    co::wait_group wg;
    wg.add(1);
    UNIGO([this, &req, &res, wg]() {
        coClient->call(req, res);
        wg.done();
    });
    wg.wait();
#else
    coClient->call(req, res);
#endif
    if (req.get("api").str().contains("Frontend.ping")) {
        _pingOK = res.get("result").as_bool() && !res.get("msg").empty();
    } else {
        DLOG_IF(TEST_LOGOUT) << "Client reply : session = " << _name.toStdString() << ", api name = "
             << req.get("api").str().c_str()
             << " \n res : " << res;
    }
    coClient->close();
}

uint16 Session::port() const
{
    return _cb_port;
}

