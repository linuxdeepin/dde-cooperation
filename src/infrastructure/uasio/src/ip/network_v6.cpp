#include "network_v6.h"
#include <stdexcept>
#include <system_error>

namespace uasio {
namespace ip {

network_v6 network_v6::from_string(const std::string& str, error_code& ec) {
    std::string::size_type pos = str.find('/');
    if (pos == std::string::npos) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return network_v6();
    }

    address_v6 addr = address_v6::from_string(str.substr(0, pos), ec);
    if (ec) {
        return network_v6();
    }

    int prefix_len = 0;
    try {
        prefix_len = std::stoi(str.substr(pos + 1));
    } catch (const std::exception&) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return network_v6();
    }

    if (prefix_len < 0 || prefix_len > 128) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return network_v6();
    }

    ec.clear();
    return network_v6(addr, prefix_len);
}

network_v6 network_v6::from_string(const std::string& str) {
    error_code ec;
    network_v6 net = from_string(str, ec);
    if (ec) {
        throw std::system_error(ec);
    }
    return net;
}

} // namespace ip
} // namespace uasio 