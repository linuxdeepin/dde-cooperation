// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "http/https_server.h"

#include "string/format.h"

namespace NetUtil {
namespace HTTP {

void HTTPSServer::AddStaticContent(const BaseKit::Path& path, const std::string& prefix, const BaseKit::Timespan& timeout)
{
    auto hanlder = [](BaseKit::FileCache & cache, const std::string& key, const std::string& value, const BaseKit::Timespan& timespan)
    {
        auto response = HTTPResponse();
        response.SetBegin(200);
        response.SetContentType(BaseKit::Path(key).extension().string());
        response.SetHeader("Cache-Control", BaseKit::format("max-age={}", timespan.seconds()));
        response.SetBody(value);
        return cache.insert(key, response.cache(), timespan);
    };

    cache().insert_path(path, prefix, timeout, hanlder);
}

} // namespace HTTP
} // namespace NetUtil
