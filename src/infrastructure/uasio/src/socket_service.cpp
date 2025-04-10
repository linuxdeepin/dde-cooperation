// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "socket_service.h"
#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    #define SOCKET_ERROR (-1)
    #define SOCKET_WOULD_BLOCK WSAEWOULDBLOCK
    #define SOCKET_IN_PROGRESS WSAEINPROGRESS
    #define SOCKET_AGAIN WSAEWOULDBLOCK
    #define close_socket closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #define SOCKET_ERROR (-1)
    #define SOCKET_WOULD_BLOCK EWOULDBLOCK
    #define SOCKET_IN_PROGRESS EINPROGRESS
    #define SOCKET_AGAIN EAGAIN
    #define close_socket close
#endif

namespace uasio {

// 将地址族转换为系统定义
static int to_native_family(uasio::address_family family)
{
    return (family == uasio::address_family::ipv4) ? AF_INET : AF_INET6;
}

// 将套接字类型转换为系统定义
static int to_native_type(uasio::socket_type type)
{
    return (type == uasio::socket_type::stream) ? SOCK_STREAM : SOCK_DGRAM;
}

// 将端点转换为系统地址结构
static bool to_native_address(const uasio::endpoint& ep, struct sockaddr_storage& addr, socklen_t& addrlen)
{
    std::memset(&addr, 0, sizeof(addr));
    
    if (ep.address().family() == uasio::address_family::ipv4) {
        struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in*>(&addr);
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = htons(ep.port());
        
        std::string ip = ep.address().to_string();
        if (inet_pton(AF_INET, ip.c_str(), &addr_in->sin_addr) != 1) {
            return false;
        }
        
        addrlen = sizeof(struct sockaddr_in);
    } else {
        struct sockaddr_in6* addr_in6 = reinterpret_cast<struct sockaddr_in6*>(&addr);
        addr_in6->sin6_family = AF_INET6;
        addr_in6->sin6_port = htons(ep.port());
        
        std::string ip = ep.address().to_string();
        if (inet_pton(AF_INET6, ip.c_str(), &addr_in6->sin6_addr) != 1) {
            return false;
        }
        
        addrlen = sizeof(struct sockaddr_in6);
    }
    
    return true;
}

// 从系统地址结构转换为端点
static uasio::endpoint from_native_address(const struct sockaddr* addr, socklen_t addrlen)
{
    if (addr->sa_family == AF_INET) {
        const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(addr);
        
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr_in->sin_addr, ip, sizeof(ip));
        
        uasio::error_code ec;
        ip_address ip_addr = ip_address::from_string(ip, ec);
        if (ec) {
            return uasio::endpoint();
        }
        
        return uasio::endpoint(ip_addr, ntohs(addr_in->sin_port));
    } else if (addr->sa_family == AF_INET6) {
        const struct sockaddr_in6* addr_in6 = reinterpret_cast<const struct sockaddr_in6*>(addr);
        
        char ip[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip, sizeof(ip));
        
        uasio::error_code ec;
        ip_address ip_addr = ip_address::from_string(ip, ec);
        if (ec) {
            return uasio::endpoint();
        }
        
        return uasio::endpoint(ip_addr, ntohs(addr_in6->sin6_port));
    }
    
    return uasio::endpoint();
}

//
// socket_service 实现
//

socket_service::socket_service(io_context& io_context)
    : io_context_(io_context)
{
#ifdef _WIN32
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 初始化失败
        throw std::runtime_error("Failed to initialize Winsock");
    }
#endif
}

socket_service::~socket_service()
{
#ifdef _WIN32
    // 清理 Winsock
    WSACleanup();
#endif
}

