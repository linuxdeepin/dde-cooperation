// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_P_H
#define COOPERATIONUTIL_P_H

#include <co/rpc.h>
#include <co/co.h>

#include <QObject>

class FrontendService;
namespace cooperation_core {

class MainWindow;
class CooperationUtil;
class CooperationUtilPrivate : public QObject
{
    Q_OBJECT
public:
    explicit CooperationUtilPrivate(CooperationUtil *qq);

    bool pingBacked();
    void localIPCStart();

public:
    CooperationUtil *q { nullptr };
    MainWindow *window { nullptr };

    FrontendService *frontendIpcSer { nullptr };
    std::shared_ptr<rpc::Client> rpcClient { nullptr };
    QString sessionId;
    bool backendOk { false };
    bool thisDestruct { false };
};

}

#endif   // COOPERATIONUTIL_P_H
