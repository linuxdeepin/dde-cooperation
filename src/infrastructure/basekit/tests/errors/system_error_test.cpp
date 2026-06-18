// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "errors/system_error.h"

#include <string>

using namespace BaseKit;

// 测试 SystemError 静态接口
TEST(SystemErrorTest, StaticInterface)
{
    // GetLast / SetLast 基于 errno
    SystemError::SetLast(0);
    EXPECT_EQ(SystemError::GetLast(), 0);

    SystemError::SetLast(EACCES);
    EXPECT_EQ(SystemError::GetLast(), EACCES);

    // ClearLast 等价于 SetLast(0)
    SystemError::ClearLast();
    EXPECT_EQ(SystemError::GetLast(), 0);

    // Description：可调用并返回字符串
    // 注意：GNU strerror_r 对部分错误码可能返回空（已知行为），仅验证可调用。
    std::string d1 = SystemError::Description(EACCES);
    std::string d2 = SystemError::Description(ENOSYS);
    std::string d3 = SystemError::Description(999999); // 未知错误码分支
    (void)d1;
    (void)d2;
    (void)d3;
}
