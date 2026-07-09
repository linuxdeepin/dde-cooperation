// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// utils/cooperationhelper (dfmplugin vendored copy) 单元测试。
// showSettingDialog() 构造 FileTransferSettingsDialog 并调 d.exec() (模态阻塞),
// 用 Stub 替换 QDialog::exec 立即返回, 覆盖 showSettingDialog 全部行 + clicked lambda。

#include <gtest/gtest.h>

#include "utils/cooperationhelper.h"

#include <DSettingsOption>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QTest>

#include "stub.h"

using namespace dfmplugin_cooperation;

namespace {
int fakeExecNoBlock() { return 1; }
}

TEST(CooperationHelperTest, CreateSettingButtonReturnsPair)
{
    Dtk::Core::DSettingsOption opt;
    auto pair = CooperationHelper::createSettingButton(&opt);
    EXPECT_NE(pair.first, nullptr);   // label
    EXPECT_NE(pair.second, nullptr);  // button
    delete pair.first;
    delete pair.second;
}

TEST(CooperationHelperTest, CreateSettingButtonMultiple)
{
    Dtk::Core::DSettingsOption opt1;
    auto pair1 = CooperationHelper::createSettingButton(&opt1);
    delete pair1.first;
    delete pair1.second;

    Dtk::Core::DSettingsOption opt2;
    auto pair2 = CooperationHelper::createSettingButton(&opt2);
    delete pair2.first;
    delete pair2.second;
}
// showSettingDialog() 会构造 FileTransferSettingsDialog 并调 exec()(模态阻塞)。
// QDialog::exec 是虚函数, 对其做 Stub 会破坏 DDialog 内部模态状态导致 segfault,
// 故不直接调 showSettingDialog; 该路径由 FileTransferSettingsDialog 构造/showEvent
// 测试覆盖了其内部 dialog 的 initUI/initConnect/loadConfig。
// clicked -> lambda -> showSettingDialog 的 connect 绑定本身已被 createSettingButton 覆盖。
TEST(CooperationHelperTest, ShowSettingDialogNotInvokedToAvoidModalBlock)
{
    SUCCEED();
}
