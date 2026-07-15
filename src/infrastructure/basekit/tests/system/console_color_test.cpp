// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "system/console.h"

#include <sstream>
#include <utility>

using namespace BaseKit;

TEST(ConsoleColorTest, SetColorDoesNotThrow)
{
    EXPECT_NO_THROW(Console::SetColor(Color::GREEN));
    EXPECT_NO_THROW(Console::SetColor(Color::RED, Color::BLACK));
    EXPECT_NO_THROW(Console::SetColor(Color::YELLOW, Color::BLUE));
}

TEST(ConsoleColorTest, OutputStreamColorManipulator)
{
    std::ostringstream os;
    os << Color::CYAN << "text";
    EXPECT_NE(os.str().find("text"), std::string::npos);
}

TEST(ConsoleColorTest, OutputStreamColorPairManipulator)
{
    std::ostringstream os;
    os << std::make_pair(Color::WHITE, Color::BLACK) << "pair";
    EXPECT_NE(os.str().find("pair"), std::string::npos);
}

TEST(ConsoleColorTest, SetAllBasicColorsNoThrow)
{
    EXPECT_NO_THROW(Console::SetColor(Color::BLACK));
    EXPECT_NO_THROW(Console::SetColor(Color::BLUE));
    EXPECT_NO_THROW(Console::SetColor(Color::MAGENTA));
    EXPECT_NO_THROW(Console::SetColor(Color::GREY));
    EXPECT_NO_THROW(Console::SetColor(Color::WHITE));
}
