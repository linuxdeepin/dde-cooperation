// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef REMOTE_SERVICE_H
#define REMOTE_SERVICE_H

#include <QObject>
#include <QReadWriteLock>
#include <QMap>

#include "message.pb.h"
#include "common/comonstruct.h"
#include "zrpc.h"
#include <ipc/proto/chan.h>

class RemoteServiceImpl : public RemoteService
{
public:
    RemoteServiceImpl() = default;
    ~RemoteServiceImpl() override = default;

    virtual void proto_msg(::google::protobuf::RpcController* controller,
                           const ::ProtoData* request,
                           ::ProtoData* response,
                           ::google::protobuf::Closure* done) override;

};

class ZRpcClientExecutor
{
public:
    ZRpcClientExecutor(const char *targetip, uint16_t port)
        : ip(targetip)
        , port(port)
    {
        _client = new zrpc_ns::ZRpcClient(targetip, port, true);
    }

    ~ZRpcClientExecutor() {
        if (_client) {
            _client->getControler()->Reset();
            delete _client;
        }
    }

    zrpc_ns::ZRpcChannel *chan() { return _client->getChannel(); }

    zrpc_ns::ZRpcController *control() { return _client->getControler(); }

    QString targetIP() { return ip; }

    uint16_t targetPort() { return port; }

private:
    zrpc_ns::ZRpcClient *_client{ nullptr };
    QString ip;
    uint16_t port;
};


class RemoteServiceSender : public QObject
{
    Q_OBJECT
public:
    explicit RemoteServiceSender(const QString &appname,
                                 const QString &ip, const uint16 port, QObject *parent = nullptr);
    ~RemoteServiceSender();

    SendResult doSendProtoMsg(const uint32 type, const QString &msg, const QByteArray &data);
    void createExecutor(const QString &session, const char *targetip, uint16_t port);
    void clearExecutor(const QString &appname);
    void remoteIP(const QString &session, QString *ip, uint16 *port);
    void setIpInfo(const QString &ip, const uint16 port);
    void setTargetAppName(const QString &targetApp);
    QString targetAppname() const { return  _tar_app_name;}
    QString remoteIP() const { return _target_ip; }
    uint16 remotePort() const {return _target_port; }
    void createExecutor();

private:
    QSharedPointer<ZRpcClientExecutor> executor(const QString &appname);

private:
    QString _tar_app_name;
    QString _app_name;
    QString _target_ip;
    uint16 _target_port;
};

class RemoteServiceBinder : public QObject {
    Q_OBJECT
public:
    explicit RemoteServiceBinder(QObject *parent = nullptr);
    ~RemoteServiceBinder() override = default;
    void startRpcListen(const char *keypath, const char *crtpath,
                        const std::function<void(int, const fastring &, const uint16)> &call = nullptr);
private:
    std::function<void(int, const fastring &, const uint16)> callback{ nullptr };
};

#endif   // REMOTE_SERVICE_H
