// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QTest>
#include <QLabel>
#include "gui/widgets/settingitem.h"

using cooperation_core::SettingItem;

TEST(SettingItemTest, ConstructDoesNotCrash)
{
    SettingItem item;
    SUCCEED();
}

TEST(SettingItemTest, SetItemInfoShortTextDoesNotCrash)
{
    SettingItem item;
    QLabel w;
    item.setItemInfo("short", &w);
    SUCCEED();
}

// 长文本触发 QFontMetrics::elidedText 截断 + setToolTip 分支。
TEST(SettingItemTest, SetItemInfoLongTextTruncates)
{
    SettingItem item;
    QLabel w;
    QString longText(200, QChar('X'));
    EXPECT_NO_FATAL_FAILURE(item.setItemInfo(longText, &w));
}

// paintEvent 圆角路径填充;show+repaint 触发。
TEST(SettingItemTest, PaintEventDoesNotCrash)
{
    SettingItem item;
    item.resize(200, 48);
    item.show();
    QTest::qWait(20);
    EXPECT_NO_FATAL_FAILURE(item.repaint());
}
