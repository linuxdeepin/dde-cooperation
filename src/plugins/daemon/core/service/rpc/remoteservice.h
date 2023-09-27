// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef REMOTE_SERVICE_H
#define REMOTE_SERVICE_H

#include <QObject>

#include "message.pb.h"
#include "../fsadapter.h"

// namespace uniapis {

class RemoteServiceImpl : public RemoteService
{
public:
    RemoteServiceImpl() = default;
    virtual ~RemoteServiceImpl() = default;

    void login(::google::protobuf::RpcController *controller,
               const ::LoginRequest *request,
               ::LoginResponse *response,
               ::google::protobuf::Closure *done);

    void query_peerinfo(::google::protobuf::RpcController *controller,
                        const ::PeerInfo *request,
                        ::PeerInfo *response,
                        ::google::protobuf::Closure *done);

    void misc(::google::protobuf::RpcController *controller,
              const ::Misc *request,
              ::Misc *response,
              ::google::protobuf::Closure *done);

    void fsaction(::google::protobuf::RpcController *controller,
                  const ::FileAction *request,
                  ::FileResponse *response,
                  ::google::protobuf::Closure *done);

    void fsflow(::google::protobuf::RpcController *controller,
                const ::FileResponse *request,
                ::FileResponse *response,
                ::google::protobuf::Closure *done);

    void filetrans_job(::google::protobuf::RpcController *controller,
                       const ::FileTransJob *request,
                       ::FileTransResponse *response,
                       ::google::protobuf::Closure *done);

    void filetrans_create(::google::protobuf::RpcController *controller,
                          const ::FileTransCreate *request,
                          ::FileTransResponse *response,
                          ::google::protobuf::Closure *done);

    void filetrans_block(::google::protobuf::RpcController *controller,
                         const ::FileTransBlock *request,
                         ::FileTransResponse *response,
                         ::google::protobuf::Closure *done);

private:

};

// }   // namespace uniapis

class RemoteServiceBinder : public QObject
{
    Q_OBJECT
public:
    explicit RemoteServiceBinder(QObject *parent = nullptr);
    ~RemoteServiceBinder();

    void startRpcListen();

    void createExecutor(const char *targetip, int16_t port);

    void doLogin(const char *username, const char *pincode);

    void doQuery();

    void doMisc();

    void doFileAction(int type, const char *actionjson);

    int doFileFlow(int type, const char *flowjson, const void *bindata = nullptr, int binlen = 0);

    // 传输单个文件的作业，不断的调用RPC实现，包含文件传输的步骤。
    int doPushfileJob(int id, const char *filepath);

signals:
    void loginResult(bool result, const char *reason);
    void queryResult(bool result, const char *reason);
    void miscResult(bool result, const char *reason);
    void fileActionResult(bool result, int id);

    void fileTransResult(bool ok, const char *path, int id);
    void fileTransSpeed(const char *path, int id);

public slots:

private:
    void *_executor_p {nullptr};
};

#endif   // REMOTE_SERVICE_H
