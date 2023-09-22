// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef REMOTE_SERVICE_H
#define REMOTE_SERVICE_H

#include <QObject>

#include "message.pb.h"

namespace uniapis {

class RemoteServiceImpl : public RemoteService {
public:
    RemoteServiceImpl() = default;
    virtual ~RemoteServiceImpl() = default;

    void login(::google::protobuf::RpcController *controller,
               const ::uniapis::LoginRequest *request,
               ::uniapis::LoginResponse *response,
               ::google::protobuf::Closure *done);

    void query_peerinfo(::google::protobuf::RpcController *controller,
                        const ::uniapis::PeerInfo *request,
                        ::uniapis::PeerInfo *response,
                        ::google::protobuf::Closure *done);

    void misc(::google::protobuf::RpcController *controller,
              const ::uniapis::Misc *request,
              ::uniapis::Misc *response,
              ::google::protobuf::Closure *done);

    void fsaction(::google::protobuf::RpcController *controller,
                  const ::uniapis::FileAction *request,
                  ::uniapis::FileResponse *response,
                  ::google::protobuf::Closure *done);
};

} // namespace uniapis

class RemoteServiceBinder : public QObject {
    Q_OBJECT
public:
    explicit RemoteServiceBinder(QObject *parent = nullptr);
    ~RemoteServiceBinder();

signals:

public slots:

private:
};

#endif // REMOTE_SERVICE_H