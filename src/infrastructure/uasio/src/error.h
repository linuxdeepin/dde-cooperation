// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_ERROR_H
#define UASIO_ERROR_H

#include <string>
#include <system_error>

namespace uasio {

// 定义错误类别
class error_category_impl : public std::error_category {
public:
    const char* name() const noexcept override {
        return "uasio";
    }

    std::string message(int value) const override {
        switch (value) {
        case 1:
            return "操作已取消";
        case 2:
            return "连接已关闭";
        case 3:
            return "连接重置";
        case 4:
            return "连接中断";
        case 5:
            return "没有可用的缓冲区空间";
        case 6:
            return "已连接";
        case 7:
            return "未连接";
        case 8:
            return "连接被拒绝";
        case 9:
            return "网络不可达";
        case 10:
            return "主机不可达";
        case 11:
            return "访问被拒绝";
        case 12:
            return "错误的地址";
        case 13:
            return "操作正在进行中";
        case 14:
            return "错误的文件描述符";
        case 15:
            return "再次尝试";
        case 16:
            return "管道破裂";
        case 17:
            return "文件结束";
        case 18:
            return "无效参数";
        case 19:
            return "地址已在使用";
        case 20:
            return "操作将阻塞";
        case 21:
            return "内部错误";
        case 22:
            return "证书未找到";
        case 23:
            return "流被截断";
        case 24:
            return "证书验证失败";
        case 25:
            return "协议错误";
        case 26:
            return "握手失败";
        case 27:
            return "未指定的SSL错误";
        case 28:
            return "解密失败";
        default:
            return "未知错误";
        }
    }
};

// 获取 uasio 错误类别单例
inline const std::error_category& error_category() {
    static error_category_impl instance;
    return instance;
}

// 错误代码定义
using error_code = std::error_code;

// 创建一个 uasio 错误代码
inline error_code make_error_code(int e) {
    return error_code(e, error_category());
}

// 定义常见错误
namespace error {
    // 操作已取消
    static const int operation_aborted = 1;
    // 连接已关闭
    static const int connection_closed = 2;
    // 连接重置
    static const int connection_reset = 3;
    // 连接中断
    static const int connection_aborted = 4;
    // 没有可用的缓冲区空间
    static const int no_buffer_space = 5;
    // 已连接
    static const int already_connected = 6;
    // 未连接
    static const int not_connected = 7;
    // 连接被拒绝
    static const int connection_refused = 8;
    // 网络不可达
    static const int network_unreachable = 9;
    // 主机不可达
    static const int host_unreachable = 10;
    // 访问被拒绝
    static const int access_denied = 11;
    // 错误的地址
    static const int bad_address = 12;
    // 操作正在进行中
    static const int operation_in_progress = 13;
    // 错误的文件描述符
    static const int bad_file_descriptor = 14;
    // 再次尝试
    static const int try_again = 15;
    // 管道破裂
    static const int broken_pipe = 16;
    // 文件结束
    static const int eof = 17;
    // 无效参数
    static const int invalid_argument = 18;
    // 地址已在使用
    static const int address_in_use = 19;
    // 操作将阻塞
    static const int would_block = 20;
    // 内部错误
    static const int internal_error = 21;
    // 证书未找到
    static const int certificate_not_found = 22;
    // 流被截断
    static const int stream_truncated = 23;
    // 证书验证失败
    static const int certificate_verification_failed = 24;
    // 协议错误
    static const int protocol_error = 25;
    // 握手失败
    static const int handshake_failed = 26;
    // 未指定的SSL错误
    static const int unspecified_ssl_error = 27;
    // 解密失败
    static const int decryption_failed = 28;
}

} // namespace uasio

#endif // UASIO_ERROR_H 