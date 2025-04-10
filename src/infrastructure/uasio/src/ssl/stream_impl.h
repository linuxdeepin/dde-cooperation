// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_SSL_STREAM_IMPL_H
#define UASIO_SSL_STREAM_IMPL_H

#include "stream.h"
#include "context.h"
#include "context_impl.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <cstring>
#include <atomic>

namespace uasio {
namespace ssl {

// SSL流内部实现
class stream_impl {
public:
    // 构造函数
    stream_impl(context& ctx) 
        : ssl_(nullptr), bio_read_(nullptr), bio_write_(nullptr) {
        
        // 获取SSL上下文
        if (!ctx.native_handle()) return;
        
        // 创建SSL对象
        ssl_ = SSL_new(ctx.native_handle()->native_handle());
        if (!ssl_) return;
        
        // 创建BIO对象
        bio_read_ = BIO_new(BIO_s_mem());
        bio_write_ = BIO_new(BIO_s_mem());
        if (!bio_read_ || !bio_write_) {
            if (ssl_) {
                SSL_free(ssl_);
                ssl_ = nullptr;
            }
            if (bio_read_) {
                BIO_free(bio_read_);
                bio_read_ = nullptr;
            }
            if (bio_write_) {
                BIO_free(bio_write_);
                bio_write_ = nullptr;
            }
            return;
        }
        
        // 设置BIO
        SSL_set_bio(ssl_, bio_read_, bio_write_);
    }
    
    // 析构函数
    ~stream_impl() {
        if (ssl_) {
            // SSL_free会自动释放关联的BIO
            SSL_free(ssl_);
            ssl_ = nullptr;
            bio_read_ = nullptr; // 已被SSL_free释放
            bio_write_ = nullptr; // 已被SSL_free释放
        }
    }
    
    // 获取SSL对象
    SSL* native_handle() {
        return ssl_;
    }
    
    // 获取SSL对象（const版本）
    const SSL* native_handle() const {
        return ssl_;
    }
    
    // 执行握手
    bool handshake(handshake_type type, error_code& ec) {
        ec = error_code();
        
        if (!ssl_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return false;
        }
        
        // 根据类型执行握手
        int result = (type == handshake_type::client) ? 
                      SSL_connect(ssl_) : SSL_accept(ssl_);
        
        if (result <= 0) {
            int ssl_error = SSL_get_error(ssl_, result);
            
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                // 需要更多数据，稍后重试
                ec = uasio::make_error_code(ssl_error == SSL_ERROR_WANT_READ ? 
                                       uasio::error::operation_in_progress : uasio::error::would_block);
                return false;
            }
            
            // 握手失败
            ec = convert_ssl_error(ssl_error);
            return false;
        }
        
        return true;
    }
    
    // 关闭SSL连接
    bool shutdown(error_code& ec) {
        ec = error_code();
        
        if (!ssl_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return false;
        }
        
        int result = SSL_shutdown(ssl_);
        
        if (result < 0) {
            int ssl_error = SSL_get_error(ssl_, result);
            
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                // 需要更多数据，稍后重试
                ec = uasio::make_error_code(ssl_error == SSL_ERROR_WANT_READ ? 
                                       uasio::error::operation_in_progress : uasio::error::would_block);
                return false;
            }
            
            // 关闭失败
            ec = convert_ssl_error(ssl_error);
            return false;
        }
        
        // 如果返回0，表示关闭还未完成（只有一个方向已关闭）
        // 如果返回1，表示已完全关闭
        return (result == 1);
    }
    
    // 写入数据到SSL
    std::size_t write(const void* data, std::size_t size, error_code& ec) {
        ec = error_code();
        
        if (!ssl_ || !data || size == 0) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return 0;
        }
        
        int result = SSL_write(ssl_, data, static_cast<int>(size));
        
        if (result <= 0) {
            int ssl_error = SSL_get_error(ssl_, result);
            
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                // 需要更多数据，稍后重试
                ec = uasio::make_error_code(ssl_error == SSL_ERROR_WANT_READ ? 
                                       uasio::error::operation_in_progress : uasio::error::would_block);
                return 0;
            }
            
