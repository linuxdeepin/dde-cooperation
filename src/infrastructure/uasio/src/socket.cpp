// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "socket.h"
#include "socket_service.h"
#include <cstring>
#include <system_error>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

namespace uasio {

//
// ip_address 实现
//

ip_address::ip_address()
    : family_(address_family::ipv4)
{
    std::memset(&addr_, 0, sizeof(addr_));
}

ip_address::ip_address(address_family family)
    : family_(family)
{
    std::memset(&addr_, 0, sizeof(addr_));
}

ip_address ip_address::from_string(const std::string& address, error_code& ec)
{
    ip_address result;
    
    // 尝试解析为 IPv4 地址
    struct in_addr addr4;
    if (inet_pton(AF_INET, address.c_str(), &addr4) == 1) {
        result.family_ = address_family::ipv4;
        std::memcpy(result.addr_.ipv4_, &addr4, sizeof(addr4));
        ec = error_code();
        return result;
    }
    
    // 尝试解析为 IPv6 地址
    struct in6_addr addr6;
    if (inet_pton(AF_INET6, address.c_str(), &addr6) == 1) {
        result.family_ = address_family::ipv6;
        std::memcpy(result.addr_.ipv6_, &addr6, sizeof(addr6));
        ec = error_code();
        return result;
    }
    
    // 解析失败
    ec = std::make_error_code(std::errc::invalid_argument);
    return result;
}

ip_address ip_address::from_hostname(const std::string& hostname, error_code& ec)
{
    ip_address result;
    
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;  // 支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_STREAM;
    
    struct addrinfo* res = nullptr;
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    
    if (ret != 0) {
        ec = std::make_error_code(std::errc::address_not_available);
        return result;
    }
    
    // 使用第一个返回的地址
    if (res->ai_family == AF_INET) {
        result.family_ = address_family::ipv4;
        struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
        std::memcpy(result.addr_.ipv4_, &addr->sin_addr, sizeof(result.addr_.ipv4_));
    } else if (res->ai_family == AF_INET6) {
        result.family_ = address_family::ipv6;
        struct sockaddr_in6* addr = reinterpret_cast<struct sockaddr_in6*>(res->ai_addr);
        std::memcpy(result.addr_.ipv6_, &addr->sin6_addr, sizeof(result.addr_.ipv6_));
    } else {
        ec = std::make_error_code(std::errc::invalid_argument);
        freeaddrinfo(res);
        return result;
    }
    
    freeaddrinfo(res);
    ec = error_code();
    return result;
}

ip_address ip_address::local_host(error_code& ec)
{
    ip_address result;
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        ec = std::make_error_code(std::errc::address_not_available);
        return result;
    }
    
    return from_hostname(hostname, ec);
}

ip_address ip_address::any(address_family family)
{
    ip_address result(family);
    
    // 不需要特殊设置，默认全零就是任意地址
    
    return result;
}

ip_address ip_address::loopback(address_family family)
{
    ip_address result(family);
    
    if (family == address_family::ipv4) {
        struct in_addr addr;
        inet_pton(AF_INET, "127.0.0.1", &addr);
        std::memcpy(result.addr_.ipv4_, &addr, sizeof(result.addr_.ipv4_));
    } else {
        struct in6_addr addr;
        inet_pton(AF_INET6, "::1", &addr);
        std::memcpy(result.addr_.ipv6_, &addr, sizeof(result.addr_.ipv6_));
    }
    
    return result;
}

address_family ip_address::family() const
{
    return family_;
}

bool ip_address::is_ipv4() const
{
    return family_ == address_family::ipv4;
}

bool ip_address::is_ipv6() const
{
    return family_ == address_family::ipv6;
}

bool ip_address::is_loopback() const
{
    if (family_ == address_family::ipv4) {
        uint8_t loopback[4] = {127, 0, 0, 1};
        return std::memcmp(addr_.ipv4_, loopback, 4) == 0;
    } else {
        uint8_t loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        return std::memcmp(addr_.ipv6_, loopback, 16) == 0;
    }
}

bool ip_address::is_any() const
{
    if (family_ == address_family::ipv4) {
        uint8_t any[4] = {0, 0, 0, 0};
        return std::memcmp(addr_.ipv4_, any, 4) == 0;
    } else {
        uint8_t any[16] = {0};
        return std::memcmp(addr_.ipv6_, any, 16) == 0;
    }
}

