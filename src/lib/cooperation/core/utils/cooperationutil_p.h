// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_P_H
#define COOPERATIONUTIL_P_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#include <QObject>

class SessionManager;
namespace cooperation_core {

class MainWindow;
class CooperationUtil;
class CooperationUtilPrivate : public QObject
{
    Q_OBJECT
public:
    explicit CooperationUtilPrivate(CooperationUtil *qq);
    ~CooperationUtilPrivate();

//    void localIPCStart();
//    QList<DeviceInfoPointer> parseDeviceInfo(const co::Json &obj);

public slots:
    void handleConnectStatus(int result, QString reason);

public:
    CooperationUtil *q { nullptr };
    MainWindow *window { nullptr };

    SessionManager *sessionManager { nullptr };
};

}

#endif   // COOPERATIONUTIL_P_H
