// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <openssl/x509.h>
#include "context_base.h"
#include "../error.h"

namespace uasio {
namespace ssl {

// 前向声明
class context_impl;

/**
 * @brief SSL上下文类，提供SSL配置和证书管理
 */
class context {
public:
    /**
     * @brief 构造函数
     * 
     * @param m SSL方法类型
     */
    explicit context(method m);
    
    /**
     * @brief 析构函数
     */
    ~context();
    
    /// 不允许复制
    context(const context&) = delete;
    context& operator=(const context&) = delete;
    
    /**
     * @brief 设置验证模式
     * 
     * @param v 验证模式
     * @param ec 错误码
     */
    void set_verify_mode(verify_mode v, error_code& ec);
    
    /**
     * @brief 设置验证深度
     * 
     * @param depth 验证深度
     * @param ec 错误码
     */
    void set_verify_depth(int depth, error_code& ec);
    
    /**
     * @brief 加载验证文件
     * 
     * @param filename 文件名
     * @param format 文件格式
     * @param ec 错误码
     */
    void load_verify_file(const std::string& filename, file_format format, error_code& ec);
    
    /**
     * @brief 添加验证路径
     * 
     * @param path 路径
     * @param ec 错误码
     */
    void add_verify_path(const std::string& path, error_code& ec);
    
    /**
     * @brief 使用证书链文件
     * 
     * @param filename 文件名
     * @param format 文件格式
     * @param ec 错误码
     */
    void use_certificate_chain_file(const std::string& filename, file_format format, error_code& ec);
    
    /**
     * @brief 使用证书文件
     * 
     * @param filename 文件名
     * @param format 文件格式
     * @param ec 错误码
     */
    void use_certificate_file(const std::string& filename, file_format format, error_code& ec);
    
    /**
     * @brief 使用私钥文件
     * 
     * @param filename 文件名
     * @param format 文件格式
     * @param ec 错误码
     */
    void use_private_key_file(const std::string& filename, file_format format, error_code& ec);
    
    /**
     * @brief 使用RSA私钥文件
     * 
     * @param filename 文件名
     * @param format 文件格式
     * @param ec 错误码
     */
    void use_rsa_private_key_file(const std::string& filename, file_format format, error_code& ec);
    
    /**
     * @brief 使用临时DH参数文件
     * 
     * @param filename 文件名
     * @param ec 错误码
     */
    void use_tmp_dh_file(const std::string& filename, error_code& ec);
    
    /**
     * @brief 设置密码回调
     * 
     * @param callback 密码回调函数
     * @param ec 错误码
     */
    void set_password_callback(std::function<std::string(std::size_t, bool)> callback, error_code& ec);
    
    /**
     * @brief 设置选项
     * 
     * @param options 选项
     * @param ec 错误码
     */
    void set_options(long options, error_code& ec);
    
    /**
     * @brief 清除选项
     * 
     * @param options 选项
     * @param ec 错误码
     */
    void clear_options(long options, error_code& ec);
    
    /**
     * @brief 设置加密算法列表
     * 
     * @param ciphers 加密算法列表
     * @param ec 错误码
     */
    void set_cipher_list(const std::string& ciphers, error_code& ec);
    
    /**
     * @brief 设置验证回调
     * @param callback 验证回调函数
     */
    template <typename VerifyCallback>
    void set_verify_callback(VerifyCallback callback) {
        // 在具体实现中完成
    }
    
    /**
     * @brief 获取本地句柄指针
     * 
     * @return 本地句柄指针
     */
    context_impl* native_handle();
    
    /**
     * @brief 获取本地句柄指针（const版本）
     * 
     * @return 本地句柄指针
     */
    const context_impl* native_handle() const;
    
private:
    // 实现指针
    std::unique_ptr<context_impl> impl_;
};

/// 验证上下文类
class verify_context {
public:
    verify_context() = default;
    
    /// 获取本地证书
    X509* native_handle();
    
    /// 设置本地证书
    void set_native_handle(X509* handle);
    
private:
    X509* cert_ = nullptr;
};

} // namespace ssl
} // namespace uasio 