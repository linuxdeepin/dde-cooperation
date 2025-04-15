// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace NetUtil {
namespace Asio {

template <class TOutputStream>
inline TOutputStream& operator<<(TOutputStream& stream, InternetProtocol protocol)
{
    switch (protocol)
    {
        case InternetProtocol::IPv4:
            stream << "IPv4";
            break;
        case InternetProtocol::IPv6:
            stream << "IPv6";
            break;
        default:
            stream << "<unknown>";
            break;
    }
    return stream;
}

} // namespace Asio
} // namespace NetUtil
