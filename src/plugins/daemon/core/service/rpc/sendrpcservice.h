// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENDRPCSERVICE_H
#define SENDRPCSERVICE_H

#include "service/rpc/remoteservice.h"

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QReadLocker>
#include <QThread>
#include <QSharedPointer>

#include "co/json.h"

class SendRpcWork : public QObject {
    Q_OBJECT
    friend class SendRpcService;
public:
    ~SendRpcWork();

Q_SIGNALS:
    void startPingTimer();
    void stopPingTimer();

    // 发送登陆
    void workDoLogin(const QString appName, const QString targetIp, const quint16 port,
                     const QString username, const QString pincode);
    void workDoQuery(const QString appName);
    //发到哪一个前端的自定义信息
    void workDoMisc(const QString appName, const QByteArray miscdata);
    // 通知远端准备执行作业：接收或发送。
    void workDoTransfileJob(const QString appName, int id, const QString jobpath,
                       bool hidden, bool recursive, bool recv);
    // 发送文件数据信息。
    void workDoSendFileInfo(const QString appName, int jobid, int fileid, const QString subdir, const QString filepath);
    // 发送文件数据块。
    void workDoSendFileBlock(const QString appName, FileTransBlock fileblock);
    // 发送文件传输报告。
    void workDoUpdateTrans(const QString appName, const int id, FileTransUpdate update);
    // 发送文件传输请求
    void workDoSendApplyTransFiles(const QString info);
    void workCreateRpcSender(const QString appName,
                         const QString targetip, quint16 port);
    void workSetTargetAppName(const QString appName, const QString targetAppName);
    void workRemovePing(const QString appName);

    void ping();

private:
    explicit SendRpcWork( QObject *parent = nullptr);
    void initConnect();

private slots:
    // 发送登陆
    void handleDoLogin(const QString appName, const QString targetIp, const quint16 port,
                 const QString username, const QString pincode);
    void handleDoQuery(const QString appName);
    //发到哪一个前端的自定义信息
    void handleDoMisc(const QString appName, const QByteArray msg);
    // 通知远端准备执行作业：接收或发送。
    void handleDoTransfileJob(const QString appName, int id, const QString obpath,
                             bool hidden, bool recursive, bool recv);
    // 发送文件数据信息。
    void handleDoSendFileInfo(const QString appName, int jobid, int fileid, const QString subdir, const QString filepath);
    // 发送文件数据块。
    void handleDoSendFileBlock(const QString appName, FileTransBlock fileblock);
    // 发送文件传输报告。
    void handleDoUpdateTrans(const QString appName, const int id, FileTransUpdate update);
    // 发送文件传输请求
    void handleDoSendApplyTransFiles(const QString param);
    void handleCreateRpcSender(const QString appName,
                         const QString targetip, quint16 port);
    void handleSetTargetAppName(const QString appName, const QString targetAppName);

    void handlePing();

private:
    QSharedPointer<RemoteServiceSender> createRpcSender(const QString &appName,
                                                        const QString &targetip, uint16_t port);
    QSharedPointer<RemoteServiceSender> rpcSender(const QString &appName);
    QString targetIp() const;
    quint16 targetPort() const;

private:
    QSharedPointer<RemoteServiceSender> _remote;
    QTimer _ping_timer;
    QStringList _ping_appname;
};

class SendRpcService : public QObject
{
    Q_OBJECT
Q_SIGNALS:
    void startPingTimer();
    void stopPingTimer();
    void createSenderWork(const QString appName,
                          const QString targetip, quint16 port);
public:
    struct ThreadInfo{
        SendRpcWork _work;
        QThread _thread;
    };

public slots:
    void handleStartTimer();
    void handleStopTimer();
    void handleTimeOut();

    void createRpcSenderWork(const QString appName,
                             const QString targetip, quint16 port);

public:
    ~SendRpcService() override;
    static SendRpcService *instance();
    // 发送登陆
    void doLogin(const QString &appName, const QString &targetIp, const quint16 port,
                     const QString &username, const QString &pincode) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoLogin(appName, targetIp, port, username, pincode);
    }

    void doQuery(const QString &appName) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoQuery(appName);
    }

    //发到哪一个前端的自定义信息
    void doMisc(const QString &appName, const char *miscdata) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoMisc(appName, miscdata); }

    // 通知远端准备执行作业：接收或发送。
    void doTransfileJob(const QString &appName, int id, const QString &jobpath,
                        bool hidden, bool recursive, bool recv) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoTransfileJob(appName, id, jobpath, hidden, recursive, recv);
    }

    // 发送文件数据信息。
    void doSendFileInfo(const QString &appName, int jobid, int fileid, const QString &subdir, const QString &filepath) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoSendFileInfo(appName, jobid, fileid, subdir, filepath);
    }

    // 发送文件数据块。
    void doSendFileBlock(const QString &appName, FileTransBlock fileblock) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoSendFileBlock(appName, fileblock);
    }

    // 发送文件传输报告。
    void doUpdateTrans(const QString &appName, const int id, FileTransUpdate update) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoUpdateTrans(appName, id, update);
    }

    // 发送文件传输请求
    void doSendApplyTransFiles(const QString &appName, co::Json info) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workDoSendApplyTransFiles(info.str().c_str());
    }

    void createRpcSender(const QString &appName,
                         const QString &targetip, quint16 port) {
        emit createSenderWork(appName, targetip, port);
    }


    void setTargetAppName(const QString &appName, const QString &targetAppName) {
        auto work = getWork(appName);
        if (work)
            emit work->_work.workSetTargetAppName(appName, targetAppName);
    }

    void removePing(const QString &appName);

    void addPing(const QString &appName);

private:
    explicit SendRpcService( QObject *parent = nullptr);
    QSharedPointer<ThreadInfo> getWork(const QString &appName);

    void initConnet();

private:
    QReadWriteLock _lock;
    QMap<QString, QSharedPointer<ThreadInfo>> _worksMap;
    QList<QSharedPointer<ThreadInfo>> _works;
    QReadWriteLock _ping_lock;
    QStringList _ping_appname;
    QTimer _ping_timer;
};

#endif // SENDRPCSERVICE_H
