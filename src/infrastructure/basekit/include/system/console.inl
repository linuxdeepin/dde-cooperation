// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

template <class TOutputStream>
inline TOutputStream& operator<<(TOutputStream& stream, Color color)
{
    Console::SetColor(color);
    return stream;
}

template <class TOutputStream>
inline TOutputStream& operator<<(TOutputStream& stream, std::pair<Color, Color> colors)
{
    Console::SetColor(colors.first, colors.second);
    return stream;
}

} // namespace BaseKit
