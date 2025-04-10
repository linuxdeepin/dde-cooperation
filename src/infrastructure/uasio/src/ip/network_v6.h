#ifndef UASIO_IP_NETWORK_V6_H
#define UASIO_IP_NETWORK_V6_H

#include "address_v6.h"
#include <system_error>

namespace uasio {
namespace ip {

/**
 * @brief 表示 IPv6 地址范围的网络类
 */
class network_v6 {
public:
    /// 默认构造函数创建 ::/128 网络
    network_v6() noexcept
        : address_(), prefix_length_(128) {}

    /// 从地址和前缀长度构造
    network_v6(const address_v6& addr, int prefix_len)
        : address_(addr), prefix_length_(prefix_len) {
        if (prefix_len < 0 || prefix_len > 128) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument));
        }
    }

    /// 获取网络地址
    address_v6 address() const noexcept {
        return address_;
    }

    /// 获取网络前缀长度
    int prefix_length() const noexcept {
        return prefix_length_;
    }

    /// 获取网络掩码
    address_v6 netmask() const noexcept {
        address_v6::bytes_type bytes = {{0}};
        int wholeBytes = prefix_length_ / 8;
        int remainingBits = prefix_length_ % 8;

        // 设置完整的字节
        for (int i = 0; i < wholeBytes; ++i) {
            bytes[i] = 0xFF;
        }

        // 设置部分字节
        if (wholeBytes < 16 && remainingBits > 0) {
            bytes[wholeBytes] = (0xFF00 >> remainingBits) & 0xFF;
        }

        return address_v6(bytes);
    }

    /// 获取网络地址 (与掩码按位与)
    address_v6 network() const noexcept {
        address_v6::bytes_type addr_bytes = address_.to_bytes();
        address_v6::bytes_type mask_bytes = netmask().to_bytes();
        address_v6::bytes_type net_bytes;

        for (std::size_t i = 0; i < 16; ++i) {
            net_bytes[i] = addr_bytes[i] & mask_bytes[i];
        }

        return address_v6(net_bytes);
    }

    /// 获取广播地址
    address_v6 broadcast() const noexcept {
        address_v6::bytes_type addr_bytes = network().to_bytes();
        address_v6::bytes_type mask_bytes = netmask().to_bytes();
        address_v6::bytes_type bcast_bytes;

        for (std::size_t i = 0; i < 16; ++i) {
            bcast_bytes[i] = addr_bytes[i] | ~mask_bytes[i];
        }

        return address_v6(bcast_bytes);
    }

    /// 检查地址是否在网络范围内
    bool contains(const address_v6& addr) const noexcept {
        address_v6::bytes_type addr_bytes = addr.to_bytes();
        address_v6::bytes_type net_bytes = network().to_bytes();
        address_v6::bytes_type mask_bytes = netmask().to_bytes();

        for (std::size_t i = 0; i < 16; ++i) {
            if ((addr_bytes[i] & mask_bytes[i]) != net_bytes[i]) {
                return false;
            }
        }

        return true;
    }

    /// 从字符串创建网络 (格式: 地址/前缀长度)
    static network_v6 from_string(const std::string& str, error_code& ec);

    /// 从字符串创建网络 (可能抛出异常)
    static network_v6 from_string(const std::string& str);

    /// 相等比较运算符
    friend bool operator==(const network_v6& a, const network_v6& b) noexcept {
        return a.address_ == b.address_ && a.prefix_length_ == b.prefix_length_;
    }

    /// 不等比较运算符
    friend bool operator!=(const network_v6& a, const network_v6& b) noexcept {
        return !(a == b);
    }

private:
    address_v6 address_;
    int prefix_length_;
};

} // namespace ip
} // namespace uasio

#endif // UASIO_IP_NETWORK_V6_H 