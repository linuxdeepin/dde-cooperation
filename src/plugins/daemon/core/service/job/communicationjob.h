// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMMUNICATIONJOB_H
#define COMMUNICATIONJOB_H

#include <QObject>
#include <service/rpc/remoteservice.h>
#include <ipc/proto/chan.h>
#include <service/comshare.h>
#include "co/co.h"
#include "co/time.h"

class CommunicationJob : public QObject
{
    Q_OBJECT

public:
    explicit CommunicationJob(QObject *parent = nullptr);
    ~CommunicationJob() override;
    void initRpc(fastring appname, fastring target, uint16 port);
    void initJob(fastring appname, fastring targetappname);

    fastring getAppName();
    fastring getTarAppName() const;
    int sendMsg(CommunicationType type, const QString &info);
private:
    bool _inited = false;
    uint16 _port = 0;
    fastring _targetIP;
    fastring _app_name; // 前端应用名
    fastring _tar_app_name; // 发送到目标的应用名称

    RemoteServiceBinder *_rpcBinder = nullptr;
};

#endif // COMMUNICATIONJOB_H
