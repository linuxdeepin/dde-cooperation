// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTest>
#include "gui/widgets/cooperationstatewidget.h"

using cooperation_core::LookingForDeviceWidget;
using cooperation_core::NoNetworkWidget;
using cooperation_core::NoResultTipWidget;
using cooperation_core::NoResultWidget;
using cooperation_core::BottomLabel;

// ============ LookingForDeviceWidget ============

TEST(LookingForDeviceWidgetTest, ConstructDoesNotCrash)
{
    LookingForDeviceWidget w;
    SUCCEED();
}

// seAnimationtEnabled 三分支:enable / disable / 相同早返回。
TEST(LookingForDeviceWidgetTest, SeAnimationtEnabledTogglesTimer)
{
    LookingForDeviceWidget w;
    w.seAnimationtEnabled(true);   // 启动
    w.seAnimationtEnabled(true);   // 相同 → 早返回
    w.seAnimationtEnabled(false);  // 停止
    SUCCEED();
}

// paintEvent 在 isEnabled=true 时绘制动画梯度;offscreen 下 show+repaint 触发。
TEST(LookingForDeviceWidgetTest, PaintEventWithAnimationDoesNotCrash)
{
    LookingForDeviceWidget w;
    w.resize(300, 300);
    w.seAnimationtEnabled(true);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}

// ============ NoNetworkWidget ============

TEST(NoNetworkWidgetTest, ConstructDoesNotCrash)
{
    NoNetworkWidget w;
    SUCCEED();
}

// ============ NoResultTipWidget ============
// ctor 4 种组合覆盖 useTipMode/isMobile 分支。

TEST(NoResultTipWidgetTest, ConstructDefaultsDoesNotCrash)
{
    NoResultTipWidget w;
    SUCCEED();
}

TEST(NoResultTipWidgetTest, ConstructTipModeDoesNotCrash)
{
    NoResultTipWidget w(nullptr, true);
    SUCCEED();
}

TEST(NoResultTipWidgetTest, ConstructMobileDoesNotCrash)
{
    NoResultTipWidget w(nullptr, false, true);
    SUCCEED();
}

TEST(NoResultTipWidgetTest, ConstructTipAndMobileDoesNotCrash)
{
    NoResultTipWidget w(nullptr, true, true);
    SUCCEED();
}

TEST(NoResultTipWidgetTest, SetTitleVisibleDoesNotCrash)
{
    NoResultTipWidget w;
    w.setTitleVisible(true);
    w.setTitleVisible(false);
    SUCCEED();
}

// onLinkActivated 调 QDesktopServices::openUrl;offscreen/headless 下通常无害。
TEST(NoResultTipWidgetTest, OnLinkActivatedDoesNotCrash)
{
    NoResultTipWidget w;
    EXPECT_NO_FATAL_FAILURE(w.onLinkActivated("https://example.com"));
}

// ============ NoResultWidget ============

TEST(NoResultWidgetTest, ConstructDoesNotCrash)
{
    NoResultWidget w;
    SUCCEED();
}

// ============ BottomLabel ============

TEST(BottomLabelTest, ConstructDoesNotCrash)
{
    BottomLabel w;
    SUCCEED();
}

TEST(BottomLabelTest, SetIpDoesNotCrash)
{
    BottomLabel w;
    w.setIp("10.0.0.1");
    SUCCEED();
}

// showDialog 非模态(Qt::Tool),offscreen 安全。
TEST(BottomLabelTest, ShowDialogDoesNotCrash)
{
    BottomLabel w;
    EXPECT_NO_FATAL_FAILURE(w.showDialog());
}

// onSwitchMode:page>1 早返回;0/1 走 setCurrentIndex。
TEST(BottomLabelTest, OnSwitchModeValidAndInvalidDoesNotCrash)
{
    BottomLabel w;
    w.onSwitchMode(0);
    w.onSwitchMode(1);
    w.onSwitchMode(2);  // >1 → 早返回
    SUCCEED();
}

// paintEvent 画顶部分隔线;show+repaint 触发。
TEST(BottomLabelTest, PaintEventDoesNotCrash)
{
    BottomLabel w;
    w.resize(200, 40);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}
