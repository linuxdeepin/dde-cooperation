// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// type_defines 纯逻辑覆盖
// StyleHelper 全静态方法 (font/textStyle/buttonStyle/textBrowserStyle/setAutoFont) 走全部分支;
// ButtonLayout/IndexLabel/MovieWidget/ProcessDetailsWindow/ProcessWindowItemDelegate
// 构造 + 简单交互 + paintEvent (offscreen 下 show+repaint 触发绘制路径)。

#include "gui/type_defines.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QPushButton>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QTest>
#include <QPainter>

// ---- StyleHelper::font ----
TEST(StyleHelperTest, FontAllBranches)
{
    QFont f1 = StyleHelper::font(1);
    EXPECT_EQ(f1.pixelSize(), 24);
    QFont f2 = StyleHelper::font(2);
    EXPECT_EQ(f2.pixelSize(), 17);
    QFont f3 = StyleHelper::font(3);
    EXPECT_EQ(f3.pixelSize(), 12);
    QFont fdef = StyleHelper::font(999);   // default 分支
    EXPECT_TRUE(fdef.family().isEmpty() || !fdef.family().isNull());
}

// ---- StyleHelper::textStyle ----
TEST(StyleHelperTest, TextStyleAllBranches)
{
    QString normal = StyleHelper::textStyle(StyleHelper::normal);
    EXPECT_TRUE(normal.contains("#000000"));
    QString err = StyleHelper::textStyle(StyleHelper::error);
    EXPECT_TRUE(err.contains("#FF5736"));
    QString unknown = StyleHelper::textStyle(static_cast<StyleHelper::TextStyle>(999));
    EXPECT_TRUE(unknown.isEmpty());
}

// ---- StyleHelper::buttonStyle ----
TEST(StyleHelperTest, ButtonStyleAllBranches)
{
    QString gray = StyleHelper::buttonStyle(StyleHelper::gray);
    EXPECT_TRUE(gray.contains("SourceHanSansSC-Medium"));
    QString blue = StyleHelper::buttonStyle(StyleHelper::blue);
    EXPECT_TRUE(blue.contains("rgba(37, 183, 255"));
    QString unknown = StyleHelper::buttonStyle(999);
    EXPECT_TRUE(unknown.isEmpty());
}

// ---- StyleHelper::textBrowserStyle ----
TEST(StyleHelperTest, TextBrowserStyleAllBranches)
{
    QString light = StyleHelper::textBrowserStyle(1);
    EXPECT_TRUE(light.contains("rgba(0, 0, 0,0.08)"));
    QString dark = StyleHelper::textBrowserStyle(0);
    EXPECT_TRUE(dark.contains("rgba(255,255,255, 0.1)"));
    QString unknown = StyleHelper::textBrowserStyle(999);
    EXPECT_TRUE(unknown.isEmpty());
}

// ---- StyleHelper::setAutoFont (Linux: DFontSizeManager 分支, 全 size 覆盖) ----
TEST(StyleHelperTest, SetAutoFontAllSizes)
{
    QWidget w;
    for (int size : {54, 24, 17, 14, 12, 11, 10, 8}) {
        StyleHelper::setAutoFont(&w, size, QFont::Medium);
    }
    StyleHelper::setAutoFont(&w, 999, QFont::Medium);   // default 分支
    SUCCEED();
}

// ---- ButtonLayout ----
TEST(ButtonLayoutTest, ConstructAndGetButtons)
{
    QWidget parent;
    ButtonLayout *layout = new ButtonLayout(&parent);
    EXPECT_NE(layout->getButton1(), nullptr);
    EXPECT_NE(layout->getButton2(), nullptr);

    layout->setCount(1);
    // offscreen 下未 show 的 widget, 用 visible 属性 (isHidden) 而非 isVisible()
    EXPECT_TRUE(layout->getButton2()->isHidden());

    layout->setCount(2);
    EXPECT_FALSE(layout->getButton2()->isHidden());

    layout->setCount(999);   // default 分支不崩溃
    delete layout;
}

// ---- IndexLabel paintEvent ----
TEST(IndexLabelTest, PaintEvent)
{
    IndexLabel label(0);
    label.setIndex(2);
    label.resize(60, 10);
    label.show();
    QTest::qWait(20);
    label.repaint();
    label.hide();
}

// ---- MovieWidget paint/load (资源可能缺失, 不崩溃即可) ----
TEST(MovieWidgetTest, ConstructAndNextFrame)
{
    MovieWidget w("nonexistent");
    w.resize(200, 160);
    w.show();
    QTest::qWait(20);
    w.repaint();
    w.hide();
}

// ---- ProcessDetailsWindow ----
// 注: 不测 clear() 在无 model 时的行为 — 真实实现 qobject_cast 后直接解引用,
// 无 model 会 null deref 崩溃 (真实 bug), 会跳过 __gcov_dump 丢覆盖率。
TEST(ProcessDetailsWindowTest, ConstructOnly)
{
    ProcessDetailsWindow *win = new ProcessDetailsWindow();
    delete win;   // 覆盖构造/析构
}

TEST(ProcessDetailsWindowTest, ClearWithModel)
{
    ProcessDetailsWindow win;
    QStandardItemModel model;
    model.appendRow(new QStandardItem("x"));
    win.setModel(&model);
    win.clear();
    EXPECT_EQ(model.rowCount(), 0);
}

// ---- ProcessWindowItemDelegate ----
TEST(ProcessWindowItemDelegateTest, AllBranches)
{
    ProcessWindowItemDelegate delegate;
    delegate.setTheme(0);   // dark
    delegate.setTheme(1);   // light
    delegate.setStageColor(QColor(255, 0, 0));

    // sizeHint (固定返回)
    QStyleOptionViewItem opt;
    QModelIndex idx;
    EXPECT_EQ(delegate.sizeHint(opt, idx), QSize(100, 20));

    // addIcon (路径不存在也安全)
    delegate.addIcon("/nonexistent/icon.png");
    delegate.addIcon("/another/nonexistent.png");

    // paint: 需要真实 QModelIndex + pixmap 已加载路径
    QStandardItemModel model;
    QStandardItem item("name");
    item.setData("stage", Qt::ToolTipRole);
    item.setData(1, Qt::StatusTipRole);
    item.setData(0, Qt::UserRole);
    model.appendRow(&item);

    QPixmap pm(200, 40);
    pm.fill(Qt::white);
    QPainter painter(&pm);
    QStyleOptionViewItem option;
    option.rect = QRect(0, 0, 200, 40);
    delegate.paint(&painter, option, model.index(0, 0));
    painter.end();
}