bool socket_service::open(uasio::address_family family, uasio::socket_type type, int& socket_fd, uasio::error_code&  ec)
{
    // 创建套接字
    socket_fd = ::socket(to_native_family(family), to_native_type(type), 0);
    if (socket_fd == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::bad_address);
#endif
        return false;
    }
    
    // 设置为非阻塞模式
    if (!set_non_blocking(socket_fd, ec)) {
        close_socket(socket_fd, ec);
        socket_fd = -1;
        return false;
    }
    
    if (socket_fd == -1) {
        close_socket(socket_fd, ec);
        ec = uasio::make_error_code(uasio::error::bad_address);
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

void socket_service::close(int socket_fd, uasio::error_code&  ec)
{
    if (socket_fd == -1) {
        ec = uasio::error_code();
        return;
    }
    
    if (::close_socket(socket_fd) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
    } else {
        ec = uasio::error_code();
    }
}

bool socket_service::bind(int socket_fd, const uasio::endpoint& endpoint, uasio::error_code&  ec)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;
    
    if (!to_native_address(endpoint, addr, addrlen)) {
        ec = uasio::make_error_code(uasio::error::bad_address);
        return false;
    }
    
    if (::bind(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::access_denied);
#endif
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

bool socket_service::connect(int socket_fd, const uasio::endpoint& endpoint, uasio::error_code&  ec)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;
    
    if (!to_native_address(endpoint, addr, addrlen)) {
        ec = uasio::make_error_code(uasio::error::bad_address);
        return false;
    }
    
    int result = ::connect(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), addrlen);
    if (result == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == SOCKET_WOULD_BLOCK || error == SOCKET_IN_PROGRESS) {
            // 连接正在进行中，需要等待完成
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return false;
        }
        ec = uasio::make_error_code(error);
#else
        if (errno == SOCKET_WOULD_BLOCK || errno == SOCKET_IN_PROGRESS) {
            // 连接正在进行中，需要等待完成
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return false;
        }
        ec = uasio::make_error_code(uasio::error::connection_refused);
#endif
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

void socket_service::async_connect(int socket_fd, const uasio::endpoint& endpoint, 
                                 std::function<void(const error_code&)>&& handler)
{
    // 尝试连接
    uasio::error_code ec;
    if (connect(socket_fd, endpoint, ec)) {
        // 连接成功，立即调用处理器
        io_context_.post([handler = std::move(handler)]() {
            handler(uasio::error_code());
        });
        return;
    }
    
    if (ec.value() != error::operation_in_progress) {
        // 连接失败，立即调用处理器
        io_context_.post([handler = std::move(handler), ec]() {
            handler(ec);
        });
        return;
    }
    
    // 连接正在进行中，需要等待完成
    start_async_operation(socket_fd, /* 写事件 */ 2, std::move(handler));
}

std::size_t socket_service::send(int socket_fd, const void* data, std::size_t size, uasio::error_code&  ec)
{
    int result = ::send(socket_fd, reinterpret_cast<const char*>(data), size, 0);
    if (result == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == SOCKET_WOULD_BLOCK) {
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return 0;
        }
        ec = uasio::make_error_code(error);
#else
        if (errno == SOCKET_WOULD_BLOCK || errno == SOCKET_AGAIN) {
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return 0;
        }
        ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
        return 0;
    }
    
    ec = uasio::error_code();
    return static_cast<std::size_t>(result);
}

void socket_service::async_send(int socket_fd, const void* data, std::size_t size, 
                              std::function<void(const error_code&, std::size_t)>&& handler)
{
    // 尝试发送
    uasio::error_code ec;
    std::size_t bytes_sent = send(socket_fd, data, size, ec);
    if (ec.value() != error::operation_in_progress) {
        // 发送完成或发生错误，立即调用处理器
        io_context_.post([handler = std::move(handler), ec, bytes_sent]() {
            handler(ec, bytes_sent);
        });
        return;
    }
    
    // 发送正在进行中，需要等待完成
    // 为简化实现，这里将完整数据保存在 lambda 中
    auto wrapper = [data, size, handler = std::move(handler)](const uasio::error_code&  ec) mutable {
        if (ec) {
            handler(ec, 0);
            return;
        }
        
        uasio::error_code send_ec;
        std::size_t bytes_sent = ::send(
            /* socket_fd */ -1, // lambda 无法获取 socket_fd，需要在实际实现中改进
            reinterpret_cast<const char*>(data),
            size,
            0
        );
        
        if (bytes_sent == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            send_ec = uasio::make_error_code(error);
#else
            send_ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
            bytes_sent = 0;
        }
        
        handler(send_ec, bytes_sent);
    };
    
    start_async_operation(socket_fd, /* 写事件 */ 2, std::move(wrapper));
}

