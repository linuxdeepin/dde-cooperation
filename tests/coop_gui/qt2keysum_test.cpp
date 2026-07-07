// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <Qt>
// rfb keysym 定义
#include <rfb/keysym.h>
// 纯函数头 (无 namespace)
#include "gui/phone/qt2keysum.h"

TEST(Qt2KeysumTest, MapsLetterKeysToX11Keysyms)
{
    EXPECT_EQ(qt2keysym(Qt::Key_A), XK_a);
    EXPECT_EQ(qt2keysym(Qt::Key_Z), XK_z);
}

TEST(Qt2KeysumTest, MapsDigitKeys)
{
    EXPECT_EQ(qt2keysym(Qt::Key_0), XK_0);
    EXPECT_EQ(qt2keysym(Qt::Key_9), XK_9);
}

TEST(Qt2KeysumTest, MapsEnterAndBackspace)
{
    // 实际 switch: Key_Return -> XK_Return, Key_Enter (小键盘) -> XK_KP_Enter
    EXPECT_EQ(qt2keysym(Qt::Key_Return), XK_Return);
    EXPECT_EQ(qt2keysym(Qt::Key_Backspace), XK_BackSpace);
}

TEST(Qt2KeysumTest, MapsArrowKeys)
{
    EXPECT_EQ(qt2keysym(Qt::Key_Up), XK_Up);
    EXPECT_EQ(qt2keysym(Qt::Key_Down), XK_Down);
    EXPECT_EQ(qt2keysym(Qt::Key_Left), XK_Left);
    EXPECT_EQ(qt2keysym(Qt::Key_Right), XK_Right);
}

TEST(Qt2KeysumTest, MapsFunctionKeys)
{
    EXPECT_EQ(qt2keysym(Qt::Key_F1), XK_F1);
    EXPECT_EQ(qt2keysym(Qt::Key_F12), XK_F12);
}

TEST(Qt2KeysumTest, MapsModifiersAndSpecialKeys)
{
    // 实际 switch: Key_Shift -> XK_Shift_L, Key_Control -> XK_Control_L
    EXPECT_EQ(qt2keysym(Qt::Key_Shift), XK_Shift_L);
    EXPECT_EQ(qt2keysym(Qt::Key_Control), XK_Control_L);
    EXPECT_EQ(qt2keysym(Qt::Key_Space), XK_space);
    EXPECT_EQ(qt2keysym(Qt::Key_Escape), XK_Escape);
    // Key_Enter (小键盘 Enter) 映射到 XK_KP_Enter,而非 XK_Return
    EXPECT_EQ(qt2keysym(Qt::Key_Enter), XK_KP_Enter);
}

TEST(Qt2KeysumTest, UnknownKeyReturnsMinusOne)
{
    EXPECT_EQ(qt2keysym(Qt::Key_unknown), -1);
}
