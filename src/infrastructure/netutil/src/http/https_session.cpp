// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "http/https_session.h"
#include "http/https_server.h"

namespace NetUtil {
namespace HTTP {

HTTPSSession::HTTPSSession(const std::shared_ptr<HTTPSServer>& server)
    : Asio::SSLSession(server),
      _cache(server->cache())
{
}

void HTTPSSession::onReceived(const void* buffer, size_t size)
{
    // Receive HTTP request header
    if (_request.IsPendingHeader())
    {
        if (_request.ReceiveHeader(buffer, size))
            onReceivedRequestHeader(_request);

        size = 0;
    }

    // Check for HTTP request error
    if (_request.error())
    {
        onReceivedRequestError(_request, "Invalid HTTP request!");
        _request.Clear();
        Disconnect();
        return;
    }

    // Receive HTTP request body
    if (_request.ReceiveBody(buffer, size))
    {
        onReceivedRequestInternal(_request);
        _request.Clear();
        return;
    }

    // Check for HTTP request error
    if (_request.error())
    {
        onReceivedRequestError(_request, "Invalid HTTP request!");
        _request.Clear();
        Disconnect();
        return;
    }
}

void HTTPSSession::onDisconnected()
{
    // Receive HTTP request body
    if (_request.IsPendingBody())
    {
        onReceivedRequestInternal(_request);
        _request.Clear();
        return;
    }
}

void HTTPSSession::onReceivedRequestInternal(const HTTPRequest& request)
{
    // Try to get the cached response
    if (request.method() == "GET")
    {
        std::string_view url = request.url();
        size_t index = url.find('?');
        auto response = cache().find(std::string((index == std::string_view::npos) ? url : url.substr(0, index)));
        if (response.first)
        {
            // Process the request with the cached response
            onReceivedCachedRequest(request, response.second);
            return;
        }
    }

    // Process the request
    onReceivedRequest(request);
}

} // namespace HTTP
} // namespace NetUtil
