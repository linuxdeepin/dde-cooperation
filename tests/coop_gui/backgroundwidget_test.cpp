// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTest>
#include "gui/widgets/backgroundwidget.h"

// BackgroundWidget 在全局命名空间(QFrame)。

// paintEvent 按 RoundRole(NoRole/Top/Bottom/TopAndBottom)走不同圆角路径;
// 经 setBackground 设 role + show + repaint 触发各分支。

TEST(BackgroundWidgetTest, setBackgroundTopAndBottomPaintDoesNotCrash)
{
    BackgroundWidget w;
    w.setBackground(8, BackgroundWidget::NoType, BackgroundWidget::TopAndBottom);
    w.resize(100, 40);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}

TEST(BackgroundWidgetTest, setBackgroundTopPaintDoesNotCrash)
{
    BackgroundWidget w;
    w.setBackground(8, BackgroundWidget::NoType, BackgroundWidget::Top);
    w.resize(100, 40);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}

TEST(BackgroundWidgetTest, setBackgroundBottomPaintDoesNotCrash)
{
    BackgroundWidget w;
    w.setBackground(8, BackgroundWidget::NoType, BackgroundWidget::Bottom);
    w.resize(100, 40);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}

TEST(BackgroundWidgetTest, setBackgroundNoRolePaintDoesNotCrash)
{
    BackgroundWidget w;
    w.setBackground(8, BackgroundWidget::NoType, BackgroundWidget::NoRole);
    w.resize(100, 40);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}

// ItemBackground 颜色类型分支。
TEST(BackgroundWidgetTest, setBackgroundItemBackgroundPaintDoesNotCrash)
{
    BackgroundWidget w;
    w.setBackground(17, BackgroundWidget::ItemBackground, BackgroundWidget::TopAndBottom);
    w.resize(100, 40);
    w.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(w.repaint());
}
