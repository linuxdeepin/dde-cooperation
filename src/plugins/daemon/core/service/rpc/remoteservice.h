// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef REMOTE_SERVICE_H
#define REMOTE_SERVICE_H

#include <QObject>
#include <QReadWriteLock>
#include <QMap>

#include "message.pb.h"
#include "zrpc.h"
#include <ipc/proto/chan.h>

class RemoteServiceImpl : public RemoteService
{
public:
    RemoteServiceImpl() = default;
    virtual ~RemoteServiceImpl() = default;

    virtual void login(::google::protobuf::RpcController *controller,
                       const ::LoginRequest *request,
                       ::LoginResponse *response,
                       ::google::protobuf::Closure *done);

    virtual void query_peerinfo(::google::protobuf::RpcController *controller,
                                const ::PeerInfo *request,
                                ::PeerInfo *response,
                                ::google::protobuf::Closure *done);

    virtual void misc(::google::protobuf::RpcController *controller,
                      const ::JsonMessage *request,
                      ::JsonMessage *response,
                      ::google::protobuf::Closure *done);

    virtual void fsaction(::google::protobuf::RpcController *controller,
                          const ::FileAction *request,
                          ::FileResponse *response,
                          ::google::protobuf::Closure *done);

    virtual void filetrans_job(::google::protobuf::RpcController *controller,
                               const ::FileTransJob *request,
                               ::FileTransResponse *response,
                               ::google::protobuf::Closure *done);

    virtual void filetrans_create(::google::protobuf::RpcController *controller,
                                  const ::FileTransCreate *request,
                                  ::FileTransResponse *response,
                                  ::google::protobuf::Closure *done);

    virtual void filetrans_block(::google::protobuf::RpcController *controller,
                                 const ::FileTransBlock *request,
                                 ::FileTransResponse *response,
                                 ::google::protobuf::Closure *done);

    virtual void filetrans_update(::google::protobuf::RpcController *controller,
                                  const ::FileTransUpdate *request,
                                  ::FileTransResponse *response,
                                  ::google::protobuf::Closure *done);
    virtual void apply_trans_files(::google::protobuf::RpcController* controller,
                                   const ::ApplyTransFilesRequest* request,
                                   ::ApplyTransFilesResponse* response,
                                   ::google::protobuf::Closure* done);

private:
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

class RemoteServiceBinder : public QObject
{
    Q_OBJECT
public:
    explicit RemoteServiceBinder(QObject *parent = nullptr);
    ~RemoteServiceBinder();

    void startRpcListen(const char *keypath, const char *crtpath,
                        const std::function<void(int, const fastring &, const uint16)> &call = nullptr);

    void createExecutor(const QString &session, const char *targetip, uint16_t port);

    co::Json doLogin(const QString &session, const char *username, const char *pincode);

    void doQuery(const QString &session);

    //发到哪一个前端的自定义信息
    QString doMisc(const char *session, const char *miscdata);

    // 通知远端准备执行作业：接收或发送。
    int doTransfileJob(const char *session, int id, const char *jobpath, bool hidden, bool recursive, bool recv);

    // 发送文件数据信息。
    int doSendFileInfo(const QString &session, int jobid, int fileid, const char *subdir, const char *filepath);

    // 发送文件数据块。
    int doSendFileBlock(const QString &session, FileTransBlock fileblock);

    // 发送文件传输报告。
    int doUpdateTrans(const QString &session, FileTransUpdate update);

    // 发送文件传输请求
    int doSendApplyTransFiles(const QString &session, const QString &info);

    void clearExecutor(const QString &appname);

    void remoteIP(const QString &session, QString *ip, uint16 *port);

signals:
    void loginResult(bool result, QString who);
    void queryResult(bool result, QString msg);
    void miscResult(bool result, QString msg);
    void fileActionResult(bool result, int id);

    void fileTransResult(const char *path, int id, bool result);
    void fileTransSpeed(const char *path, int id, size_t speed);

public slots:

private:
    QSharedPointer<ZRpcClientExecutor> executor(const QString &appname);

private:
    std::function<void(int, const fastring &, const uint16)> callback{ nullptr };
};

class RemoteServiceSender : public RemoteServiceBinder {
    Q_OBJECT
public:
    explicit RemoteServiceSender(const QString &appname, const QString &ip, const uint16 port, QObject *parent = nullptr);
    ~RemoteServiceSender();

    void setIpInfo(const QString &ip, const uint16 port);
    void setTargetAppName(const QString &targetApp);
    QString remoteIP() const { return _target_ip; }
    uint16 remotePort() const {return _target_port; }
    void createExecutor();

private:
    QString _tar_app_name;
    QString _app_name;
    QString _target_ip;
    uint16 _target_port;
};

#endif   // REMOTE_SERVICE_H
