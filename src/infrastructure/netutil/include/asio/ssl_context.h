// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETUTIL_ASIO_SSL_CONTEXT_H
#define NETUTIL_ASIO_SSL_CONTEXT_H

#include "service.h"

namespace NetUtil {
namespace Asio {

//! SSL context
/*!
    SSL context is used to handle and validate certificates in SSL clients and servers.

    Thread-safe.
*/
class SSLContext : public asio::ssl::context
{
public:
    using asio::ssl::context::context;

    SSLContext(const SSLContext&) = delete;
    SSLContext(SSLContext&&) = delete;
    ~SSLContext() = default;

    SSLContext& operator=(const SSLContext&) = delete;
    SSLContext& operator=(SSLContext&&) = delete;

    //! Configures the context to use system root certificates
    void set_root_certs();
};

} // namespace Asio
} // namespace NetUtil

#endif // NETUTIL_ASIO_SSL_CONTEXT_H
