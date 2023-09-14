// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rpcwraper.h"

void fail(boost::system::error_code ec, char const *what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

RpcSession::RpcSession(stream_type stream)
    : stream_(std::move(stream))
    , rpc_stub_(stream_)
{
}

RpcSession::~RpcSession()
{
    std::cout << "~session\n";
}

void RpcSession::run(net::yield_context yield) {
    boost::beast::multi_buffer buf;
    boost::system::error_code ec;

    while (true) {
        auto bytes = stream_.async_read(buf, yield[ec]);
        if (ec) return fail(ec, "async_read");
        rpc_stub_.dispatch(buf, ec);
        if (ec) return fail(ec, "dispatch");
        buf.consume(bytes);
    }
}

void RpcSession::bindrun(net::yield_context yield) {
    using request = uniapis::Message;
    using reply = uniapis::Message;
    rpc_stub_.rpc_bind<request, reply>(
        std::bind(&RpcSession::handleRequest, this, std::placeholders::_1, std::placeholders::_2));

    run(yield);
    // boost::beast::multi_buffer buf;
    // boost::system::error_code ec;

    // while (true) {
    //     auto bytes = stream_.async_read(buf, yield[ec]);
    //     if (ec) return fail(ec, "async_read");
    //     rpc_stub_.dispatch(buf, ec);
    //     if (ec) return fail(ec, "dispatch");
    //     buf.consume(bytes);
    // }
}

void RpcSession::handleRequest(const uniapis::Message &req, uniapis::Message &reply) {
    // std::cout << req.name() << " say: " << req.message() << std::endl;

    // reply.set_name("server");
    // reply.set_message(req.message() + " copy!");
	std::string ver = req.version();
	std::cout << " protocol version: " << ver << std::endl;

	// if (req.has_login_request()) {
	// 	std::cout << " login request: " << req.login_request().name() << std::endl;
    //     std::cout << " login request: " << req.login_request().auth() << std::endl;
	// } else if (req.has_peer_info()) {
	// 	std::cout << " peer info: " << req.peer_info().name() << std::endl;
    //     std::cout << " peer info: " << req.peer_info().ip() << std::endl;
    //     std::cout << " peer info: " << req.peer_info().port() << std::endl;
    //     std::cout << " peer info: " << req.peer_info().version() << std::endl;
	// } else if (req.has_file_action()) {
	// 	std::cout << " file action: " << req.file_action().name() << std::endl;
    //     std::cout << " file action: " << req.file_action().action() << std::endl;
    //     std::cout << " file action: " << req.file_action().path() << std::endl;
    //     std::cout << " file action: " << req.file_action().offset() << std::endl;
    //     std::cout << " file action: " << req.file_action().size() << std::endl;
    //     std::cout << " file action: " << req.file_action().data() << std::endl;
	// } else if (req.has_file_response()) {
	// 	std::cout << " file response: " << req.file_response().name() << std::endl;
    //     std::cout << " file response: " << req.file_response().action() << std::endl;
    //     std::cout << " file response: " << req.file_response().path() << std::endl;
    //     std::cout << " file response: " << req.file_response().offset() << std::endl;
    //     std::cout << " file response: " << req.file_response().size() << std::endl;
    //     std::cout << " file response: " << req.file_response().data() << std::endl;
    // } else if (req.has_login_response()) {
	// 	std::cout << " login response: " << req.login_response().name() << std::endl;
    //     std::cout << " login response: " << req.login_response().auth() << std::endl;
    // };
	uniapis::Message::UnionCase reqone = req.union_case();
	switch (reqone)
	{
		case uniapis::Message::UnionCase::kLoginRequest:
		{
			break;
		}
		case uniapis::Message::UnionCase::kLoginResponse:
		{
			break;
		}
		case uniapis::Message::UnionCase::kFileAction:
		{
			break;
		}
		case uniapis::Message::UnionCase::kFileResponse:
		{
			break;
		}
		case uniapis::Message::UnionCase::kMisc:
		{
			break;
		}
		case uniapis::Message::UnionCase::kPeerInfo:
		{
			break;
		}
		default: //UNION_NOT_SET
            break;
	}

	reply.set_version(ver);
}

void RpcSession::reqProc(net::yield_context yield) {
    uniapis::Message msg;
    msg.set_version("1.0"); // todo: set version from config

    std::cout << "input your name: ";
    std::string context;
    std::getline(std::cin, context);
    // msg.set_name(context);

    uniapis::Message reply;

    // while (true)
    // {
    //     std::cout << msg.name() << ": ";

    //     context.clear();
    //     std::getline(std::cin, context);

    //     msg.set_message(context);

    //     boost::system::error_code ec;
    //     rpc_stub_.async_call(msg, reply, yield[ec]);
    //     if (ec)
    //         return fail(ec, "async_call");

    //     std::cout << reply.name() << " reply: " << reply.message() << std::endl;
    // }
}

RpcWraper::RpcWraper(QObject *parent)
    : QObject(parent) {
}

void RpcWraper::doServerSession(tcp::socket &socket, net::yield_context yield) {
    boost::system::error_code ec;

    stream_type s{std::move(socket)};

    s.async_accept(yield[ec]);
    if (ec) return fail(ec, "accept");

    s.binary(true);

    // 完成websocket握手事宜之后开始进入rpc服务.
    // auto ses = std::make_shared<rpc_session>(std::move(s));
    auto session = new RpcSession(std::move(s));
    session->bindrun(yield);
}

void RpcWraper::doListen(net::io_context &ioc, tcp::endpoint endpoint, net::yield_context yield) {
    boost::system::error_code ec;

    tcp::acceptor acceptor(ioc);
    acceptor.open(endpoint.protocol(), ec);
    if (ec) return fail(ec, "open");

    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) return fail(ec, "set_option");

    acceptor.bind(endpoint, ec);
    if (ec) return fail(ec, "bind");

    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) return fail(ec, "listen");

    for (;;) {
        tcp::socket socket(ioc);
        acceptor.async_accept(socket, yield[ec]);
        if (ec) {
            fail(ec, "accept");
        } else {
            net::spawn(acceptor.get_executor(),
                       std::bind(&RpcWraper::doServerSession,
                                 this,
                                 std::move(socket),
                                 std::placeholders::_1));
        }
    }
}