std::size_t socket_service::receive(int socket_fd, void* data, std::size_t max_size, uasio::error_code&  ec)
{
    int result = ::recv(socket_fd, reinterpret_cast<char*>(data), max_size, 0);
    if (result == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == SOCKET_WOULD_BLOCK) {
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return 0;
        }
        ec = uasio::make_error_code(error);
#else
        if (errno == SOCKET_WOULD_BLOCK || errno == SOCKET_AGAIN) {
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return 0;
        }
        ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
        return 0;
    } else if (result == 0) {
        // 连接已关闭
        ec = uasio::make_error_code(uasio::error::connection_closed);
        return 0;
    }
    
    ec = uasio::error_code();
    return static_cast<std::size_t>(result);
}

void socket_service::async_receive(int socket_fd, void* data, std::size_t max_size, 
                                 std::function<void(const error_code&, std::size_t)>&& handler)
{
    // 尝试接收
    uasio::error_code ec;
    std::size_t bytes_received = receive(socket_fd, data, max_size, ec);
    if (ec.value() != error::operation_in_progress) {
        // 接收完成或发生错误，立即调用处理器
        io_context_.post([handler = std::move(handler), ec, bytes_received]() {
            handler(ec, bytes_received);
        });
        return;
    }
    
    // 接收正在进行中，需要等待完成
    // 为简化实现，这里将完整数据保存在 lambda 中
    auto wrapper = [data, max_size, handler = std::move(handler)](const uasio::error_code&  ec) mutable {
        if (ec) {
            handler(ec, 0);
            return;
        }
        
        uasio::error_code recv_ec;
        std::size_t bytes_received = ::recv(
            /* socket_fd */ -1, // lambda 无法获取 socket_fd，需要在实际实现中改进
            reinterpret_cast<char*>(data),
            max_size,
            0
        );
        
        if (bytes_received == SOCKET_ERROR) {
#ifdef _WIN32
            int error = WSAGetLastError();
            recv_ec = uasio::make_error_code(error);
#else
            recv_ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
            bytes_received = 0;
        } else if (bytes_received == 0) {
            // 连接已关闭
            recv_ec = uasio::make_error_code(uasio::error::connection_closed);
        }
        
        handler(recv_ec, bytes_received);
    };
    
    start_async_operation(socket_fd, /* 读事件 */ 1, std::move(wrapper));
}

uasio::endpoint socket_service::local_endpoint(int socket_fd, uasio::error_code&  ec) const
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    
    if (::getsockname(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::not_connected);
#endif
        return uasio::endpoint();
    }
    
    ec = uasio::error_code();
    return from_native_address(reinterpret_cast<struct sockaddr*>(&addr), addrlen);
}

uasio::endpoint socket_service::remote_endpoint(int socket_fd, uasio::error_code&  ec) const
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    
    if (::getpeername(socket_fd, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::not_connected);
#endif
        return uasio::endpoint();
    }
    
    ec = uasio::error_code();
    return from_native_address(reinterpret_cast<struct sockaddr*>(&addr), addrlen);
}

bool socket_service::set_non_blocking(int socket_fd, uasio::error_code&  ec)
{
#ifdef _WIN32
    u_long mode = 1;
    if (::ioctlsocket(socket_fd, FIONBIO, &mode) != 0) {
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
        return false;
    }
#else
    int flags = ::fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        ec = uasio::make_error_code(uasio::error::access_denied);
        return false;
    }
    
    flags |= O_NONBLOCK;
    if (::fcntl(socket_fd, F_SETFL, flags) == -1) {
        ec = uasio::make_error_code(uasio::error::access_denied);
        return false;
    }
#endif
    
    ec = uasio::error_code();
    return true;
}

void socket_service::start_async_operation(int socket_fd, int events, 
                                        std::function<void(const error_code&)>&& handler)
{
    // 为简化实现，直接使用 post 模拟异步操作完成
    // 在实际实现中，应该使用 epoll/select/IOCP 等方式等待事件
    io_context_.post([handler = std::move(handler)]() {
        handler(uasio::error_code());
    });
}

bool socket_service::is_socket_valid(int socket_fd) const
{
    return socket_fd != -1;
}

//
// tcp_acceptor::acceptor_service 实现
//

