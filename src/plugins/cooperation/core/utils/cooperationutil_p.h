// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_P_H
#define COOPERATIONUTIL_P_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#include <co/json.h>
#include <co/co.h>

#include <QObject>


namespace cooperation_core {

class MainWindow;
class CooperationUtil;
class CooperationUtilPrivate : public QObject
{
    Q_OBJECT
public:
    explicit CooperationUtilPrivate(CooperationUtil *qq);
    ~CooperationUtilPrivate();

    QList<DeviceInfoPointer> parseDeviceInfo(const co::Json &obj);

public slots:
    void backendMessageSlot(int type, const QString& msg);

public:
    CooperationUtil *q { nullptr };
    MainWindow *window { nullptr };

    QString sessionId;
    bool backendOk { false };

    CuteIPCInterface *ipcInterface { nullptr };
};

}

#endif   // COOPERATIONUTIL_P_H
