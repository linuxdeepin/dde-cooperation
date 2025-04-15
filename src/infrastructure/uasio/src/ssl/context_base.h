#pragma once

namespace uasio {
namespace ssl {

// 文件格式枚举
enum class file_format {
    pem,  // PEM格式
    asn1  // ASN.1 (DER) 格式
};

// 验证模式枚举
enum class verify_mode {
    none = 0,                 // 不验证
    peer = 1,                 // 验证对端
    fail_if_no_peer_cert = 2, // 对端无证书失败
    client_once = 4           // 仅验证客户端一次
};

// SSL方法枚举
enum class method {
    // 客户端方法
    tls_client,          // TLS客户端（推荐用于客户端）
    sslv2_client,        // SSLv2客户端（已废弃）
    sslv3_client,        // SSLv3客户端（已废弃）
    tlsv1_client,        // TLSv1客户端（已废弃）
    tlsv11_client,       // TLSv1.1客户端（已废弃）
    tlsv12_client,       // TLSv1.2客户端（已废弃）
    tlsv13_client,       // TLSv1.3客户端（已废弃）
    
    // 服务器方法
    tls_server,          // TLS服务器（推荐用于服务器）
    sslv2_server,        // SSLv2服务器（已废弃）
    sslv3_server,        // SSLv3服务器（已废弃）
    tlsv1_server,        // TLSv1服务器（已废弃）
    tlsv11_server,       // TLSv1.1服务器（已废弃）
    tlsv12_server,       // TLSv1.2服务器（已废弃）
    tlsv13_server,       // TLSv1.3服务器（已废弃）
    
    // 通用方法
    tls,                 // TLS（推荐用于通用）
    sslv2,               // SSLv2（已废弃）
    sslv3,               // SSLv3（已废弃）
    tlsv1,               // TLSv1（已废弃）
    tlsv11,              // TLSv1.1（已废弃）
    tlsv12,              // TLSv1.2（已废弃）
    tlsv13,              // TLSv1.3（已废弃）
    sslv23               // 兼容模式（已废弃，使用tls替代）
};

} // namespace ssl
} // namespace uasio 