tcp_acceptor::acceptor_service::acceptor_service(io_context& io_context)
    : io_context_(io_context)
{
}

tcp_acceptor::acceptor_service::~acceptor_service()
{
}

bool tcp_acceptor::acceptor_service::open(uasio::address_family family, int& acceptor_fd, uasio::error_code&  ec)
{
    // 创建套接字
    acceptor_fd = ::socket(to_native_family(family), SOCK_STREAM, 0);
    if (acceptor_fd == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::bad_address);
#endif
        return false;
    }
    
    // 设置为非阻塞模式
    if (!set_non_blocking(acceptor_fd, ec)) {
        close_socket(acceptor_fd, ec);
        acceptor_fd = -1;
        return false;
    }
    
    // 设置地址重用
#ifdef _WIN32
    BOOL option_value = TRUE;
    if (::setsockopt(acceptor_fd, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&option_value), sizeof(option_value)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
        close_socket(acceptor_fd, ec);
        acceptor_fd = -1;
        return false;
    }
#else
    int option_value = 1;
    if (::setsockopt(acceptor_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &option_value, sizeof(option_value)) == SOCKET_ERROR) {
        ec = uasio::make_error_code(uasio::error::access_denied);
        close_socket(acceptor_fd, ec);
        acceptor_fd = -1;
        return false;
    }
