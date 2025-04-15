// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "asio/tcp_resolver.h"

namespace NetUtil {
namespace Asio {

TCPResolver::TCPResolver(const std::shared_ptr<Service>& service)
    : _service(service),
      _io_service(_service->GetAsioService()),
      _strand(*_io_service),
      _strand_required(_service->IsStrandRequired()),
      _resolver(*_io_service)
{
    assert((service != nullptr) && "Asio service is invalid!");
    if (service == nullptr)
        throw BaseKit::ArgumentException("Asio service is invalid!");
}

} // namespace Asio
} // namespace NetUtil
