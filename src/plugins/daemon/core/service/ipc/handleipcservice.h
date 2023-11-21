// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HANDLEIPDSERVICE_H
#define HANDLEIPDSERVICE_H

#include <QObject>
#include <QMap>
#include "co/json.h"

class BackendService;
class HandleIpcService : public QObject
{
    Q_OBJECT
public:
    explicit HandleIpcService(QObject *parent = nullptr);
    ~HandleIpcService();

signals:
    void connectClosed(const quint16 port);
    // 使用这个信号必须是不需要等待客户端返回值的。
    void notifyConnect(const QString session, const QString ip, const QString pass);

public slots:
    void newTransSendJob(QString session, const QString targetSession, int jobId, QStringList paths, bool sub, QString savedir);
    void handleNodeRegister(bool unreg, const co::Json &info);
    void handleGetAllNodes(const QSharedPointer<BackendService> _backendIpcService);
    void handleBackApplyTransFiles(co::Json param);
    void handleConnectClosed(const quint16 port);
    void handleTryConnect(co::Json json);
    bool handleJobActions(const uint type, co::Json &msg);
private:
    void ipcServiceStart();
    void createIpcBackend(const quint16 port);
    void handleAllMsg(const QSharedPointer<BackendService> backend, const uint type, co::Json &msg);

    QString handlePing(const co::Json &msg);

private:
    // port backend
    QMap<uint16, QSharedPointer<BackendService>> _backendIpcServices;
    QMap<QString, QString> _sessionIDs;
    // <appName, ip>
    QMap<QString, QString> _ips;
};

#endif // HANDLEIPDSERVICE_H
