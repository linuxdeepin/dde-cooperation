#ifndef UASIO_IP_ADDRESS_H
#define UASIO_IP_ADDRESS_H

#include <string>
#include <variant>
#include <system_error>
#include "../error.h"
#include "address_v4.h"
#include "address_v6.h"

namespace uasio {
namespace ip {

// 使用uasio命名空间中的error_code
using error_code = std::error_code;
using system_error = std::system_error;

/**
 * @brief 通用 IP 地址类，可以同时表示 IPv4 和 IPv6 地址
 */
class address {
public:
    /// 默认构造函数创建未指定的地址
    address() noexcept : addr_(address_v4()) {}

    /// 从 IPv4 地址构造
    address(const address_v4& ipv4_address) noexcept
        : addr_(ipv4_address) {}

    /// 从 IPv6 地址构造
    address(const address_v6& ipv6_address) noexcept
        : addr_(ipv6_address) {}

    /// 检查地址是否为 IPv4
    bool is_v4() const noexcept {
        return addr_.index() == 0;
    }

    /// 检查地址是否为 IPv6
    bool is_v6() const noexcept {
        return addr_.index() == 1;
    }

    /// 获取 IPv4 地址 (如果是 IPv4)
    address_v4 to_v4() const {
        if (!is_v4()) {
            throw system_error(std::make_error_code(std::errc::address_family_not_supported));
        }
        return std::get<0>(addr_);
    }

    /// 获取 IPv6 地址 (如果是 IPv6)
    address_v6 to_v6() const {
        if (!is_v6()) {
            throw system_error(std::make_error_code(std::errc::address_family_not_supported));
        }
        return std::get<1>(addr_);
    }

    /// 检查是否为回环地址
    bool is_loopback() const noexcept {
        if (is_v4())
            return to_v4().is_loopback();
        if (is_v6())
            return to_v6().is_loopback();
        return false;
    }

    /// 检查是否为多播地址
    bool is_multicast() const noexcept {
        if (is_v4())
            return to_v4().is_multicast();
        if (is_v6())
            return to_v6().is_multicast();
        return false;
    }

    /// 检查是否为单播地址
    bool is_unicast() const noexcept {
        return !is_multicast();
    }

    /// 转换为字符串
    std::string to_string() const {
        if (is_v4())
            return to_v4().to_string();
        if (is_v6())
            return to_v6().to_string();
        return "";
    }

    /// 从字符串创建 IP 地址
    static address from_string(const std::string& str, error_code& ec);

    /// 从字符串创建 IP 地址 (可能抛出异常)
    static address from_string(const std::string& str);

    /// 相等比较运算符
    friend bool operator==(const address& a1, const address& a2) noexcept {
        return a1.addr_ == a2.addr_;
    }

    /// 不等比较运算符
    friend bool operator!=(const address& a1, const address& a2) noexcept {
        return !(a1 == a2);
    }

    /// 小于比较运算符
    friend bool operator<(const address& a1, const address& a2) noexcept;

private:
    // 使用 std::variant 存储 IPv4 或 IPv6 地址
    std::variant<address_v4, address_v6> addr_;
};

} // namespace ip
} // namespace uasio

#endif // UASIO_IP_ADDRESS_H 