#endif
    
    if (acceptor_fd == -1) {
        close_socket(acceptor_fd, ec);
        ec = uasio::make_error_code(uasio::error::bad_address);
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

void tcp_acceptor::acceptor_service::close(int acceptor_fd, uasio::error_code&  ec)
{
    if (acceptor_fd == -1) {
        ec = uasio::error_code();
        return;
    }
    
    if (::close_socket(acceptor_fd) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
    } else {
        ec = uasio::error_code();
    }
}

bool tcp_acceptor::acceptor_service::bind(int acceptor_fd, const uasio::endpoint& endpoint, uasio::error_code&  ec)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;
    
    if (!to_native_address(endpoint, addr, addrlen)) {
        ec = uasio::make_error_code(uasio::error::bad_address);
        return false;
    }
    
    if (::bind(acceptor_fd, reinterpret_cast<struct sockaddr*>(&addr), addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::access_denied);
#endif
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

bool tcp_acceptor::acceptor_service::listen(int acceptor_fd, int backlog, uasio::error_code&  ec)
{
    if (::listen(acceptor_fd, backlog) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::access_denied);
#endif
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

bool tcp_acceptor::acceptor_service::accept(int acceptor_fd, socket& socket, uasio::error_code&  ec)
{
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);
    
    int client_fd = ::accept(acceptor_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &addrlen);
    if (client_fd == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == SOCKET_WOULD_BLOCK) {
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return false;
        }
        ec = uasio::make_error_code(error);
#else
        if (errno == SOCKET_WOULD_BLOCK || errno == SOCKET_AGAIN) {
            ec = uasio::make_error_code(uasio::error::operation_in_progress);
            return false;
        }
        ec = uasio::make_error_code(uasio::error::connection_aborted);
#endif
        return false;
    }
    
    // 首先关闭客户端 socket 中已存在的连接
    socket.close(ec);
    if (ec) {
        close_socket(client_fd, ec);
        return false;
    }
    
    // 打开 socket
    uasio::socket_type type = uasio::socket_type::stream;
    uasio::address_family family = (client_addr.ss_family == AF_INET) ? 
                          uasio::address_family::ipv4 : uasio::address_family::ipv6;
    
    // 在实际实现中，应该将已接受的套接字赋值给 socket 对象
    // 为简化，这里我们创建一个新的套接字并关闭已接受的套接字
    if (!socket.service_->open(family, type, socket.socket_fd_, ec)) {
        close_socket(client_fd, ec);
        return false;
    }
    
    // 设置为非阻塞模式
    if (!socket.service_->set_non_blocking(socket.socket_fd_, ec)) {
        close_socket(client_fd, ec);
        socket.close(ec);
        return false;
    }
    
    if (client_fd == -1) {
        close_socket(client_fd, ec);
        ec = uasio::make_error_code(uasio::error::bad_address);
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

void tcp_acceptor::acceptor_service::async_accept(int acceptor_fd, socket& socket, 
                                               std::function<void(const error_code&)>&& handler)
{
    // 尝试接受连接
    uasio::error_code ec;
    if (accept(acceptor_fd, socket, ec)) {
        // 接受成功，立即调用处理器
        io_context_.post([handler = std::move(handler)]() {
            handler(uasio::error_code());
        });
        return;
    }
    
    if (ec.value() != error::operation_in_progress) {
        // 接受失败，立即调用处理器
        io_context_.post([handler = std::move(handler), ec]() {
            handler(ec);
        });
        return;
    }
    
    // 接受正在进行中，需要等待完成
    start_async_operation(acceptor_fd, socket, std::move(handler));
}

uasio::endpoint tcp_acceptor::acceptor_service::local_endpoint(int acceptor_fd, uasio::error_code&  ec) const
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    
    if (::getsockname(acceptor_fd, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::not_connected);
#endif
        return uasio::endpoint();
    }
    
    ec = uasio::error_code();
    return from_native_address(reinterpret_cast<struct sockaddr*>(&addr), addrlen);
}

bool tcp_acceptor::acceptor_service::set_non_blocking(int acceptor_fd, uasio::error_code&  ec)
{
#ifdef _WIN32
    u_long mode = 1;
    if (::ioctlsocket(acceptor_fd, FIONBIO, &mode) != 0) {
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
        return false;
    }
#else
    int flags = ::fcntl(acceptor_fd, F_GETFL, 0);
    if (flags == -1) {
        ec = uasio::make_error_code(uasio::error::access_denied);
        return false;
    }
    
    flags |= O_NONBLOCK;
    if (::fcntl(acceptor_fd, F_SETFL, flags) == -1) {
        ec = uasio::make_error_code(uasio::error::access_denied);
        return false;
    }
#endif
    
    ec = uasio::error_code();
    return true;
}

void tcp_acceptor::acceptor_service::start_async_operation(int acceptor_fd, socket& socket,
                                                        std::function<void(const error_code&)>&& handler)
{
    // 为简化实现，直接使用 post 模拟异步操作完成
    // 在实际实现中，应该使用 epoll/select/IOCP 等方式等待事件
    io_context_.post([this, acceptor_fd, &socket, handler = std::move(handler)]() mutable {
        uasio::error_code ec;
        if (accept(acceptor_fd, socket, ec)) {
            handler(uasio::error_code());
        } else {
            handler(ec);
        }
    });
}

bool tcp_acceptor::acceptor_service::is_acceptor_valid(int acceptor_fd) const
{
    return acceptor_fd != -1;
}

// 发送数据到指定端点
std::size_t socket_service::send_to(int socket_fd, const void* data, std::size_t size,
                                  const uasio::endpoint& endpoint, uasio::error_code&  ec)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;
    
    if (!to_native_address(endpoint, addr, addrlen)) {
        ec = uasio::make_error_code(uasio::error::bad_address);
        return 0;
    }
    
    ssize_t bytes_sent = ::sendto(socket_fd, 
                                static_cast<const char*>(data), 
                                size, 
                                0, 
                                reinterpret_cast<struct sockaddr*>(&addr), 
                                addrlen);
    
    if (bytes_sent == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == SOCKET_WOULD_BLOCK) {
            ec = uasio::make_error_code(uasio::error::try_again);
            return 0;
        }
        ec = uasio::make_error_code(error);
#else
        if (errno == SOCKET_WOULD_BLOCK || errno == SOCKET_AGAIN) {
            ec = uasio::make_error_code(uasio::error::try_again);
            return 0;
        }
        ec = uasio::make_error_code(uasio::error::broken_pipe);
#endif
        return 0;
    }
    
    ec = uasio::error_code();
    return static_cast<std::size_t>(bytes_sent);
}

