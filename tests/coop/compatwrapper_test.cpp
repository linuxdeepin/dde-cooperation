// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QString>
#include "lib/cooperation/core/net/compatwrapper.h"

using cooperation_core::CompatWrapper;

// 单例身份:两次 instance() 返回同一指针。
TEST(CompatWrapperTest, InstanceReturnsSameSingleton)
{
    EXPECT_EQ(CompatWrapper::instance(), CompatWrapper::instance());
}

// session() 返回 d->sessionId;daemon 未连接时该字段保持空字符串。
// 构造函数启动的 IPC 定时器连接不到 daemon,不会写入 sessionId。
TEST(CompatWrapperTest, SessionIsEmptyBeforeBackendConnect)
{
    EXPECT_EQ(CompatWrapper::instance()->session().toStdString(), "");
}

// ipcInterface() 在构造时已由 CompatWrapperPrivate 创建,永不返回 nullptr。
TEST(CompatWrapperTest, IpcInterfaceReturnsNonNull)
{
    EXPECT_NE(CompatWrapper::instance()->ipcInterface(), nullptr);
}

// 多次调用 ipcInterface() 返回同一对象(构造时创建,非延迟分配)。
TEST(CompatWrapperTest, IpcInterfaceReturnsSamePointer)
{
    auto *a = CompatWrapper::instance()->ipcInterface();
    auto *b = CompatWrapper::instance()->ipcInterface();
    EXPECT_EQ(a, b);
}
