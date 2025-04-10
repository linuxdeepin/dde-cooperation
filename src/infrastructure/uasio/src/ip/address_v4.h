#ifndef UASIO_IP_ADDRESS_V4_H
#define UASIO_IP_ADDRESS_V4_H

#include <array>
#include <string>
#include <cstdint>
#include "../error.h"

namespace uasio {
namespace ip {

/**
 * @brief IPv4地址类
 */
class address_v4 {
public:
    /// IPv4地址的字节类型
    typedef std::array<unsigned char, 4> bytes_type;

    /// IPv4地址的整数类型
    typedef uint_least32_t uint_type;

    /// 默认构造函数
    address_v4() noexcept : addr_(0) {}

    /// 从整数构造
    explicit address_v4(uint_type addr) noexcept 
        : addr_(addr) {}

    /// 从字节数组构造
    explicit address_v4(const bytes_type& bytes) noexcept {
        addr_ = (static_cast<uint_type>(bytes[0]) << 24)
              | (static_cast<uint_type>(bytes[1]) << 16)
              | (static_cast<uint_type>(bytes[2]) << 8)
              | static_cast<uint_type>(bytes[3]);
    }

    /// 获取任意地址 (0.0.0.0)
    static address_v4 any() noexcept {
        return address_v4();
    }

    /// 获取本地回环地址 (127.0.0.1)
    static address_v4 loopback() noexcept {
        return address_v4(0x7F000001);
    }

    /// 获取广播地址 (255.255.255.255)
    static address_v4 broadcast() noexcept {
        return address_v4(0xFFFFFFFF);
    }

    /// 检查是否为回环地址
    bool is_loopback() const noexcept {
        return (to_uint() & 0xFF000000) == 0x7F000000;
    }

    /// 检查是否为多播地址
    bool is_multicast() const noexcept {
        return (to_uint() & 0xF0000000) == 0xE0000000;
    }

    /// 检查是否为单播地址
    bool is_unicast() const noexcept {
        return !is_multicast() && to_uint() != 0xFFFFFFFF;
    }

    /// 将地址转换为整数
    uint_type to_uint() const noexcept {
        return addr_;
    }

    /// 将地址转换为字节数组
    bytes_type to_bytes() const noexcept {
        bytes_type bytes = {{
            static_cast<unsigned char>((addr_ >> 24) & 0xFF),
            static_cast<unsigned char>((addr_ >> 16) & 0xFF),
            static_cast<unsigned char>((addr_ >> 8) & 0xFF),
            static_cast<unsigned char>(addr_ & 0xFF)
        }};
        return bytes;
    }

    /// 将地址转换为字符串
    std::string to_string() const;

    /// 从字符串创建地址（可抛出异常）
    static address_v4 from_string(const std::string& str);

    /// 从字符串创建地址，带错误码
    static address_v4 from_string(const std::string& str, error_code& ec) noexcept;

    /// 相等比较运算符
    friend bool operator==(const address_v4& a1, const address_v4& a2) noexcept {
        return a1.addr_ == a2.addr_;
    }

    /// 不等比较运算符
    friend bool operator!=(const address_v4& a1, const address_v4& a2) noexcept {
        return a1.addr_ != a2.addr_;
    }

    /// 小于比较运算符（用于排序）
    friend bool operator<(const address_v4& a1, const address_v4& a2) noexcept {
        return a1.addr_ < a2.addr_;
    }

    /// 大于比较运算符
    friend bool operator>(const address_v4& a1, const address_v4& a2) noexcept {
        return a1.addr_ > a2.addr_;
    }

    /// 小于等于比较运算符
    friend bool operator<=(const address_v4& a1, const address_v4& a2) noexcept {
        return a1.addr_ <= a2.addr_;
    }

    /// 大于等于比较运算符
    friend bool operator>=(const address_v4& a1, const address_v4& a2) noexcept {
        return a1.addr_ >= a2.addr_;
    }

private:
    // IPv4地址的内部表示
    uint_type addr_;
};

} // namespace ip
} // namespace uasio

#endif // UASIO_IP_ADDRESS_V4_H 