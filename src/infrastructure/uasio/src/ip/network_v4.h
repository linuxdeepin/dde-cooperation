#ifndef UASIO_IP_NETWORK_V4_H
#define UASIO_IP_NETWORK_V4_H

#include "address_v4.h"
#include <system_error>

namespace uasio {
namespace ip {

/**
 * @brief 表示IPv4网络的类
 */
class network_v4 {
public:
    /// 默认构造函数创建 0.0.0.0/0 网络
    network_v4() noexcept
        : address_(), prefix_length_(0) {}

    /// 从地址和前缀长度构造
    network_v4(const address_v4& addr, int prefix_len)
        : address_(addr), prefix_length_(prefix_len) {
        if (prefix_len < 0 || prefix_len > 32) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument));
        }
    }

    /// 获取网络地址
    address_v4 address() const noexcept {
        return address_;
    }

    /// 获取网络前缀长度
    int prefix_length() const noexcept {
        return prefix_length_;
    }

    /// 获取网络掩码
    address_v4 netmask() const noexcept {
        if (prefix_length_ == 0)
            return address_v4(0);
        if (prefix_length_ == 32)
            return address_v4(0xFFFFFFFF);
        
        return address_v4((0xFFFFFFFF << (32 - prefix_length_)) & 0xFFFFFFFF);
    }

    /// 获取网络地址（地址与掩码按位与）
    address_v4 network() const noexcept {
        return address_v4(address_.to_uint() & netmask().to_uint());
    }

    /// 获取广播地址
    address_v4 broadcast() const noexcept {
        return address_v4(network().to_uint() | ~netmask().to_uint());
    }

    /// 检查地址是否在网络范围内
    bool contains(const address_v4& addr) const noexcept {
        return (addr.to_uint() & netmask().to_uint()) == network().to_uint();
    }

    /// 从字符串创建网络 (格式: 地址/前缀长度)
    static network_v4 from_string(const std::string& str, error_code& ec);

    /// 从字符串创建网络 (可能抛出异常)
    static network_v4 from_string(const std::string& str);

    /// 相等比较运算符
    friend bool operator==(const network_v4& a, const network_v4& b) noexcept {
        return a.address_ == b.address_ && a.prefix_length_ == b.prefix_length_;
    }

    /// 不等比较运算符
    friend bool operator!=(const network_v4& a, const network_v4& b) noexcept {
        return !(a == b);
    }

private:
    address_v4 address_;
    int prefix_length_;
};

} // namespace ip
} // namespace uasio

#endif // UASIO_IP_NETWORK_V4_H 