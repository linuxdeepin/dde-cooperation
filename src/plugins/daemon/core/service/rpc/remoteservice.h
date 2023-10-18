// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef REMOTE_SERVICE_H
#define REMOTE_SERVICE_H

#include <QObject>

#include "message.pb.h"

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

    void filetrans_update(::google::protobuf::RpcController* controller,
                         const ::FileTransUpdate* request,
                         ::FileTransResponse* response,
                         ::google::protobuf::Closure* done);

private:
};

class RemoteServiceBinder : public QObject
{
    Q_OBJECT
public:
    explicit RemoteServiceBinder(QObject *parent = nullptr);
    ~RemoteServiceBinder();

    void startRpcListen(const char *keypath, const char *crtpath);

    void createExecutor(const char *targetip, uint16_t port);

    void doLogin(const char *username, const char *pincode);

    void doQuery();

    void doMisc();

    // 通知远端准备执行作业：接收或发送。
    int doTransfileJob(const char *appname, int id, const char *jobpath, bool hidden, bool recursive, bool recv);

    // 发送文件数据信息。
    int doSendFileInfo(int jobid, int fileid, const char *subdir, const char *filepath);

    // 发送文件数据块。
    int doSendFileBlock(FileTransBlock fileblock);

    // 发送文件传输报告。
    int doUpdateTrans(FileTransUpdate update);

signals:
    void loginResult(bool result, QString who);
    void queryResult(bool result, QString msg);
    void miscResult(bool result, QString msg);
    void fileActionResult(bool result, int id);

    void fileTransResult(const char *path, int id, bool result);
    void fileTransSpeed(const char *path, int id, size_t speed);

public slots:

private:
    void *_executor_p {nullptr};
};

#endif   // REMOTE_SERVICE_H
