// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "errors/exceptions.h"
#include "errors/exceptions_handler.h"
#include "system/stack_trace.h"

#include <functional>

using namespace BaseKit;

// 测试安装自定义异常处理函数
TEST(ExceptionsHandlerTest, SetupHandler)
{
    bool invoked = false;
    std::function<void(const SystemException&, const StackTrace&)> handler =
        [&invoked](const SystemException&, const StackTrace&) { invoked = true; };

    // 安装有效的处理函数应正常返回
    ExceptionsHandler::SetupHandler(handler);
    SUCCEED();

    // 重新安装默认处理函数（恢复初始状态，避免影响其它测试）
    ExceptionsHandler::SetupHandler([](const SystemException&, const StackTrace&) {});
    SUCCEED();
}

// 测试进程级异常处理初始化（注册信号处理）
TEST(ExceptionsHandlerTest, SetupProcess)
{
    // 第一次调用：注册各信号处理
    EXPECT_NO_THROW(ExceptionsHandler::SetupProcess());

    // 第二次调用：由于已初始化，应直接返回（幂等）
    EXPECT_NO_THROW(ExceptionsHandler::SetupProcess());
}

// 测试线程级异常处理初始化（Unix 下为空实现，但调用不应抛异常）
TEST(ExceptionsHandlerTest, SetupThread)
{
    EXPECT_NO_THROW(ExceptionsHandler::SetupThread());
}
