// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETUTIL_ASIO_TCP_RESOLVER_H
#define NETUTIL_ASIO_TCP_RESOLVER_H

#include "service.h"

namespace NetUtil {
namespace Asio {

//! TCP resolver
/*!
    TCP resolver is used to resolve DNS while connecting TCP/SSL clients.

    Thread-safe.
*/
class TCPResolver
{
public:
    //! Initialize resolver with a given Asio service
    /*!
        \param service - Asio service
    */
    TCPResolver(const std::shared_ptr<Service>& service);
    TCPResolver(const TCPResolver&) = delete;
    TCPResolver(TCPResolver&&) = delete;
    virtual ~TCPResolver() { Cancel(); }

    TCPResolver& operator=(const TCPResolver&) = delete;
    TCPResolver& operator=(TCPResolver&&) = delete;

    //! Get the Asio service
    std::shared_ptr<Service>& service() noexcept { return _service; }
    //! Get the Asio IO service
    std::shared_ptr<asio::io_service>& io_service() noexcept { return _io_service; }
    //! Get the Asio service strand for serialized handler execution
    asio::io_service::strand& strand() noexcept { return _strand; }
    //! Get the TCP resolver
    asio::ip::tcp::resolver& resolver() noexcept { return _resolver; }

    //! Cancel any asynchronous operations that are waiting on the resolver
    virtual void Cancel() { _resolver.cancel(); }

private:
    // Asio service
    std::shared_ptr<Service> _service;
    // Asio IO service
    std::shared_ptr<asio::io_service> _io_service;
    // Asio service strand for serialized handler execution
    asio::io_service::strand _strand;
    bool _strand_required;
    // TCP resolver
    asio::ip::tcp::resolver _resolver;
};

} // namespace Asio
} // namespace NetUtil

#endif // NETUTIL_ASIO_TCP_RESOLVER_H
