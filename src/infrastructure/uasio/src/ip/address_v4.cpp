#include "address_v4.h"
#include <cstring>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace uasio {
namespace ip {

std::string address_v4::to_string() const {
    char buf[INET_ADDRSTRLEN];
    in_addr addr;
    addr.s_addr = htonl(addr_);
    
    if (inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN) == nullptr) {
        return "";
    }
    
    return buf;
}

address_v4 address_v4::from_string(const std::string& str, error_code& ec) noexcept {
    in_addr addr;
    
    // 尝试将字符串转换为IPv4地址
    #ifdef _WIN32
    int result = inet_pton(AF_INET, str.c_str(), &addr);
    #else
    int result = inet_pton(AF_INET, str.c_str(), &addr);
    #endif
    
    if (result <= 0) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return address_v4();
    }
    
    ec = error_code();
    return address_v4(ntohl(addr.s_addr));
}

address_v4 address_v4::from_string(const std::string& str) {
    error_code ec;
    address_v4 addr = from_string(str, ec);
    if (ec) {
        throw std::system_error(ec);
    }
    return addr;
}

} // namespace ip
} // namespace uasio 