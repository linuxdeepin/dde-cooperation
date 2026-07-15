// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "system/console.h"

#include <iostream>
#include <sstream>
#include <utility>

using namespace BaseKit;

TEST(ConsoleTest, SetColorDoesNotThrow)
{
    EXPECT_NO_THROW(Console::SetColor(Color::RED));
    EXPECT_NO_THROW(Console::SetColor(Color::GREEN, Color::BLACK));
    EXPECT_NO_THROW(Console::SetColor(Color::WHITE, Color::BLUE));
}

TEST(ConsoleTest, SetColorCoversRepresentativeIndices)
{
    const Color samples[] = {
        Color::BLACK, Color::BLUE, Color::GREEN, Color::CYAN,
        Color::RED, Color::MAGENTA, Color::BROWN, Color::GREY,
        Color::DARKGREY, Color::LIGHTBLUE, Color::LIGHTGREEN, Color::LIGHTCYAN,
        Color::LIGHTRED, Color::LIGHTMAGENTA, Color::YELLOW, Color::WHITE
    };
    for (Color fg : samples)
    {
        EXPECT_NO_THROW(Console::SetColor(fg));
        for (Color bg : samples)
            EXPECT_NO_THROW(Console::SetColor(fg, bg));
    }
}

TEST(ConsoleTest, StreamOperatorWithColorReturnsStream)
{
    std::ostringstream oss;
    auto& result = (oss << Color::YELLOW);
    EXPECT_EQ(&result, &oss);
}

TEST(ConsoleTest, StreamOperatorWithColorPairReturnsStream)
{
    std::ostringstream oss;
    auto& result = (oss << std::make_pair(Color::RED, Color::GREEN));
    EXPECT_EQ(&result, &oss);
}

TEST(ConsoleTest, StreamOperatorChaining)
{
    std::ostringstream oss;
    oss << Color::RED << "text" << Color::WHITE;
    EXPECT_EQ(oss.str(), "text");
}