            // 写入失败
            ec = convert_ssl_error(ssl_error);
            return 0;
        }
        
        return static_cast<std::size_t>(result);
    }
    
    // 从SSL读取数据
    std::size_t read(void* data, std::size_t max_size, error_code& ec) {
        ec = error_code();
        
        if (!ssl_ || !data || max_size == 0) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return 0;
        }
        
        int result = SSL_read(ssl_, data, static_cast<int>(max_size));
        
        if (result <= 0) {
            int ssl_error = SSL_get_error(ssl_, result);
            
            if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                // 需要更多数据，稍后重试
                ec = uasio::make_error_code(ssl_error == SSL_ERROR_WANT_READ ? 
                                       uasio::error::operation_in_progress : uasio::error::would_block);
                return 0;
            }
            
            // 读取失败
            ec = convert_ssl_error(ssl_error);
            return 0;
        }
        
        return static_cast<std::size_t>(result);
    }
    
    // 向BIO写入数据（从网络读取的加密数据）
    std::size_t write_encrypted_data(const void* data, std::size_t size, error_code& ec) {
        ec = error_code();
        
        if (!ssl_ || !bio_read_ || !data || size == 0) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return 0;
        }
        
        int result = BIO_write(bio_read_, data, static_cast<int>(size));
        
        if (result <= 0) {
            if (BIO_should_retry(bio_read_)) {
                ec = uasio::make_error_code(uasio::error::would_block);
                return 0;
            }
            
            ec = uasio::make_error_code(uasio::error::protocol_error);
            return 0;
        }
        
        return static_cast<std::size_t>(result);
    }
    
    // 从BIO读取数据（要发送到网络的加密数据）
    std::size_t read_encrypted_data(void* data, std::size_t max_size, error_code& ec) {
        ec = error_code();
        
        if (!ssl_ || !bio_write_ || !data || max_size == 0) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return 0;
        }
        
        int result = BIO_read(bio_write_, data, static_cast<int>(max_size));
        
        if (result <= 0) {
            if (BIO_should_retry(bio_write_)) {
                ec = uasio::make_error_code(uasio::error::would_block);
                return 0;
            }
            
            ec = uasio::make_error_code(uasio::error::protocol_error);
            return 0;
        }
        
        return static_cast<std::size_t>(result);
    }
    
    // 获取BIO中可读取的字节数
    std::size_t available_bytes() const {
        if (!ssl_ || !bio_write_) {
            return 0;
        }
        
        return static_cast<std::size_t>(BIO_ctrl_pending(bio_write_));
    }
    
    // 获取SSL会话信息
    std::string get_session_info() const {
        if (!ssl_) {
            return "";
        }
        
        std::string info;
        
        // 获取SSL版本
        const char* version = SSL_get_version(ssl_);
        if (version) {
            info += "版本: ";
            info += version;
            info += "\n";
        }
        
        // 获取加密算法
        const char* cipher = SSL_get_cipher_name(ssl_);
        if (cipher) {
            info += "加密算法: ";
            info += cipher;
            info += "\n";
        }
        
        // 获取密钥交换算法
        const char* kx = SSL_get_cipher_version(ssl_);
        if (kx) {
            info += "密钥交换: ";
            info += kx;
            info += "\n";
        }
        
        return info;
    }
    
    // 获取对端证书信息
    std::string get_peer_certificate_info() const {
        if (!ssl_) {
            return "";
        }
        
        // 获取对端证书
        X509* cert = SSL_get_peer_certificate(ssl_);
        if (!cert) {
            return "无对端证书";
        }
        
        // 获取证书信息
        std::string info;
        
        // 获取主题
        char subject[256] = {0};
        X509_NAME* subject_name = X509_get_subject_name(cert);
        if (subject_name) {
            X509_NAME_oneline(subject_name, subject, sizeof(subject) - 1);
            info += "主题: ";
            info += subject;
            info += "\n";
        }
        
        // 获取颁发者
        char issuer[256] = {0};
        X509_NAME* issuer_name = X509_get_issuer_name(cert);
        if (issuer_name) {
            X509_NAME_oneline(issuer_name, issuer, sizeof(issuer) - 1);
            info += "颁发者: ";
            info += issuer;
            info += "\n";
        }
        
        // 获取有效期
        ASN1_TIME* not_before = X509_get_notBefore(cert);
        ASN1_TIME* not_after = X509_get_notAfter(cert);
        
        if (not_before && not_after) {
            char before_str[128] = {0};
            char after_str[128] = {0};
            
            // 格式化有效期开始时间
            BIO* bio = BIO_new(BIO_s_mem());
            if (bio) {
                ASN1_TIME_print(bio, not_before);
                BIO_read(bio, before_str, sizeof(before_str) - 1);
                BIO_free(bio);
            }
            
            bio = BIO_new(BIO_s_mem());
            if (bio) {
                ASN1_TIME_print(bio, not_after);
                BIO_read(bio, after_str, sizeof(after_str) - 1);
                BIO_free(bio);
            }
            
            info += "有效期: 从 ";
            info += before_str;
            info += " 到 ";
            info += after_str;
            info += "\n";
        }
        
        // 释放证书
        X509_free(cert);
        
        return info;
    }
    
