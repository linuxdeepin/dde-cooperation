// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QLabel>
#include <QWidget>
#include <QPalette>
#include <QColor>
#include <QList>
#include <QFont>
#include "gui/utils/cooperationguihelper.h"

using cooperation_core::CooperationGuiHelper;

// Singleton instance should be non-null and stable across calls.
TEST(CooperationGuiHelperTest, InstanceReturnsSameSingleton)
{
    auto *first = CooperationGuiHelper::instance();
    auto *second = CooperationGuiHelper::instance();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, second);
}

// Boundary: colorList.size() != 2 must return false and not touch the widget.
TEST(CooperationGuiHelperTest, AutoUpdateTextColorReturnsFalseOnWrongColorCount)
{
    auto *helper = CooperationGuiHelper::instance();
    ASSERT_NE(helper, nullptr);
    QWidget w;
    QList<QColor> oneColor{Qt::black};
    EXPECT_FALSE(helper->autoUpdateTextColor(&w, oneColor));

    QList<QColor> threeColors{Qt::black, Qt::white, Qt::red};
    EXPECT_FALSE(helper->autoUpdateTextColor(&w, threeColors));

    QList<QColor> empty;
    EXPECT_FALSE(helper->autoUpdateTextColor(&w, empty));
}

// Two colors should return true and apply one of them to WindowText.
TEST(CooperationGuiHelperTest, AutoUpdateTextColorReturnsTrueOnTwoColors)
{
    auto *helper = CooperationGuiHelper::instance();
    QWidget w;
    QList<QColor> twoColors{Qt::black, Qt::white};
    EXPECT_TRUE(helper->autoUpdateTextColor(&w, twoColors));
    const QColor applied = w.palette().color(QPalette::WindowText);
    EXPECT_TRUE(applied == Qt::black || applied == Qt::white);
}

// Static setFontColor should set the palette WindowText color exactly.
TEST(CooperationGuiHelperTest, SetFontColorChangesPaletteWindowText)
{
    QWidget w;
    CooperationGuiHelper::setFontColor(&w, Qt::red);
    EXPECT_EQ(w.palette().color(QPalette::WindowText), QColor(Qt::red));
}

// setLabelFont uses setPixelSize internally (verified in source), so check pixelSize.
TEST(CooperationGuiHelperTest, SetLabelFontAppliesPixelSize)
{
    QLabel label;
    CooperationGuiHelper::setLabelFont(&label, 20, 10, QFont::Bold);
    EXPECT_EQ(label.font().pixelSize(), 20);
}

// setAutoFont binds via DFontSizeManager; the resulting font should have a
// non-default pixel size (> 0). We avoid asserting an exact DTK T-size value
// since it can vary with DTK configuration; we only verify a font was applied.
TEST(CooperationGuiHelperTest, SetAutoFontAppliesNonDefaultFont)
{
    QWidget w;
    const int beforePixel = w.font().pixelSize();
    CooperationGuiHelper::setAutoFont(&w, 16, QFont::Normal);
    // DFontSizeManager::bind sets a pixel size on the widget's font.
    EXPECT_GT(w.font().pixelSize(), 0);
    // The font should differ from the default unless default already matched.
    if (beforePixel > 0) {
        EXPECT_NE(w.font().pixelSize(), beforePixel);
    }
}

// isDarkTheme is static and should not crash under offscreen platform.
TEST(CooperationGuiHelperTest, IsDarkThemeDoesNotCrash)
{
    // Just exercise the API; offscreen defaults to light theme.
    bool dark = CooperationGuiHelper::isDarkTheme();
    (void)dark;  // value depends on DTK; we only assert it doesn't crash.
    SUCCEED();
}
