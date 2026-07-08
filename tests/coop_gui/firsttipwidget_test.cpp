// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTest>
#include <QEvent>
#include <QHideEvent>
#include <QResizeEvent>
#include <QCoreApplication>
#include "gui/widgets/firsttipwidget.h"

using cooperation_core::FirstTipWidget;
using cooperation_core::LineWidget;
using cooperation_core::ElidedLabel;

TEST(FirstTipWidgetTest, ConstructDoesNotCrash)
{
    FirstTipWidget w;
    SUCCEED();
}

// themeTypeChanged() 公共方法,读 isDarkTheme 切换明暗分支。
TEST(FirstTipWidgetTest, ThemeTypeChangedDoesNotCrash)
{
    FirstTipWidget w;
    EXPECT_NO_FATAL_FAILURE(w.themeTypeChanged());
}

// showEvent 调 drawLine + 定位 close 按钮;hideEvent/resizeEvent 经 show/sendEvent。
TEST(FirstTipWidgetTest, ShowHideResizeEventsDoNotCrash)
{
    FirstTipWidget w;
    w.resize(400, 300);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());

    QHideEvent hide;
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&w, &hide));

    QResizeEvent resize(QSize(500, 400), QSize(400, 300));
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&w, &resize));
}

// ============ LineWidget ============

TEST(LineWidgetTest, PaintEventDoesNotCrash)
{
    LineWidget w;
    w.resize(200, 4);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}

// ============ ElidedLabel ============

TEST(ElidedLabelTest, ConstructAndPaintDoesNotCrash)
{
    ElidedLabel label("some long text here", 100);
    label.resize(120, 20);
    label.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(label.repaint());
}

TEST(ElidedLabelTest, ShortTextPaintDoesNotCrash)
{
    ElidedLabel label("hi", 200);
    label.resize(120, 20);
    EXPECT_NO_FATAL_FAILURE(label.repaint());
}