private:
    // 将OpenSSL错误码转换为SSL错误
    error_code convert_ssl_error(int ssl_error) const {
        unsigned long err = ERR_get_error();
        
        if (err == 0) {
            // 没有特定错误
            switch (ssl_error) {
                case SSL_ERROR_ZERO_RETURN:
                    return uasio::make_error_code(uasio::error::stream_truncated);
                    
                case SSL_ERROR_WANT_READ:
                    return uasio::make_error_code(uasio::error::operation_in_progress);
                    
                case SSL_ERROR_WANT_WRITE:
                    return uasio::make_error_code(uasio::error::would_block);
                    
                case SSL_ERROR_WANT_CONNECT:
                case SSL_ERROR_WANT_ACCEPT:
                    return uasio::make_error_code(uasio::error::operation_in_progress);
                    
                case SSL_ERROR_SYSCALL:
                    return uasio::make_error_code(uasio::error::certificate_not_found);
                    
                case SSL_ERROR_SSL:
                    return uasio::make_error_code(uasio::error::protocol_error);
                    
                default:
                    if (X509_V_ERR_CERT_HAS_EXPIRED == err) {
                        return uasio::make_error_code(uasio::error::certificate_verification_failed);
                    } else {
                        return uasio::make_error_code(uasio::error::unspecified_ssl_error);
                    }
            }
        } else {
            // 有特定OpenSSL错误
            return uasio::make_error_code(uasio::error::unspecified_ssl_error);
        }
    }
    
    // SSL对象
    SSL* ssl_;
    
    // BIO对象
    BIO* bio_read_;  // 用于读取
    BIO* bio_write_; // 用于写入
};

// stream 模板类实现

template <typename Stream>
inline stream<Stream>::stream(io_context& io_context, context& ctx)
    : next_layer_(io_context), impl_(std::make_shared<stream_impl>(ctx))
{
}

template <typename Stream>
inline stream<Stream>::~stream() = default;

template <typename Stream>
inline typename stream<Stream>::next_layer_type& stream<Stream>::next_layer() {
    return next_layer_;
}

template <typename Stream>
inline const typename stream<Stream>::next_layer_type& stream<Stream>::next_layer() const {
    return next_layer_;
}