std::string ip_address::to_string() const
{
    char buf[INET6_ADDRSTRLEN];
    
    if (family_ == address_family::ipv4) {
        struct in_addr addr;
        std::memcpy(&addr, addr_.ipv4_, sizeof(addr));
        inet_ntop(AF_INET, &addr, buf, sizeof(buf));
    } else {
        struct in6_addr addr;
        std::memcpy(&addr, addr_.ipv6_, sizeof(addr));
        inet_ntop(AF_INET6, &addr, buf, sizeof(buf));
    }
    
    return std::string(buf);
}

bool ip_address::operator==(const ip_address& other) const
{
    if (family_ != other.family_) {
        return false;
    }
    
    if (family_ == address_family::ipv4) {
        return std::memcmp(addr_.ipv4_, other.addr_.ipv4_, sizeof(addr_.ipv4_)) == 0;
    } else {
        return std::memcmp(addr_.ipv6_, other.addr_.ipv6_, sizeof(addr_.ipv6_)) == 0;
    }
}

bool ip_address::operator!=(const ip_address& other) const
{
    return !(*this == other);
}

//
// endpoint 实现
//

endpoint::endpoint()
    : port_(0)
{
}

endpoint::endpoint(const ip_address& addr, uint16_t port)
    : address_(addr)
    , port_(port)
{
}

const ip_address& endpoint::address() const
{
    return address_;
}

uint16_t endpoint::port() const
{
    return port_;
}

void endpoint::set_address(const ip_address& addr)
{
    address_ = addr;
}

void endpoint::set_port(uint16_t port)
{
    port_ = port;
}

std::string endpoint::to_string() const
{
    return address_.to_string() + ":" + std::to_string(port_);
}

//
// socket 实现
//

socket::socket(io_context& io_context)
    : io_context_(io_context), socket_fd_(-1), connected_(false)
{
    service_ = std::make_shared<socket_service>(io_context);
}

socket::socket(io_context& io_context, address_family family, socket_type type)
    : socket(io_context)
{
    error_code ec;
    open(family, type, ec);
}

socket::~socket()
{
    error_code ec;
    close(ec);
}

bool socket::is_open() const
{
    return socket_fd_ != -1;
}

bool socket::open(address_family family, socket_type type, error_code& ec)
{
    if (socket_fd_ != -1) {
        close(ec);
    }

    int domain = (family == address_family::ipv4) ? AF_INET : AF_INET6;
    int sock_type = (type == socket_type::stream) ? SOCK_STREAM : SOCK_DGRAM;
    socket_fd_ = ::socket(domain, sock_type, 0);

    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    // 设置非阻塞模式
    int flags = ::fcntl(socket_fd_, F_GETFL, 0);
    if (flags == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        close(ec);
        return false;
    }

    if (::fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        close(ec);
        return false;
    }

    ec.clear();
    return true;
}

void socket::close(error_code& ec)
{
    if (socket_fd_ != -1) {
        if (::close(socket_fd_) == -1) {
            ec = std::make_error_code(std::errc::bad_file_descriptor);
        } else {
            ec.clear();
        }
        socket_fd_ = -1;
        connected_ = false;
    }
}

bool socket::bind(const endpoint& endpoint, error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    sockaddr_storage addr;
    std::memset(&addr, 0, sizeof(addr));

    if (endpoint.address().is_ipv4()) {
        auto* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = htons(endpoint.port());
        inet_pton(AF_INET, endpoint.address().to_string().c_str(), &addr_in->sin_addr);
    } else {
        auto* addr_in6 = reinterpret_cast<sockaddr_in6*>(&addr);
        addr_in6->sin6_family = AF_INET6;
        addr_in6->sin6_port = htons(endpoint.port());
        inet_pton(AF_INET6, endpoint.address().to_string().c_str(), &addr_in6->sin6_addr);
    }

    if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr),
               endpoint.address().is_ipv4() ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) == -1) {
        ec = std::make_error_code(std::errc::address_in_use);
        return false;
    }

    ec.clear();
    return true;
}

