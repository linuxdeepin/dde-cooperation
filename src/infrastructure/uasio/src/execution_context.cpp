// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "execution_context.h"

namespace uasio {

execution_context::execution_context()
    : registry_(*this)
{
}

execution_context::~execution_context()
{
    shutdown();
    destroy();
}

void execution_context::shutdown()
{
    registry_.shutdown_services();
}

void execution_context::destroy()
{
    registry_.destroy_services();
}

execution_context::service_registry::service_registry(execution_context& owner)
    : owner_(owner)
{
}

execution_context::service_registry::~service_registry()
{
}

void execution_context::service_registry::shutdown_services()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从后向前关闭服务，以确保服务依赖关系得到尊重
    for (auto it = services_.rbegin(); it != services_.rend(); ++it) {
        (*it)->shutdown();
    }
}

void execution_context::service_registry::destroy_services()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 清空服务列表
    services_.clear();
}

} // namespace uasio 