template <typename Stream>
inline void stream<Stream>::handshake(handshake_type type, error_code& ec) {
    // 先确保底层连接建立
    if (!next_layer_.is_open()) {
        ec = uasio::make_error_code(uasio::error::not_connected);
        return;
    }
    
    // 执行SSL握手
    impl_->handshake(type, ec);
    
    // 如果握手进行中，需要读写数据
    while (ec == uasio::make_error_code(uasio::error::operation_in_progress) || 
           ec == uasio::make_error_code(uasio::error::would_block)) {
        
        // 检查是否有加密数据需要发送
        char outbuf[4096];
        error_code write_ec;
        std::size_t bytes_available = impl_->available_bytes();
        
        if (bytes_available > 0) {
            // 从SSL读取加密数据
            std::size_t bytes_read = impl_->read_encrypted_data(outbuf, sizeof(outbuf), write_ec);
            
            if (!write_ec && bytes_read > 0) {
                // 发送数据到对端
                next_layer_.send(outbuf, bytes_read, write_ec);
                
                if (write_ec) {
                    ec = write_ec;
                    return;
                }
            }
            else if (write_ec) {
                ec = write_ec;
                return;
            }
        }
        
        // 读取来自对端的加密数据
        char inbuf[4096];
        error_code read_ec;
        std::size_t bytes_received = next_layer_.receive(inbuf, sizeof(inbuf), read_ec);
        
        if (!read_ec && bytes_received > 0) {
            // 将接收到的加密数据写入SSL
            impl_->write_encrypted_data(inbuf, bytes_received, read_ec);
            
            if (read_ec) {
                ec = read_ec;
                return;
            }
        }
        else if (read_ec && read_ec != uasio::make_error_code(uasio::error::would_block)) {
            ec = read_ec;
            return;
        }
        
        // 继续握手
        impl_->handshake(type, ec);
    }
}

template <typename Stream>
template <typename HandshakeHandler>
inline void stream<Stream>::async_handshake(handshake_type type, HandshakeHandler&& handler) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        io_context& ioc = next_layer_.get_io_context();
        ioc.post([handler = std::forward<HandshakeHandler>(handler)]() {
            handler(uasio::make_error_code(uasio::error::not_connected));
        });
        return;
    }
    
    // 握手操作的具体实现将在stream.cpp中定义
    // 这里用一个简单的实现作为示例
    error_code ec;
    handshake(type, ec);
    
    // 调用完成处理程序
    io_context& ioc = next_layer_.get_io_context();
    ioc.post([handler = std::forward<HandshakeHandler>(handler), ec]() {
        handler(ec);
    });
}

template <typename Stream>
inline void stream<Stream>::shutdown(error_code& ec) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        ec = uasio::make_error_code(uasio::error::not_connected);
        return;
    }
    
    // 执行SSL关闭
    impl_->shutdown(ec);
    
    // 如果关闭进行中，需要读写数据
    while (ec == uasio::make_error_code(uasio::error::operation_in_progress) || 
           ec == uasio::make_error_code(uasio::error::would_block)) {
        
        // 检查是否有加密数据需要发送
        char outbuf[4096];
        error_code write_ec;
        std::size_t bytes_available = impl_->available_bytes();
        
        if (bytes_available > 0) {
            // 从SSL读取加密数据
            std::size_t bytes_read = impl_->read_encrypted_data(outbuf, sizeof(outbuf), write_ec);
            
            if (!write_ec && bytes_read > 0) {
                // 发送数据到对端
                next_layer_.send(outbuf, bytes_read, write_ec);
                
                if (write_ec) {
                    ec = write_ec;
                    return;
                }
            }
            else if (write_ec) {
                ec = write_ec;
                return;
            }
        }
        
        // 读取来自对端的加密数据
        char inbuf[4096];
        error_code read_ec;
        std::size_t bytes_received = next_layer_.receive(inbuf, sizeof(inbuf), read_ec);
        
        if (!read_ec && bytes_received > 0) {
            // 将接收到的加密数据写入SSL
            impl_->write_encrypted_data(inbuf, bytes_received, read_ec);
            
            if (read_ec) {
                ec = read_ec;
                return;
            }
        }
        else if (read_ec && read_ec != uasio::make_error_code(uasio::error::would_block)) {
            ec = read_ec;
            return;
        }
        
        // 继续关闭
        impl_->shutdown(ec);
    }
}

template <typename Stream>
template <typename ShutdownHandler>
inline void stream<Stream>::async_shutdown(ShutdownHandler&& handler) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        io_context& ioc = next_layer_.get_io_context();
        ioc.post([handler = std::forward<ShutdownHandler>(handler)]() {
            handler(uasio::make_error_code(uasio::error::not_connected));
        });
        return;
    }
    
    // 关闭操作的具体实现将在stream.cpp中定义
    // 这里用一个简单的实现作为示例
    error_code ec;
    shutdown(ec);
    
    // 调用完成处理程序
    io_context& ioc = next_layer_.get_io_context();
    ioc.post([handler = std::forward<ShutdownHandler>(handler), ec]() {
        handler(ec);
    });
}