bool socket::connect(const endpoint& endpoint, error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    sockaddr_storage addr;
    std::memset(&addr, 0, sizeof(addr));

    if (endpoint.address().is_ipv4()) {
        auto* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = htons(endpoint.port());
        inet_pton(AF_INET, endpoint.address().to_string().c_str(), &addr_in->sin_addr);
    } else {
        auto* addr_in6 = reinterpret_cast<sockaddr_in6*>(&addr);
        addr_in6->sin6_family = AF_INET6;
        addr_in6->sin6_port = htons(endpoint.port());
        inet_pton(AF_INET6, endpoint.address().to_string().c_str(), &addr_in6->sin6_addr);
    }

    if (::connect(socket_fd_, reinterpret_cast<sockaddr*>(&addr),
                 endpoint.address().is_ipv4() ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) == -1) {
        if (errno == EINPROGRESS) {
            ec.clear();
            connected_ = true;
            return true;
        }
        ec = std::make_error_code(std::errc::connection_refused);
        return false;
    }

    ec.clear();
    connected_ = true;
    return true;
}

void socket::async_connect(const endpoint& endpoint, std::function<void(const error_code&)>&& handler)
{
    if (service_) {
        service_->async_connect(socket_fd_, endpoint, std::move(handler));
    } else {
        handler(std::make_error_code(std::errc::bad_file_descriptor));
    }
}

std::size_t socket::send(const void* data, std::size_t size, error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    ssize_t bytes_sent = ::send(socket_fd_, data, size, 0);
    if (bytes_sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ec = std::make_error_code(std::errc::resource_unavailable_try_again);
        } else {
            ec = std::make_error_code(std::errc::broken_pipe);
        }
        return 0;
    }

    ec.clear();
    return static_cast<std::size_t>(bytes_sent);
}

std::size_t socket::receive(void* data, std::size_t size, error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    ssize_t bytes_received = ::recv(socket_fd_, data, size, 0);
    if (bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ec = std::make_error_code(std::errc::resource_unavailable_try_again);
        } else {
            ec = std::make_error_code(std::errc::broken_pipe);
        }
        return 0;
    }

    if (bytes_received == 0) {
        ec = std::make_error_code(std::errc::connection_reset);
        return 0;
    }

    ec.clear();
    return static_cast<std::size_t>(bytes_received);
}

void socket::async_send(const void* data, std::size_t size,
                      std::function<void(const error_code&, std::size_t)>&& handler)
{
    service_->async_send(socket_fd_, data, size, std::move(handler));
}

void socket::async_receive(void* data, std::size_t size,
                         std::function<void(const error_code&, std::size_t)>&& handler)
{
    service_->async_receive(socket_fd_, data, size, std::move(handler));
}

std::size_t socket::send_to(const void* data, std::size_t size,
                          const endpoint& endpoint, error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    sockaddr_storage addr;
    std::memset(&addr, 0, sizeof(addr));

    if (endpoint.address().is_ipv4()) {
        auto* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = htons(endpoint.port());
        inet_pton(AF_INET, endpoint.address().to_string().c_str(), &addr_in->sin_addr);
    } else {
        auto* addr_in6 = reinterpret_cast<sockaddr_in6*>(&addr);
        addr_in6->sin6_family = AF_INET6;
        addr_in6->sin6_port = htons(endpoint.port());
        inet_pton(AF_INET6, endpoint.address().to_string().c_str(), &addr_in6->sin6_addr);
    }

    ssize_t bytes_sent = ::sendto(socket_fd_, data, size, 0,
                                reinterpret_cast<sockaddr*>(&addr),
                                endpoint.address().is_ipv4() ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

    if (bytes_sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ec = std::make_error_code(std::errc::resource_unavailable_try_again);
        } else {
            ec = std::make_error_code(std::errc::broken_pipe);
        }
        return 0;
    }

    ec.clear();
    return static_cast<std::size_t>(bytes_sent);
}

std::size_t socket::receive_from(void* data, std::size_t size,
                               endpoint& endpoint, error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    sockaddr_storage addr;
    std::memset(&addr, 0, sizeof(addr));
    socklen_t addr_len = sizeof(addr);

    ssize_t bytes_received = ::recvfrom(socket_fd_, data, size, 0,
                                      reinterpret_cast<sockaddr*>(&addr), &addr_len);

    if (bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ec = std::make_error_code(std::errc::resource_unavailable_try_again);
        } else {
            ec = std::make_error_code(std::errc::broken_pipe);
        }
        return 0;
    }

    if (bytes_received == 0) {
        ec = std::make_error_code(std::errc::connection_reset);
        return 0;
    }

    char ip[INET6_ADDRSTRLEN];
    uint16_t port;

    if (addr.ss_family == AF_INET) {
        auto* addr_in = reinterpret_cast<sockaddr_in*>(&addr);
        inet_ntop(AF_INET, &addr_in->sin_addr, ip, sizeof(ip));
        port = ntohs(addr_in->sin_port);
        endpoint = uasio::endpoint(ip_address::from_string(ip, ec), port);
    } else {
        auto* addr_in6 = reinterpret_cast<sockaddr_in6*>(&addr);
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip, sizeof(ip));
        port = ntohs(addr_in6->sin6_port);
        endpoint = uasio::endpoint(ip_address::from_string(ip, ec), port);
    }

    if (ec) {
        return 0;
    }

    ec.clear();
    return static_cast<std::size_t>(bytes_received);
}

