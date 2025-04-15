#pragma once

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <string>
#include <functional>
#include <cstring>
#include <mutex>
#include "../error.h"
#include "context_base.h"

namespace uasio {
namespace ssl {

// OpenSSL初始化类
class openssl_init {
public:
    openssl_init() {
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_crypto_strings();
        OpenSSL_add_all_algorithms();
    }
    
    ~openssl_init() {
        ERR_free_strings();
        EVP_cleanup();
    }
};

// 初始化OpenSSL
inline void init_openssl() {
    static openssl_init init;
}

// 确保OpenSSL已初始化
inline void ensure_openssl_init() {
    static std::once_flag init_flag;
    std::call_once(init_flag, init_openssl);
}

// SSL上下文实现类
class context_impl {
public:
    // 密码回调封装类型
    using password_callback_type = std::function<std::string(std::size_t, bool)>;
    
    // 密码回调函数
    static int password_callback(char* buf, int size, int rwflag, void* user_data) {
        if (!user_data)
            return 0;
            
        password_callback_type* callback = static_cast<password_callback_type*>(user_data);
        std::string passwd = (*callback)(static_cast<std::size_t>(size), rwflag != 0);
        
        if (passwd.length() > static_cast<std::size_t>(size))
            passwd.resize(static_cast<std::size_t>(size));
            
        std::memcpy(buf, passwd.c_str(), passwd.length());
        return static_cast<int>(passwd.length());
    }

    // 构造函数
    explicit context_impl(method m) : ssl_ctx_(nullptr) {
        // 确保 OpenSSL 已初始化
        ensure_openssl_init();
        
        // 根据方法类型创建SSL上下文
        ssl_ctx_ = create_context(m);
    }
    
    // 析构函数
    ~context_impl() {
        if (ssl_ctx_) {
            SSL_CTX_free(ssl_ctx_);
            ssl_ctx_ = nullptr;
        }
    }
    
    // 获取本地SSL上下文
    SSL_CTX* native_handle() {
        return ssl_ctx_;
    }
    
    // 获取本地SSL上下文（const版本）
    const SSL_CTX* native_handle() const {
        return ssl_ctx_;
    }
    
    // 设置验证模式
    void set_verify_mode(verify_mode v, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        int mode = 0;
        if ((static_cast<int>(v) & static_cast<int>(verify_mode::peer)) != 0)
            mode |= SSL_VERIFY_PEER;
        if ((static_cast<int>(v) & static_cast<int>(verify_mode::fail_if_no_peer_cert)) != 0)
            mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        if ((static_cast<int>(v) & static_cast<int>(verify_mode::client_once)) != 0)
            mode |= SSL_VERIFY_CLIENT_ONCE;
            
        SSL_CTX_set_verify(ssl_ctx_, mode, nullptr);
    }
    
    // 设置验证深度
    void set_verify_depth(int depth, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        SSL_CTX_set_verify_depth(ssl_ctx_, depth);
    }
    
    // 加载验证文件
    void load_verify_file(const std::string& filename, file_format format, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        int file_type = (format == file_format::pem) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;
        if (SSL_CTX_load_verify_locations(ssl_ctx_, filename.c_str(), nullptr) != 1) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
    }
    
    // 添加验证路径
    void add_verify_path(const std::string& path, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        if (SSL_CTX_load_verify_locations(ssl_ctx_, nullptr, path.c_str()) != 1) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
    }
    
    // 使用证书链文件
    void use_certificate_chain_file(const std::string& filename, file_format format, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        if (format != file_format::pem) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return;
        }
        
        if (SSL_CTX_use_certificate_chain_file(ssl_ctx_, filename.c_str()) != 1) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
    }
    
    // 使用证书文件
    void use_certificate_file(const std::string& filename, file_format format, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        int file_type = (format == file_format::pem) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;
        if (SSL_CTX_use_certificate_file(ssl_ctx_, filename.c_str(), file_type) != 1) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
    }
    
