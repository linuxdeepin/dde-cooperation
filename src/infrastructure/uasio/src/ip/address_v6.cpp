#include "address_v6.h"
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace uasio {
namespace ip {

bool address_v6::is_loopback() const noexcept {
    return ((addr_[0] == 0) && (addr_[1] == 0) && (addr_[2] == 0) && 
            (addr_[3] == 0) && (addr_[4] == 0) && (addr_[5] == 0) && 
            (addr_[6] == 0) && (addr_[7] == 0) && (addr_[8] == 0) && 
            (addr_[9] == 0) && (addr_[10] == 0) && (addr_[11] == 0) && 
            (addr_[12] == 0) && (addr_[13] == 0) && (addr_[14] == 0) && 
            (addr_[15] == 1));
}

bool address_v6::is_multicast() const noexcept {
    return addr_[0] == 0xFF; // 前缀为 FF 的地址是多播地址
}

bool address_v6::is_link_local() const noexcept {
    return ((addr_[0] == 0xFE) && ((addr_[1] & 0xC0) == 0x80));
}

bool address_v6::is_site_local() const noexcept {
    return ((addr_[0] == 0xFE) && ((addr_[1] & 0xC0) == 0xC0));
}

bool address_v6::is_v4_mapped() const noexcept {
    return ((addr_[0] == 0) && (addr_[1] == 0) && (addr_[2] == 0) && 
            (addr_[3] == 0) && (addr_[4] == 0) && (addr_[5] == 0) && 
            (addr_[6] == 0) && (addr_[7] == 0) && (addr_[8] == 0) && 
            (addr_[9] == 0) && (addr_[10] == 0xFF) && (addr_[11] == 0xFF));
}

std::string address_v6::to_string() const {
    char buf[INET6_ADDRSTRLEN];
    in6_addr addr;
    std::memcpy(&addr, addr_.data(), 16);
    
    if (inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN) == nullptr) {
        return "";
    }
    
    if (scope_id_ != 0) {
        std::string result(buf);
        result += '%';
        result += std::to_string(scope_id_);
        return result;
    }
    
    return buf;
}

address_v6 address_v6::from_string(const std::string& str, error_code& ec) {
    address_v6 result;
    unsigned long scope_id = 0;
    
    // 检查是否有接口ID（%后面的部分）
    std::string host = str;
    std::string::size_type pos = str.find('%');
    if (pos != std::string::npos) {
        host = str.substr(0, pos);
        try {
            scope_id = std::stoul(str.substr(pos + 1));
        } catch (const std::exception&) {
            ec = std::make_error_code(std::errc::invalid_argument);
            return result;
        }
    }

    in6_addr addr;
    if (inet_pton(AF_INET6, host.c_str(), &addr) != 1) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return result;
    }
    
    bytes_type bytes;
    std::memcpy(bytes.data(), &addr, 16);
    
    ec = error_code();
    return address_v6(bytes, scope_id);
}

address_v6 address_v6::from_string(const std::string& str) {
    error_code ec;
    address_v6 addr = from_string(str, ec);
    if (ec) {
        throw std::system_error(ec);
    }
    return addr;
}

bool operator<(const address_v6& a1, const address_v6& a2) noexcept {
    const address_v6::bytes_type& b1 = a1.to_bytes();
    const address_v6::bytes_type& b2 = a2.to_bytes();
    
    for (std::size_t i = 0; i < 16; ++i) {
        if (b1[i] < b2[i])
            return true;
        if (b1[i] > b2[i])
            return false;
    }
    
    return a1.scope_id() < a2.scope_id();
}

} // namespace ip
} // namespace uasio 