// 异步发送数据到指定端点
void socket_service::async_send_to(int socket_fd, const void* data, std::size_t size,
                                 const uasio::endpoint& endpoint,
                                 std::function<void(const error_code&, std::size_t)>&& handler)
{
    // 尝试立即发送
    uasio::error_code ec;
    std::size_t bytes_sent = send_to(socket_fd, data, size, endpoint, ec);
    
    if (ec == uasio::make_error_code(uasio::error::try_again)) {
        // 缓冲区已满，等待可写事件
        auto data_copy = data;
        auto size_copy = size;
        auto endpoint_copy = endpoint;
        auto handler_copy = std::move(handler);
        
        start_async_operation(socket_fd, FD_WRITE, [this, socket_fd, data_copy, size_copy, endpoint_copy, handler_copy](const uasio::error_code&  ec) mutable {
            if (ec) {
                handler_copy(ec, 0);
                return;
            }
            
            // 再次尝试发送
            uasio::error_code send_ec;
            std::size_t bytes_sent = send_to(socket_fd, data_copy, size_copy, endpoint_copy, send_ec);
            handler_copy(send_ec, bytes_sent);
        });
    } else {
        // 立即完成
        handler(ec, bytes_sent);
    }
}

// 从指定端点接收数据
std::size_t socket_service::receive_from(int socket_fd, void* data, std::size_t max_size,
                                       uasio::endpoint& endpoint, uasio::error_code&  ec)
{
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    
    ssize_t bytes_received = ::recvfrom(socket_fd, 
                                      static_cast<char*>(data), 
                                      max_size, 
                                      0,
                                      reinterpret_cast<struct sockaddr*>(&addr), 
                                      &addrlen);
    
    if (bytes_received == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        if (error == SOCKET_WOULD_BLOCK) {
            ec = uasio::make_error_code(uasio::error::try_again);
            return 0;
        }
        ec = uasio::make_error_code(error);
#else
        if (errno == SOCKET_WOULD_BLOCK || errno == SOCKET_AGAIN) {
            ec = uasio::make_error_code(uasio::error::try_again);
            return 0;
        }
        ec = uasio::make_error_code(uasio::error::broken_pipe);
#endif
        return 0;
    }
    
    if (bytes_received == 0) {
        // 对于 UDP，0 字节接收不一定表示连接关闭
        ec = uasio::error_code();
        return 0;
    }
    
    // 设置源端点
    endpoint = from_native_address(reinterpret_cast<struct sockaddr*>(&addr), addrlen);
    
    ec = uasio::error_code();
    return static_cast<std::size_t>(bytes_received);
}

// 异步从指定端点接收数据
void socket_service::async_receive_from(int socket_fd, void* data, std::size_t max_size,
                                      uasio::endpoint& endpoint,
                                      std::function<void(const error_code&, std::size_t)>&& handler)
{
    // 尝试立即接收
    uasio::error_code ec;
    std::size_t bytes_received = receive_from(socket_fd, data, max_size, endpoint, ec);
    
    if (ec == uasio::make_error_code(uasio::error::try_again)) {
        // 没有可用数据，等待可读事件
        auto data_copy = data;
        auto max_size_copy = max_size;
        auto endpoint_ptr = &endpoint;
        auto handler_copy = std::move(handler);
        
        start_async_operation(socket_fd, FD_READ, [this, socket_fd, data_copy, max_size_copy, endpoint_ptr, handler_copy](const uasio::error_code&  ec) mutable {
            if (ec) {
                handler_copy(ec, 0);
                return;
            }
            
            // 再次尝试接收
            uasio::error_code receive_ec;
            std::size_t bytes_received = receive_from(socket_fd, data_copy, max_size_copy, *endpoint_ptr, receive_ec);
            handler_copy(receive_ec, bytes_received);
        });
    } else {
        // 立即完成
        handler(ec, bytes_received);
    }
}

// 设置套接字选项
bool socket_service::setsockopt(int socket_fd, int level, int option_name,
                              const void* option_value, std::size_t option_size,
                              uasio::error_code&  ec)
{
    if (socket_fd == -1) {
        ec = uasio::make_error_code(uasio::error::bad_file_descriptor);
        return false;
    }
    
    if (::setsockopt(socket_fd, level, option_name, 
                   static_cast<const char*>(option_value), 
                   static_cast<socklen_t>(option_size)) == SOCKET_ERROR) {
#ifdef _WIN32
        int error = WSAGetLastError();
        ec = uasio::make_error_code(error);
#else
        ec = uasio::make_error_code(uasio::error::invalid_argument);
#endif
        return false;
    }
    
    ec = uasio::error_code();
    return true;
}

} // namespace uasio 