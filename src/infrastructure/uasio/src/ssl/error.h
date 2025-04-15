// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SSL_ERROR_H
#define UASIO_SSL_ERROR_H

#include "../error.h"
#include <string>
#include <system_error>

namespace uasio {
namespace ssl {

/**
 * @brief SSL错误代码枚举
 */
enum class error {
    // 无错误
    success = 0,
    
    // 一般错误
    invalid_argument,
    operation_aborted,
    
    // SSL特定错误
    stream_truncated,
    certificate_verification_failed,
    certificate_not_found,
    protocol_error,
    handshake_failed,
    unspecified_ssl_error,
    decryption_failed,
    expired_certificate,
    revoked_certificate,
    no_certificates,
    out_of_memory,
    client_hello_failed,
    internal_error,
    inappropriate_fallback,
    bad_certificate_status_response,
    bad_certificate,
    unsupported_certificate,
    certificate_expired,
    unknown_ca,
    bad_record_mac,
    wrong_version_number,
    certificate_time_invalid
};

/**
 * @brief SSL错误类别类
 */
class error_category_impl : public std::error_category {
public:
    /**
     * @brief 获取错误类别名称
     * @return 错误类别名称
     */
    const char* name() const noexcept override {
        return "uasio.ssl";
    }
    
    /**
     * @brief 获取错误消息
     * @param value 错误代码
     * @return 错误消息
     */
    std::string message(int value) const override;
};

/**
 * @brief 获取SSL错误类别单例
 * @return SSL错误类别的引用
 */
inline const std::error_category& ssl_category() {
    static error_category_impl instance;
    return instance;
}

/**
 * @brief 创建SSL错误代码
 * @param e SSL错误代码枚举值
 * @return 错误代码对象
 */
inline uasio::error_code make_error_code(error e) {
    return uasio::error_code(static_cast<int>(e), ssl_category());
}

} // namespace ssl
} // namespace uasio

#endif // UASIO_SSL_ERROR_H 