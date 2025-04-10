// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datagram_socket.h"
#include "socket.h"
#include <cstring>
#include <system_error>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
#endif

namespace uasio {

datagram_socket::datagram_socket(io_context& io_context)
    : socket(io_context)
{
}

datagram_socket::datagram_socket(io_context& io_context, address_family family, error_code& ec)
    : socket(io_context)
{
    if (!open(family, ec)) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
    }
}

bool datagram_socket::open(address_family family, error_code& ec)
{
    return socket::open(family, socket_type::dgram, ec);
}

void datagram_socket::bind(const endpoint& endpoint, error_code& ec)
{
    if (!socket::bind(endpoint, ec)) {
        ec = std::make_error_code(std::errc::address_in_use);
    }
}

std::size_t datagram_socket::send(const void* data, std::size_t size, error_code& ec)
{
    return socket::send(data, size, ec);
}

void datagram_socket::async_send(const void* data, std::size_t size, 
                               std::function<void(const error_code&, std::size_t)>&& handler)
{
    socket::async_send(data, size, std::move(handler));
}

std::size_t datagram_socket::send_to(const void* data, std::size_t size,
                                   const endpoint& endpoint, error_code& ec)
{
    return socket::send_to(data, size, endpoint, ec);
}

void datagram_socket::async_send_to(const void* data, std::size_t size,
                                 const endpoint& endpoint,
                                 std::function<void(const error_code&, std::size_t)>&& handler)
{
    socket::async_send_to(data, size, endpoint, std::move(handler));
}

std::size_t datagram_socket::receive(void* data, std::size_t size, error_code& ec)
{
    return socket::receive(data, size, ec);
}

void datagram_socket::async_receive(void* data, std::size_t size,
                                 std::function<void(const error_code&, std::size_t)>&& handler)
{
    socket::async_receive(data, size, std::move(handler));
}

std::size_t datagram_socket::receive_from(void* data, std::size_t size,
                                       endpoint& endpoint, error_code& ec)
{
    return socket::receive_from(data, size, endpoint, ec);
}

void datagram_socket::async_receive_from(void* data, std::size_t size,
                                      endpoint& endpoint,
                                      std::function<void(const error_code&, std::size_t)>&& handler)
{
    socket::async_receive_from(data, size, endpoint, std::move(handler));
}

endpoint datagram_socket::local_endpoint(error_code& ec) const
{
    return socket::local_endpoint(ec);
}

endpoint datagram_socket::remote_endpoint(error_code& ec) const
{
    return socket::remote_endpoint(ec);
}

bool datagram_socket::set_option(int level, int option_name,
                              const void* option_value, std::size_t option_size,
                              error_code& ec)
{
    return socket::setsockopt(level, option_name, option_value, option_size, ec);
}

bool datagram_socket::set_broadcast(bool value, error_code& ec)
{
    int option = value ? 1 : 0;
    return set_option(SOL_SOCKET, SO_BROADCAST, &option, sizeof(option), ec);
}

bool datagram_socket::set_receive_buffer_size(int size, error_code& ec)
{
    return set_option(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size), ec);
}

bool datagram_socket::set_send_buffer_size(int size, error_code& ec)
{
    return set_option(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size), ec);
}

} // namespace uasio 