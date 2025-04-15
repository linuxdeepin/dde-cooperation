#ifndef UASIO_IP_ADDRESS_V6_H
#define UASIO_IP_ADDRESS_V6_H

#include <array>
#include <string>
#include <vector>
#include <cstring>
#include "../error.h"

namespace uasio {
namespace ip {

/**
 * @brief 一个 IPv6 地址的封装类
 */
class address_v6 {
public:
    /// IPv6 地址的字节类型
    typedef std::array<unsigned char, 16> bytes_type;

    /// 默认构造函数
    address_v6() noexcept : addr_(), scope_id_(0) {}

    /// 从 bytes_type 构造
    explicit address_v6(const bytes_type& bytes, unsigned long scope_id = 0) noexcept
        : addr_(bytes), scope_id_(scope_id) {}

    /// 获取本地回环地址 ::1
    static address_v6 loopback() noexcept {
        bytes_type bytes = {{0}};
        bytes[15] = 1;
        return address_v6(bytes);
    }

    /// 获取任意地址 ::
    static address_v6 any() noexcept {
        bytes_type bytes = {{0}};
        return address_v6(bytes);
    }

    /// 获取地址字节
    bytes_type to_bytes() const noexcept {
        return addr_;
    }

    /// 获取范围 ID
    unsigned long scope_id() const noexcept {
        return scope_id_;
    }

    /// 设置范围 ID
    void scope_id(unsigned long id) noexcept {
        scope_id_ = id;
    }

    /// 检查是否为回环地址
    bool is_loopback() const noexcept;

    /// 检查是否为多播地址
    bool is_multicast() const noexcept;

    /// 检查是否为单播地址
    bool is_unicast() const noexcept {
        return !is_multicast();
    }

    /// 检查是否为链路本地地址
    bool is_link_local() const noexcept;

    /// 检查是否为站点本地地址
    bool is_site_local() const noexcept;

    /// 检查是否为 IPv4 映射地址
    bool is_v4_mapped() const noexcept;

    /// 转换为字符串
    std::string to_string() const;

    /// 从字符串创建 IPv6 地址
    static address_v6 from_string(const std::string& str, error_code& ec);

    /// 从字符串创建 IPv6 地址（可能抛出异常）
    static address_v6 from_string(const std::string& str);

    /// 相等比较运算符
    friend bool operator==(const address_v6& a1, const address_v6& a2) noexcept {
        return a1.addr_ == a2.addr_ && a1.scope_id_ == a2.scope_id_;
    }

    /// 不等比较运算符
    friend bool operator!=(const address_v6& a1, const address_v6& a2) noexcept {
        return !(a1 == a2);
    }

    /// 小于比较运算符（用于排序）
    friend bool operator<(const address_v6& a1, const address_v6& a2) noexcept;

private:
    // IPv6 地址存储
    bytes_type addr_;
    
    // 接口范围 ID
    unsigned long scope_id_;
};

} // namespace ip
} // namespace uasio

#endif // UASIO_IP_ADDRESS_V6_H 