template <typename Stream>
inline std::size_t stream<Stream>::write_some(const void* data, std::size_t size, error_code& ec) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        ec = uasio::make_error_code(uasio::error::not_connected);
        return 0;
    }
    
    // 写入SSL
    std::size_t bytes_written = impl_->write(data, size, ec);
    
    // 如果有数据要发送
    char outbuf[4096];
    std::size_t bytes_available = impl_->available_bytes();
    
    while (bytes_available > 0) {
        // 从SSL读取加密数据
        error_code read_ec;
        std::size_t bytes_read = impl_->read_encrypted_data(outbuf, sizeof(outbuf), read_ec);
        
        if (read_ec) {
            ec = read_ec;
            return 0;
        }
        
        if (bytes_read == 0) {
            break;
        }
        
        // 发送数据到对端
        error_code write_ec;
        std::size_t bytes_sent = next_layer_.send(outbuf, bytes_read, write_ec);
        
        if (write_ec) {
            ec = write_ec;
            return 0;
        }
        
        // 检查是否有更多数据
        bytes_available = impl_->available_bytes();
    }
    
    return bytes_written;
}

template <typename Stream>
inline std::size_t stream<Stream>::write_some(const const_buffer& buffer, error_code& ec) {
    return write_some(buffer.data(), buffer.size(), ec);
}

template <typename Stream>
template <typename WriteHandler>
inline void stream<Stream>::async_write_some(const void* data, std::size_t size, WriteHandler&& handler) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        io_context& ioc = next_layer_.get_io_context();
        ioc.post([handler = std::forward<WriteHandler>(handler)]() {
            handler(uasio::make_error_code(uasio::error::not_connected), 0);
        });
        return;
    }
    
    // 写入操作的具体实现将在stream.cpp中定义
    // 这里用一个简单的实现作为示例
    error_code ec;
    std::size_t bytes_written = write_some(data, size, ec);
    
    // 调用完成处理程序
    io_context& ioc = next_layer_.get_io_context();
    ioc.post([handler = std::forward<WriteHandler>(handler), ec, bytes_written]() {
        handler(ec, bytes_written);
    });
}

template <typename Stream>
template <typename WriteHandler>
inline void stream<Stream>::async_write_some(const const_buffer& buffer, WriteHandler&& handler) {
    async_write_some(buffer.data(), buffer.size(), std::forward<WriteHandler>(handler));
}

template <typename Stream>
inline std::size_t stream<Stream>::read_some(void* data, std::size_t max_size, error_code& ec) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        ec = uasio::make_error_code(uasio::error::not_connected);
        return 0;
    }
    
    // 尝试从SSL读取数据
    std::size_t bytes_read = impl_->read(data, max_size, ec);
    
    // 如果SSL需要更多数据
    while (ec == uasio::make_error_code(uasio::error::operation_in_progress) || 
           ec == uasio::make_error_code(uasio::error::would_block)) {
        
        // 读取来自对端的加密数据
        char inbuf[4096];
        error_code read_ec;
        std::size_t bytes_received = next_layer_.receive(inbuf, sizeof(inbuf), read_ec);
        
        if (read_ec && read_ec != uasio::make_error_code(uasio::error::would_block)) {
            ec = read_ec;
            return 0;
        }
        
        if (bytes_received > 0) {
            // 将接收到的加密数据写入SSL
            impl_->write_encrypted_data(inbuf, bytes_received, read_ec);
            
            if (read_ec) {
                ec = read_ec;
                return 0;
            }
            
            // 再次尝试读取
            bytes_read = impl_->read(data, max_size, ec);
        }
        else {
            // 没有数据，可能需要稍后再试
            break;
        }
    }
    
    return bytes_read;
}

