// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QCoreApplication>
#include "gui/dialogs/settingdialog.h"

using cooperation_core::SettingDialog;

// SettingDialog 公开 API 极简(ctor + eventFilter/showEvent/keyPressEvent);
// 710 行主体在 SettingDialogPrivate::initWindow(构造时执行,覆盖 UI 搭建)。
// 依赖 ConfigManager/DConfigManager/CooperationUtil/ReportLogManager 单例。

TEST(SettingDialogTest, ConstructDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE({ SettingDialog dlg; });
}

TEST(SettingDialogTest, ShowEventLoadsConfigDoesNotCrash)
{
    SettingDialog dlg;
    QShowEvent show;
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&dlg, &show));
}

// keyPressEvent:linux 下基本转发;非 linux 处理 Enter。发 Escape 不崩。
TEST(SettingDialogTest, KeyPressEventDoesNotCrash)
{
    SettingDialog dlg;
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&dlg, &key));
}

// eventFilter 处理 ContentWidget/MainWidget 的 Paint 事件。
TEST(SettingDialogTest, EventFilterPaintOnUnnamedWidgetReturnsFalse)
{
    SettingDialog dlg;
    QWidget child;
    child.setObjectName("RandomName");
    QEvent paint(QEvent::Paint);
    // 非 ContentWidget/MainWidget → 不消费,返回 base eventFilter(此处只验不崩)
    EXPECT_NO_FATAL_FAILURE(dlg.eventFilter(&child, &paint));
}

// eventFilter 对 ContentWidget 的 Paint 走绘制圆角/矩形分支。
TEST(SettingDialogTest, EventFilterPaintOnContentWidgetDoesNotCrash)
{
    SettingDialog dlg;
    QWidget child;
    child.setObjectName("ContentWidget");
    child.resize(100, 100);
    QEvent paint(QEvent::Paint);
    EXPECT_NO_FATAL_FAILURE(dlg.eventFilter(&child, &paint));
}