void socket::async_send_to(const void* data, std::size_t size,
                         const endpoint& endpoint,
                         std::function<void(const error_code&, std::size_t)>&& handler)
{
    service_->async_send_to(socket_fd_, data, size, endpoint, std::move(handler));
}

void socket::async_receive_from(void* data, std::size_t size,
                             endpoint& endpoint,
                             std::function<void(const error_code&, std::size_t)>&& handler)
{
    service_->async_receive_from(socket_fd_, data, size, endpoint, std::move(handler));
}

bool socket::setsockopt(int level, int option_name,
                      const void* option_value, std::size_t option_size,
                      error_code& ec)
{
    if (socket_fd_ == -1) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return false;
    }

    if (::setsockopt(socket_fd_, level, option_name,
                     option_value, static_cast<socklen_t>(option_size)) == -1) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return false;
    }

    ec.clear();
    return true;
}

endpoint socket::local_endpoint(error_code& ec) const
{
    if (!is_open()) {
        ec = std::make_error_code(std::errc::not_connected);
        return endpoint();
    }
    
    if (service_) {
        return service_->local_endpoint(socket_fd_, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return endpoint();
    }
}

endpoint socket::remote_endpoint(error_code& ec) const
{
    if (!is_open() || !connected_) {
        ec = std::make_error_code(std::errc::not_connected);
        return endpoint();
    }
    
    if (service_) {
        return service_->remote_endpoint(socket_fd_, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return endpoint();
    }
}

//
// tcp_acceptor 实现
//

tcp_acceptor::tcp_acceptor(io_context& io_ctx)
    : service_(std::make_shared<acceptor_service>(io_ctx))
    , acceptor_fd_(-1)
{
}

tcp_acceptor::~tcp_acceptor()
{
    error_code ec;
    close(ec);
}

bool tcp_acceptor::is_open() const
{
    return acceptor_fd_ != -1;
}

bool tcp_acceptor::open(address_family family, error_code& ec)
{
    if (is_open()) {
        close(ec);
        if (ec) {
            return false;
        }
    }
    
    if (service_) {
        return service_->open(family, acceptor_fd_, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
}

void tcp_acceptor::close(error_code& ec)
{
    if (!is_open()) {
        ec = error_code();
        return;
    }
    
    if (service_) {
        service_->close(acceptor_fd_, ec);
        acceptor_fd_ = -1;
    } else {
        ec = std::make_error_code(std::errc::not_connected);
    }
}

bool tcp_acceptor::bind(const endpoint& endpoint, error_code& ec)
{
    if (!is_open()) {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
    
    if (service_) {
        return service_->bind(acceptor_fd_, endpoint, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
}

bool tcp_acceptor::listen(int backlog, error_code& ec)
{
    if (!is_open()) {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
    
    if (service_) {
        return service_->listen(acceptor_fd_, backlog, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
}

bool tcp_acceptor::accept(socket& socket, error_code& ec)
{
    if (!is_open()) {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
    
    if (service_) {
        return service_->accept(acceptor_fd_, socket, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return false;
    }
}

void tcp_acceptor::async_accept(socket& socket, std::function<void(const error_code&)>&& handler)
{
    if (!is_open()) {
        handler(std::make_error_code(std::errc::not_connected));
        return;
    }
    
    if (service_) {
        service_->async_accept(acceptor_fd_, socket, std::move(handler));
    } else {
        handler(std::make_error_code(std::errc::not_connected));
    }
}

endpoint tcp_acceptor::local_endpoint(error_code& ec) const
{
    if (!is_open()) {
        ec = std::make_error_code(std::errc::not_connected);
        return endpoint();
    }
    
    if (service_) {
        return service_->local_endpoint(acceptor_fd_, ec);
    } else {
        ec = std::make_error_code(std::errc::not_connected);
        return endpoint();
    }
}

} // namespace uasio 