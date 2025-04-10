// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "context.h"
#include "context_impl.h"
#include "../error.h"
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <mutex>
#include <functional>
#include <stdexcept>
#include <iostream>

namespace uasio {
namespace ssl {

// OpenSSL库初始化/清理
namespace {
    // 初始化 OpenSSL 库
    class openssl_init {
    public:
        openssl_init() {
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_all_algorithms();
        }
        
        ~openssl_init() {
            ERR_free_strings();
            EVP_cleanup();
        }
    };
    
    void init_openssl() {
        static openssl_init init;
    }
    
    std::once_flag init_flag;
    
    void ensure_openssl_init() {
        std::call_once(init_flag, init_openssl);
    }
}

// 上下文类实现

context::context(method m) : impl_(std::make_unique<context_impl>(m)) {
}

context::~context() = default;

void context::set_verify_mode(verify_mode v, error_code& ec) {
    impl_->set_verify_mode(v, ec);
}

void context::set_verify_depth(int depth, error_code& ec) {
    impl_->set_verify_depth(depth, ec);
}

void context::load_verify_file(const std::string& filename, file_format format, error_code& ec) {
    impl_->load_verify_file(filename, format, ec);
}

void context::add_verify_path(const std::string& path, error_code& ec) {
    impl_->add_verify_path(path, ec);
}

void context::use_certificate_chain_file(const std::string& filename, file_format format, error_code& ec) {
    impl_->use_certificate_chain_file(filename, format, ec);
}

void context::use_certificate_file(const std::string& filename, file_format format, error_code& ec) {
    impl_->use_certificate_file(filename, format, ec);
}

void context::use_private_key_file(const std::string& filename, file_format format, error_code& ec) {
    impl_->use_private_key_file(filename, format, ec);
}

void context::use_rsa_private_key_file(const std::string& filename, file_format format, error_code& ec) {
    impl_->use_rsa_private_key_file(filename, format, ec);
}

void context::use_tmp_dh_file(const std::string& filename, error_code& ec) {
    impl_->use_tmp_dh_file(filename, ec);
}

void context::set_password_callback(std::function<std::string(std::size_t, bool)> callback, error_code& ec) {
    impl_->set_password_callback(std::move(callback), ec);
}

void context::set_options(long options, error_code& ec) {
    impl_->set_options(options, ec);
}

void context::clear_options(long options, error_code& ec) {
    impl_->clear_options(options, ec);
}

void context::set_cipher_list(const std::string& ciphers, error_code& ec) {
    impl_->set_cipher_list(ciphers, ec);
}

context_impl* context::native_handle() {
    return impl_.get();
}

const context_impl* context::native_handle() const {
    return impl_.get();
}

} // namespace ssl
} // namespace uasio 