template <typename Stream>
inline std::size_t stream<Stream>::read_some(const mutable_buffer& buffer, error_code& ec) {
    return read_some(buffer.data(), buffer.size(), ec);
}

template <typename Stream>
template <typename ReadHandler>
inline void stream<Stream>::async_read_some(void* data, std::size_t max_size, ReadHandler&& handler) {
    // 确保底层连接建立
    if (!next_layer_.is_open()) {
        io_context& ioc = next_layer_.get_io_context();
        ioc.post([handler = std::forward<ReadHandler>(handler)]() {
            handler(uasio::make_error_code(uasio::error::not_connected), 0);
        });
        return;
    }
    
    // 读取操作的具体实现将在stream.cpp中定义
    // 这里用一个简单的实现作为示例
    error_code ec;
    std::size_t bytes_read = read_some(data, max_size, ec);
    
    // 调用完成处理程序
    io_context& ioc = next_layer_.get_io_context();
    ioc.post([handler = std::forward<ReadHandler>(handler), ec, bytes_read]() {
        handler(ec, bytes_read);
    });
}

template <typename Stream>
template <typename ReadHandler>
inline void stream<Stream>::async_read_some(const mutable_buffer& buffer, ReadHandler&& handler) {
    async_read_some(buffer.data(), buffer.size(), std::forward<ReadHandler>(handler));
}

template <typename Stream>
inline std::string stream<Stream>::get_session_info() const {
    return impl_->get_session_info();
}

template <typename Stream>
inline std::string stream<Stream>::get_peer_certificate_info() const {
    return impl_->get_peer_certificate_info();
}

// 完整读写操作的实现

template <typename Stream>
inline std::size_t write(stream<Stream>& s, const const_buffer& buffer, error_code& ec) {
    std::size_t bytes_transferred = 0;
    std::size_t total_size = buffer.size();
    
    while (bytes_transferred < total_size) {
        std::size_t bytes = s.write_some(
            static_cast<const char*>(buffer.data()) + bytes_transferred,
            total_size - bytes_transferred, ec);
        
        if (ec && ec != uasio::make_error_code(uasio::error::eof))
            break;
            
        bytes_transferred += bytes;
        
        if (bytes == 0)
            break;
    }
    
    return bytes_transferred;
}

template <typename Stream, typename WriteHandler>
inline void async_write(stream<Stream>& s, const const_buffer& buffer, WriteHandler&& handler) {
    // 异步写入操作的具体实现将在stream.cpp中定义
    // 这里用一个简单的实现作为示例
    error_code ec;
    std::size_t bytes_written = write(s, buffer, ec);
    
    // 调用完成处理程序
    io_context& ioc = s.next_layer().get_io_context();
    ioc.post([handler = std::forward<WriteHandler>(handler), ec, bytes_written]() {
        handler(ec, bytes_written);
    });
}

template <typename Stream>
inline std::size_t read(stream<Stream>& s, const mutable_buffer& buffer, error_code& ec) {
    std::size_t bytes_transferred = 0;
    std::size_t total_size = buffer.size();
    
    while (bytes_transferred < total_size) {
        std::size_t bytes = s.read_some(
            static_cast<char*>(buffer.data()) + bytes_transferred,
            total_size - bytes_transferred, ec);
        
        if (ec && ec != uasio::make_error_code(uasio::error::eof))
            break;
            
        bytes_transferred += bytes;
        
        if (bytes == 0)
            break;
    }
    
    return bytes_transferred;
}

template <typename Stream, typename ReadHandler>
inline void async_read(stream<Stream>& s, const mutable_buffer& buffer, ReadHandler&& handler) {
    // 异步读取操作的具体实现将在stream.cpp中定义
    // 这里用一个简单的实现作为示例
    error_code ec;
    std::size_t bytes_read = read(s, buffer, ec);
    
    // 调用完成处理程序
    io_context& ioc = s.next_layer().get_io_context();
    ioc.post([handler = std::forward<ReadHandler>(handler), ec, bytes_read]() {
        handler(ec, bytes_read);
    });
}

} // namespace ssl
} // namespace uasio

#endif // UASIO_SSL_STREAM_IMPL_H 