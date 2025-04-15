// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_RESOLVER_H
#define UASIO_RESOLVER_H

#include "io_context.h"
#include "error.h"
#include "socket.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>

namespace uasio {

/**
 * @brief 查询类型，指定解析时要返回的端点的类型
 */
enum class query_type {
    all,          ///< 解析所有地址（默认）
    ipv4_only,    ///< 仅解析 IPv4 地址
    ipv6_only     ///< 仅解析 IPv6 地址
};

/**
 * @brief 查询标志，控制解析行为
 */
enum class query_flags {
    none = 0,               ///< 无特殊标志
    canonical_name = 1,     ///< 返回规范主机名
    passive = 2,            ///< 被动模式（用于接收连接）
    numeric_host = 4,       ///< 主机名必须是数字地址字符串
    numeric_service = 8,    ///< 服务名必须是数字端口字符串
    v4_mapped = 16,         ///< 将 IPv4 地址映射到 IPv6 格式
    all_matching = 32,      ///< 返回所有匹配的结果
    address_configured = 64 ///< 仅返回已配置接口地址
};

/**
 * @brief 解析器查询类
 */
class resolver_query {
public:
    /**
     * @brief 默认构造函数
     */
    resolver_query() = default;
    
    /**
     * @brief 构造函数
     * @param host 主机名或 IP 地址
     * @param service 服务名或端口号
     * @param type 查询类型
     * @param flags 查询标志
     */
    resolver_query(const std::string& host, const std::string& service, 
                 query_type type = query_type::all, 
                 query_flags flags = query_flags::none);
    
    /**
     * @brief 获取主机名
     * @return 主机名
     */
    const std::string& host() const;
    
    /**
     * @brief 获取服务名
     * @return 服务名
     */
    const std::string& service() const;
    
    /**
     * @brief 获取查询类型
     * @return 查询类型
     */
    query_type type() const;
    
    /**
     * @brief 获取查询标志
     * @return 查询标志
     */
    query_flags flags() const;
    
private:
    std::string host_;
    std::string service_;
    query_type type_ = query_type::all;
    query_flags flags_ = query_flags::none;
};

/**
 * @brief 解析器条目类，表示一个解析结果
 */
class resolver_entry {
public:
    /// 默认构造函数
    resolver_entry() = default;
    
    /// 从端点和主机名构造
    resolver_entry(const uasio::endpoint& ep,
                  const std::string& host_name,
                  const std::string& service_name);
    
    /// 获取端点
    const uasio::endpoint& get_endpoint() const;
    
    /// 获取主机名
    const std::string& host_name() const;
    
    /// 获取服务名
    const std::string& service_name() const;

private:
    uasio::endpoint endpoint_;
    std::string host_name_;
    std::string service_name_;
};

/**
 * @brief 解析器结果类，包含多个解析器条目
 */
class resolver_results {
public:
    using iterator = std::vector<resolver_entry>::const_iterator;
    
    /**
     * @brief 默认构造函数
     */
    resolver_results() = default;
    
    /**
     * @brief 迭代器开始
     * @return 指向第一个条目的迭代器
     */
    iterator begin() const;
    
    /**
     * @brief 迭代器结束
     * @return 指向最后一个条目之后的迭代器
     */
    iterator end() const;
    
    /**
     * @brief 获取条目数量
     * @return 条目数量
     */
    std::size_t size() const;
    
    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    bool empty() const;
    
    /**
     * @brief 添加条目
     * @param entry 解析器条目
     */
    void add(const resolver_entry& entry);
    
private:
    std::vector<resolver_entry> entries_;
};

// 前向声明
class resolver_service;

/**
 * @brief 解析器类
 */
class resolver {
public:
    /**
     * @brief 构造函数
     * @param io_context IO 上下文
     */
    explicit resolver(uasio::io_context& io_context);
    
    /**
     * @brief 析构函数
     */
    ~resolver();
    
    /**
     * @brief 取消所有未完成的异步操作
     * @return 取消的操作数量
     */
    std::size_t cancel();
    
    /**
     * @brief 同步解析主机名和服务名
     * @param query 解析器查询
     * @param ec 错误代码
     * @return 解析结果
     */
    resolver_results resolve(const resolver_query& query, uasio::error_code& ec);
    
    /**
     * @brief 异步解析主机名和服务名
     * @param query 解析器查询
     * @param handler 完成处理器
     */
    void async_resolve(const resolver_query& query, 
                     std::function<void(const uasio::error_code&, resolver_results)>&& handler);
    
private:
    uasio::io_context& io_context_;
    std::shared_ptr<resolver_service> service_;
};

/**
 * @brief 解析器服务类
 */
class resolver_service {
public:
    /**
     * @brief 构造函数
     * @param io_context IO 上下文
     */
    explicit resolver_service(uasio::io_context& io_context);
    
    /**
     * @brief 析构函数
     */
    ~resolver_service() = default;
    
    /**
     * @brief 取消所有未完成的异步操作
     * @return 取消的操作数量
     */
    std::size_t cancel();
    
    /**
     * @brief 同步解析主机名和服务名
     * @param query 解析器查询
     * @param ec 错误代码
     * @return 解析结果
     */
    resolver_results resolve(const resolver_query& query, uasio::error_code& ec);
    
    /**
     * @brief 异步解析主机名和服务名
     * @param query 解析器查询
     * @param handler 完成处理器
     */
    void async_resolve(const resolver_query& query, 
                     std::function<void(const uasio::error_code&, resolver_results)>&& handler);
    
private:
    struct query_op {
        resolver_query query;
        std::function<void(const uasio::error_code&, resolver_results)> handler;
    };
    
    uasio::io_context& io_context_;
    std::vector<query_op> pending_ops_;
    std::atomic<bool> stopped_ {false};
};

} // namespace uasio

#endif // UASIO_RESOLVER_H 