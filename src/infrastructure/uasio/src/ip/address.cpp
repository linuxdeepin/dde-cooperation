#include "address.h"
#include <arpa/inet.h>
#include <system_error>

namespace uasio {
namespace ip {

address address::from_string(const std::string& str, error_code& ec) {
    // 首先尝试解析为 IPv4 地址
    error_code v4_ec;
    address_v4 v4 = address_v4::from_string(str, v4_ec);
    if (!v4_ec) {
        ec = error_code();
        return address(v4);
    }

    // 如果不是有效的 IPv4 地址，尝试解析为 IPv6 地址
    error_code v6_ec;
    address_v6 v6 = address_v6::from_string(str, v6_ec);
    if (!v6_ec) {
        ec = error_code();
        return address(v6);
    }

    // 如果两者都不是，返回错误
    ec = std::make_error_code(std::errc::invalid_argument);
    return address();
}

address address::from_string(const std::string& str) {
    error_code ec;
    address addr = from_string(str, ec);
    if (ec) {
        throw system_error(ec);
    }
    return addr;
}

bool operator<(const address& a1, const address& a2) noexcept {
    // 如果地址类型不同，IPv4 地址小于 IPv6 地址
    if (a1.is_v4() && a2.is_v6())
        return true;
    if (a1.is_v6() && a2.is_v4())
        return false;

    // 如果两者都是 IPv4，则比较实际地址
    if (a1.is_v4() && a2.is_v4())
        return a1.to_v4() < a2.to_v4();

    // 如果两者都是 IPv6，则比较实际地址
    if (a1.is_v6() && a2.is_v6())
        return a1.to_v6() < a2.to_v6();

    return false;
}

} // namespace ip
} // namespace uasio 