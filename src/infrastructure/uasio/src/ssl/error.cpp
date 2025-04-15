// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "error.h"

namespace uasio {
namespace ssl {

std::string error_category_impl::message(int value) const {
    switch (static_cast<error>(value)) {
        case error::success:
            return "成功";
        case error::invalid_argument:
            return "无效参数";
        case error::operation_aborted:
            return "操作已中止";
        case error::stream_truncated:
            return "SSL流被截断";
        case error::certificate_verification_failed:
            return "证书验证失败";
        case error::certificate_not_found:
            return "找不到证书";
        case error::protocol_error:
            return "SSL协议错误";
        case error::handshake_failed:
            return "SSL握手失败";
        case error::unspecified_ssl_error:
            return "未指定的SSL错误";
        case error::decryption_failed:
            return "解密失败";
        case error::expired_certificate:
            return "证书已过期";
        case error::revoked_certificate:
            return "证书已被吊销";
        case error::no_certificates:
            return "没有证书";
        case error::out_of_memory:
            return "内存不足";
        case error::client_hello_failed:
            return "ClientHello失败";
        case error::internal_error:
            return "SSL内部错误";
        case error::inappropriate_fallback:
            return "不适当的回退";
        case error::bad_certificate_status_response:
            return "错误的证书状态响应";
        case error::bad_certificate:
            return "无效的证书";
        case error::unsupported_certificate:
            return "不支持的证书";
        case error::certificate_expired:
            return "证书已过期";
        case error::unknown_ca:
            return "未知的证书颁发机构";
        case error::bad_record_mac:
            return "错误的记录MAC";
        case error::wrong_version_number:
            return "错误的版本号";
        case error::certificate_time_invalid:
            return "证书时间无效";
        default:
            return "未知的SSL错误";
    }
}

} // namespace ssl
} // namespace uasio 