    // 使用私钥文件
    void use_private_key_file(const std::string& filename, file_format format, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        int file_type = (format == file_format::pem) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, filename.c_str(), file_type) != 1) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
    }
    
    // 使用RSA私钥文件 (已废弃，使用通用私钥方法代替)
    void use_rsa_private_key_file(const std::string& filename, file_format format, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        // 在OpenSSL 3.0中，推荐使用SSL_CTX_use_PrivateKey_file而不是特定于RSA的函数
        int file_type = (format == file_format::pem) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, filename.c_str(), file_type) != 1) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
    }
    
    // 使用临时DH参数文件
    void use_tmp_dh_file(const std::string& filename, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
        // OpenSSL 3.0及以上版本，使用SSL_CTX_set1_curves_list设置DH参数
        // 设置一个合理的默认值，例如"ffdhe2048"
        if (SSL_CTX_set1_curves_list(ssl_ctx_, "ffdhe2048") != 1) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
#else
        // 老版本OpenSSL，使用传统的DH参数方法
        BIO* bio = BIO_new_file(filename.c_str(), "r");
        if (!bio) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
        
        DH* dh = PEM_read_bio_DHparams(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        
        if (!dh) {
            ec = uasio::make_error_code(uasio::error::certificate_not_found);
            return;
        }
        
        int result = SSL_CTX_set_tmp_dh(ssl_ctx_, dh);
        DH_free(dh);
        
        if (result != 1) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
#endif
    }
    
    // 设置密码回调
    void set_password_callback(std::function<std::string(std::size_t, bool)> callback, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        if (!callback) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return;
        }
        
        // 保存回调对象
        password_callback_ = std::move(callback);
        
        // 设置OpenSSL回调
        SSL_CTX_set_default_passwd_cb(ssl_ctx_, &password_callback);
        SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx_, &password_callback_);
    }
    
    // 设置选项
    void set_options(long options, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        SSL_CTX_set_options(ssl_ctx_, options);
    }
    
    // 清除选项
    void clear_options(long options, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        SSL_CTX_clear_options(ssl_ctx_, options);
    }
    
    // 设置加密算法列表
    void set_cipher_list(const std::string& ciphers, error_code& ec) {
        ec = error_code();
        if (!ssl_ctx_) {
            ec = uasio::make_error_code(uasio::error::internal_error);
            return;
        }
        
        if (ciphers.empty()) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return;
        }
        
        if (SSL_CTX_set_cipher_list(ssl_ctx_, ciphers.c_str()) != 1) {
            ec = uasio::make_error_code(uasio::error::invalid_argument);
            return;
        }
    }
    
private:
    // 根据方法类型创建SSL上下文
    SSL_CTX* create_context(method m) {
        const SSL_METHOD* method_impl = nullptr;
        
        // 在OpenSSL 1.1.0及以上版本中，推荐使用TLS_method，TLS_client_method和
        // TLS_server_method，然后通过设置选项来限制特定的协议版本
        switch (m) {
            // 客户端方法
            case method::tls_client:
                method_impl = TLS_client_method();
                break;
            
            // 服务器方法
            case method::tls_server:
                method_impl = TLS_server_method();
                break;
            
            // 通用方法
            case method::tls:
                method_impl = TLS_method();
                break;
                
            // 以下方法已过时，使用TLS方法并设置适当的选项
            case method::sslv2:
            case method::sslv3:
            case method::sslv2_client:
            case method::sslv3_client:
            case method::sslv2_server:
            case method::sslv3_server:
                // SSL v2/v3 已被弃用，不安全
                return nullptr;
                
            case method::tlsv1:
            case method::tlsv1_client:
            case method::tlsv1_server:
                method_impl = TLS_method();
                break;
                
            case method::tlsv11:
            case method::tlsv11_client:
            case method::tlsv11_server:
                method_impl = TLS_method();
                break;
                
            case method::tlsv12:
            case method::tlsv12_client:
            case method::tlsv12_server:
                method_impl = TLS_method();
                break;
                
            case method::tlsv13:
            case method::tlsv13_client:
            case method::tlsv13_server:
                method_impl = TLS_method();
                break;
                
            case method::sslv23:
                // 兼容模式
                method_impl = TLS_method();
                break;
                
            default:
                method_impl = TLS_method();
                break;
        }
        
        // 创建SSL上下文
        SSL_CTX* ctx = SSL_CTX_new(method_impl);
        
        // 根据方法设置适当的选项
        if (ctx) {
            // 默认禁用不安全的SSL协议
            SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
            
            // 根据请求的方法设置协议版本限制
            switch (m) {
                case method::tlsv1:
                case method::tlsv1_client:
                case method::tlsv1_server:
                    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3);
                    break;
                    
                case method::tlsv11:
                case method::tlsv11_client:
                case method::tlsv11_server:
                    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3);
                    break;
                    
                case method::tlsv12:
                case method::tlsv12_client:
                case method::tlsv12_server:
                    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_3);
                    break;
                    
                case method::tlsv13:
                case method::tlsv13_client:
                case method::tlsv13_server:
                    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2);
                    break;
                    
                default:
                    // 不做限制，使用TLS最高可用版本
                    break;
            }
        }
        
        return ctx;
    }
    
    // SSL上下文
    SSL_CTX* ssl_ctx_;
    
    // 密码回调
    password_callback_type password_callback_;
};

} // namespace ssl
} // namespace uasio 