// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "resolver.h"
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    #pragma comment(lib, "Ws2_32.lib")
#else
    #include <sys/types.h>
    #include <unistd.h>
    #include <string.h>
#endif

namespace uasio {

//
// resolver_query 实现
//

resolver_query::resolver_query(const std::string& host, const std::string& service, 
                             query_type type, query_flags flags)
    : host_(host), service_(service), type_(type), flags_(flags)
{
}

const std::string& resolver_query::host() const
{
    return host_;
}

const std::string& resolver_query::service() const
{
    return service_;
}

query_type resolver_query::type() const
{
    return type_;
}

query_flags resolver_query::flags() const
{
    return flags_;
}

//
// resolver_entry 实现
//

resolver_entry::resolver_entry(const uasio::endpoint& ep,
                             const std::string& host_name,
                             const std::string& service_name)
    : endpoint_(ep), host_name_(host_name), service_name_(service_name)
{
}

const uasio::endpoint& resolver_entry::get_endpoint() const
{
    return endpoint_;
}

const std::string& resolver_entry::host_name() const
{
    return host_name_;
}

const std::string& resolver_entry::service_name() const
{
    return service_name_;
}

//
// resolver_results 实现
//

resolver_results::iterator resolver_results::begin() const
{
    return entries_.begin();
}

resolver_results::iterator resolver_results::end() const
{
    return entries_.end();
}

std::size_t resolver_results::size() const
{
    return entries_.size();
}

bool resolver_results::empty() const
{
    return entries_.empty();
}

void resolver_results::add(const resolver_entry& entry)
{
    entries_.push_back(entry);
}

//
// resolver 实现
//

resolver::resolver(uasio::io_context& io_context)
    : io_context_(io_context)
{
    service_ = std::make_shared<resolver_service>(io_context);
}

resolver::~resolver()
{
    // 取消所有未完成的操作
    cancel();
}

std::size_t resolver::cancel()
{
    return service_->cancel();
}

resolver_results resolver::resolve(const resolver_query& query, uasio::error_code& ec)
{
    return service_->resolve(query, ec);
}

void resolver::async_resolve(const resolver_query& query, 
                          std::function<void(const uasio::error_code&, resolver_results)>&& handler)
{
    service_->async_resolve(query, std::move(handler));
}

//
// resolver_service 实现
//

resolver_service::resolver_service(uasio::io_context& io_context)
    : io_context_(io_context), stopped_(false)
{
}

std::size_t resolver_service::cancel()
{
    // 获取未完成操作数量
    std::size_t count = pending_ops_.size();
    
    // 标记为已停止
    stopped_ = true;
    
    // 通知所有未完成的操作已取消
    for (auto& op : pending_ops_) {
        io_context_.post([handler = std::move(op.handler)]() {
            handler(uasio::make_error_code(uasio::error::operation_aborted), resolver_results());
        });
    }
    
    // 清空操作队列
    pending_ops_.clear();
    
    // 重置停止标志
    stopped_ = false;
    
    return count;
}

resolver_results resolver_service::resolve(const resolver_query& query, uasio::error_code& ec)
{
    if (stopped_) {
        ec = uasio::make_error_code(uasio::error::operation_aborted);
        return resolver_results();
    }

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int ret = getaddrinfo(query.host().c_str(), 
                         query.service().c_str(),
                         &hints, &result);

    if (ret != 0) {
        ec = uasio::make_error_code(uasio::error::host_unreachable);
        return resolver_results();
    }

    // 创建结果集
    resolver_results results;

    // 遍历解析结果
    for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
        uasio::endpoint ep;
        
        // 处理 IPv4 地址
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
            
            uasio::ip_address addr_ip = uasio::ip_address::from_string(ip, ec);
            if (!ec) {
                ep = uasio::endpoint(addr_ip, ntohs(addr->sin_port));
            }
        }
        // 处理 IPv6 地址
        else if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6* addr = reinterpret_cast<struct sockaddr_in6*>(rp->ai_addr);
            char ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(addr->sin6_addr), ip, INET6_ADDRSTRLEN);
            
            uasio::ip_address addr_ip = uasio::ip_address::from_string(ip, ec);
            if (!ec) {
                ep = uasio::endpoint(addr_ip, ntohs(addr->sin6_port));
            }
        }
        
        // 添加到结果集
        results.add(resolver_entry(ep, query.host(), query.service()));
    }

    // 释放解析结果
    freeaddrinfo(result);

    ec.clear();
    return results;
}

void resolver_service::async_resolve(const resolver_query& query, 
                                   std::function<void(const uasio::error_code&, resolver_results)>&& handler) {
    if (stopped_) {
        io_context_.post([handler]() {
            handler(uasio::make_error_code(uasio::error::operation_aborted), resolver_results());
        });
        return;
    }
    
    // 使用线程池异步执行解析操作
    io_context_.post([this, query, handler]() {
        uasio::error_code ec;
        resolver_results results = resolve(query, ec);
        handler(ec, results);
    });
}

} // namespace uasio 