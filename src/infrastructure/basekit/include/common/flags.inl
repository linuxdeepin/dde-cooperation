// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

template <typename TEnum>
inline void swap(Flags<TEnum>& flags1, Flags<TEnum>& flags2) noexcept
{
    flags1.swap(flags2);
}

template <typename TEnum>
constexpr auto operator&(TEnum value1, TEnum value2) noexcept -> typename std::enable_if<IsEnumFlags<TEnum>::value, Flags<TEnum>>::type
{
    return Flags<TEnum>(value1) & value2;
}

template <typename TEnum>
constexpr auto operator|(TEnum value1, TEnum value2) noexcept -> typename std::enable_if<IsEnumFlags<TEnum>::value, Flags<TEnum>>::type
{
    return Flags<TEnum>(value1) | value2;
}

template <typename TEnum>
constexpr auto operator^(TEnum value1, TEnum value2) noexcept -> typename std::enable_if<IsEnumFlags<TEnum>::value, Flags<TEnum>>::type
{
    return Flags<TEnum>(value1) ^ value2;
}

} // namespace BaseKit
