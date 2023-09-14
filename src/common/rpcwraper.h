// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RPCWRAPER_H
#define RPCWRAPER_H

#include <QObject>

#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/noncopyable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "tinyrpc/rpc_websocket_service.hpp"

#include "message.pb.h"

// namespace uniapi::rpc
// {

using namespace tinyrpc;

namespace net = boost::asio;
namespace sys = boost::system;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;


class RpcSession : public QObject
{
Q_OBJECT
public:
    using stream_type = boost::beast::websocket::stream<net::ip::tcp::socket>;
	explicit RpcSession(stream_type stream);

	~RpcSession();

	void run(net::yield_context yield);
    void bindrun(net::yield_context yield);

	void handleRequest(const uniapis::Message& req, uniapis::Message& reply);
    void reqProc(net::yield_context yield);

private:
    stream_type stream_;
	rpc_websocket_service<stream_type> rpc_stub_;
};

class RpcWraper : public QObject
{
    Q_OBJECT
public:
    using stream_type = websocket::stream<tcp::socket>;
    explicit RpcWraper(QObject *parent = nullptr);

    int start_server(char *host, char *port_str);
    int start_client(char *host, char *port_str);

signals:
    void notifyVersion(QString version);


public slots:

private:
    void doServerSession(tcp::socket& socket, net::yield_context yield);
    void doListen(net::io_context& ioc, tcp::endpoint endpoint, net::yield_context yield);
    void doClientSession(std::string const& host, std::string const& port,
	                 net::io_context& ioc, net::yield_context yield);

private:

};

// }

#endif // RPCWRAPER_H