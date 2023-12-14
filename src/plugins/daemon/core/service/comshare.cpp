// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "comshare.h"


Comshare::Comshare()
{

}

Comshare::~Comshare()
{

}

Comshare *Comshare::instance()
{
    static Comshare s;
    return &s;
}

void Comshare::updateStatus(const CurrentStatus st)
{
    QWriteLocker lk(&_lock);
    _cur_status = st;
}

void Comshare::updateComdata(const QString &appName, const QString &tarappName, const QString &ip)
{
    QWriteLocker lk(&_lock);
    _target_app.remove(appName);
    _target_app.insert(appName, tarappName);
    _target_ip.remove(appName);
    _target_ip.insert(appName, ip);
}

QString Comshare::targetAppName(const QString &appName)
{
    QReadLocker lk(&_lock);
    return _target_app.value(appName);
}

QString Comshare::targetIP(const QString &appName)
{
    QReadLocker lk(&_lock);
    return _target_ip.value(appName);
}

CurrentStatus Comshare::currentStatus()
{
    QReadLocker lk(&_lock);
    return static_cast<CurrentStatus>(_cur_status.load());
}

bool Comshare::checkTransCanConnect()
{
    QReadLocker lk(&_lock);
    return _cur_status <= CurrentStatus::CURRENT_STATUS_DISCONNECT ||
            _cur_status > CURRENT_STATUS_TRAN_FILE_RCV;
}
