// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HANDLERPCSERVICE_H
#define HANDLERPCSERVICE_H

#include <QObject>
#include <QMap>
#include <QSharedPointer>
#include <QTimer>
#include <QReadWriteLock>

#include "co/co.h"
#include "co/json.h"

struct OutData;
class RemoteServiceBinder;
class HandleRpcService : public QObject
{
    Q_OBJECT
public:
    explicit HandleRpcService(QObject *parent = nullptr);
    ~HandleRpcService();

    void startRemoteServer();

    void handleRpcLogin(bool result,const QString &targetAppname,
                        const QString &appName, const QString &ip);
    bool handleRemoteApplyTransFile(co::Json &info);
    bool handleRemoteLogin(co::Json &info, co::chan<OutData> *resp_chan = nullptr);
    void handleRemoteDisc(co::Json &info);
    void handleRemoteFileBlock(co::Json &info, fastring data, co::chan<OutData> *resp_chan = nullptr);
    void handleRemoteReport(co::Json &info, co::chan<OutData> *resp_chan = nullptr);
    void handleRemoteJobCancel(co::Json &info, co::chan<OutData> *resp_chan = nullptr);
    void handleTransJob(co::Json &info, co::chan<OutData> *resp_chan = nullptr);
    void handleRemoteShareConnect(co::Json &info);
    void handleRemoteShareDisConnect(co::Json &info);
    void handleRemoteShareConnectReply(co::Json &info);
    void handleRemoteShareStart(co::Json &info);
    void handleRemoteShareStartRes(co::Json &info);
    void handleRemoteShareStop(co::Json &info);
    void handleRemoteDisConnectCb(co::Json &info);
    void handleRemotePing(co::Json &info);
    void handleRemoteDisApplyShareConnect(co::Json &info);
    // 检查51597和51599两个端口是否有连接阻塞，没有退出
    bool checkConnected();
    void handleRemoteSearchIp(co::Json &info, co::chan<OutData> *resp_chan = nullptr);
    void hanldeRemoteDiscover(co::Json &info, co::chan<OutData> *resp_chan = nullptr);

private:
    void handleOffline(const QString ip);
    void startRemoteServer(const quint16 port);

private Q_SLOTS:
    void handleTimeOut();
    void handleStartTimer();

signals:
    void remoteRequestJob(const QString info);
    void startTimer();

private:
    QSharedPointer<RemoteServiceBinder> _rpc{ nullptr };
    QSharedPointer<RemoteServiceBinder> _rpc_trans{ nullptr };
    QTimer _timeOut;
    QReadWriteLock _lock;
    QMap<QString, int> _ping_lost_count;
    QMap<QString, QString> _remote_ping_ips;
};

#endif // HANDLERPCSERVICE_H
