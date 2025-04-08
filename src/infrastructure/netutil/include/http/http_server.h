// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETUTIL_HTTP_HTTP_SERVER_H
#define NETUTIL_HTTP_HTTP_SERVER_H

#include "http_session.h"

#include "cache/filecache.h"
#include "asio/tcp_server.h"

namespace NetUtil {
namespace HTTP {

//! HTTP server
/*!
    HTTP server is used to create HTTP Web server and
    communicate with clients using HTTP protocol.
    It allows to receive GET, POST, PUT, DELETE requests and
    send HTTP responses.

    Thread-safe.
*/
class HTTPServer : public Asio::TCPServer
{
public:
    using TCPServer::TCPServer;

    HTTPServer(const HTTPServer&) = delete;
    HTTPServer(HTTPServer&&) = delete;
    virtual ~HTTPServer() = default;

    HTTPServer& operator=(const HTTPServer&) = delete;
    HTTPServer& operator=(HTTPServer&&) = delete;

    //! Get the static content cache
    BaseKit::FileCache& cache() noexcept { return _cache; }
    const BaseKit::FileCache& cache() const noexcept { return _cache; }

    //! Add static content cache
    /*!
        \param path - Static content path
        \param prefix - Cache prefix (default is "/")
        \param timeout - Refresh cache timeout (default is 1 hour)
    */
    void AddStaticContent(const BaseKit::Path& path, const std::string& prefix = "/", const BaseKit::Timespan& timeout = BaseKit::Timespan::hours(1));
    //! Remove static content cache
    /*!
        \param path - Static content path
    */
    void RemoveStaticContent(const BaseKit::Path& path) { _cache.remove_path(path); }
    //! Clear static content cache
    void ClearStaticContent() { _cache.clear(); }

    //! Watchdog the static content cache
    void Watchdog(const BaseKit::UtcTimestamp& utc = BaseKit::UtcTimestamp()) { _cache.watchdog(utc); }

protected:
    std::shared_ptr<Asio::TCPSession> CreateSession(const std::shared_ptr<Asio::TCPServer>& server) override { return std::make_shared<HTTPSession>(std::dynamic_pointer_cast<HTTPServer>(server)); }

private:
    // Static content cache
    BaseKit::FileCache _cache;
};

/*! \example http_server.cpp HTTP server example */

} // namespace HTTP
} // namespace NetUtil

#endif // NETUTIL_HTTP_HTTP_SERVER_H
