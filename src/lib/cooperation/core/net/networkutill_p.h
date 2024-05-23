// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKUTIL_P_H
#define NETWORKUTIL_P_H

#include "global_defines.h"
#include "discover/deviceinfo.h"

#include <QObject>

class SessionManager;
namespace cooperation_core {

class NetworkUtil;
class NetworkUtilPrivate : public QObject
{
    friend class NetworkUtil;
    Q_OBJECT
public:
    explicit NetworkUtilPrivate(NetworkUtil *qq);
    ~NetworkUtilPrivate();

private:
    NetworkUtil *q { nullptr };
    SessionManager *sessionManager { nullptr };
    void handleConnectStatus(int result, QString reason);

    QString confirmTargetAddress {}; // remote ip address
    QString storageFolder = {}; //sub folder under storage dir config
};

}

#endif   // NETWORKUTIL_P_H
