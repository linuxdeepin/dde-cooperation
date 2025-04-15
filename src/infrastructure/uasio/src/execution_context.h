// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UASIO_EXECUTION_CONTEXT_H
#define UASIO_EXECUTION_CONTEXT_H

#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <type_traits>

namespace uasio {

class execution_context {
public:
    execution_context();
    
    execution_context(const execution_context&) = delete;
    execution_context& operator=(const execution_context&) = delete;
    
    virtual ~execution_context();
    
    // 创建服务
    template <typename Service>
    typename Service::key_type& use_service();
    
    // 检查服务是否已注册
    template <typename Service>
    bool has_service() const noexcept;
    
protected:
    // 通知所有服务关闭
    void shutdown();
    
    // 销毁所有服务
    void destroy();
    
private:
    // 服务基类
    class service_base {
    public:
        virtual ~service_base() = default;
        
        // 服务关闭
        virtual void shutdown() = 0;
    };
    
    // 服务键类型
    template <typename Key>
    class service_key {
    public:
        using type = service_key;
    };
    
    // 服务ID
    template <typename Service>
    struct service_id {
        static service_key<Service> id;
    };
    
    // 服务所有者
    class service_registry {
    public:
        service_registry(execution_context& owner);
        ~service_registry();
        
        // 获取服务
        template <typename Service>
        Service& use_service();
        
        // 添加服务
        template <typename Service>
        void add_service(Service* new_service);
        
        // 检查服务是否存在
        template <typename Service>
        bool has_service() const noexcept;
        
        // 关闭所有服务
        void shutdown_services();
        
        // 销毁所有服务
        void destroy_services();
        
    private:
        execution_context& owner_;
        std::mutex mutex_;
        std::vector<std::unique_ptr<service_base>> services_;
    };
    
    service_registry registry_;
};

} // namespace uasio

#endif // UASIO_EXECUTION_CONTEXT_H 