void RpcWraper::doClientSession(std::string const &host,
                                std::string const &port,
                                net::io_context &ioc,
                                net::yield_context yield) {
    boost::system::error_code ec;

    tcp::resolver resolver{ioc};
    stream_type s{ioc};

    auto const results = resolver.async_resolve(host, port, yield[ec]);
    if (ec) return fail(ec, "resolve");

    net::async_connect(s.next_layer(), results.begin(), results.end(), yield[ec]);
    if (ec) return fail(ec, "connect");

    s.async_handshake(host, "/test", yield[ec]);
    if (ec) return fail(ec, "handshake");

    s.binary(true);

    tcp::socket ss(ioc);

    // 完成websocket握手事宜之后开始进入rpc服务.
    // auto ses = std::make_shared<rpc_session>(std::move(s));
    auto session = new RpcSession(std::move(s));
    net::spawn(ioc, std::bind(&RpcSession::run, session, std::placeholders::_1));
    session->reqProc(yield);
}

int RpcWraper::start_server(char *host, char *port_str) {
    if (host == NULL || port_str == NULL) {
        std::cerr << "error host or port\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(host);
    auto const port = static_cast<unsigned short>(std::atoi(port_str));

    net::io_context ioc;

    net::spawn(ioc,
               std::bind(&RpcWraper::doListen,
                         this,
                         std::ref(ioc),
                         tcp::endpoint{address, port},
                         std::placeholders::_1));

    ioc.run();

    return EXIT_SUCCESS;
}

int RpcWraper::start_client(char *host, char *port) {

    if (host == NULL || port == NULL) {
        std::cerr << "error host or port\n";
        return EXIT_FAILURE;
    }

    net::io_context ioc;

    net::spawn(ioc,
               std::bind(&RpcWraper::doClientSession,
                         this,
                         std::string(host),
                         std::string(port),
                         std::ref(ioc),
                         std::placeholders::_1));

    ioc.run();

    return EXIT_SUCCESS;
}
