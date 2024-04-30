// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_P_H
#define COOPERATIONUTIL_P_H

#include "global_defines.h"

#include <QObject>

class FrontendService;
namespace daemon_cooperation {

class MainWindow;
class CooperationUtil;
class CooperationUtilPrivate : public QObject
{
    Q_OBJECT
public:
    explicit CooperationUtilPrivate(CooperationUtil *qq);
    ~CooperationUtilPrivate();

public:
    CooperationUtil *q { nullptr };
};

}

#endif   // COOPERATIONUTIL_P_H
