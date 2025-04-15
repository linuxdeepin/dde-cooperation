// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

template <typename... T>
inline std::string format(fmt::format_string<T...> pattern, T&&... args)
{
    return fmt::vformat(pattern, fmt::make_format_args(args...));
}

template <typename... T>
inline std::wstring format(fmt::wformat_string<T...> pattern, T&&... args)
{
    return fmt::vformat<wchar_t>(pattern, fmt::make_format_args<fmt::wformat_context>(args...));
}

template <typename... T>
inline void print(fmt::format_string<T...> pattern, T&&... args)
{
    return fmt::vprint(pattern, fmt::make_format_args(args...));
}

template <typename... T>
inline void print(fmt::wformat_string<T...> pattern, T&&... args)
{
    return fmt::vprint<wchar_t>(pattern, fmt::make_format_args<fmt::wformat_context>(args...));
}

template <typename TOutputStream, typename... T>
inline void print(TOutputStream& stream, fmt::format_string<T...> pattern, T&&... args)
{
    return fmt::vprint(stream, pattern, fmt::make_format_args(args...));
}

template <typename TOutputStream, typename... T>
inline void print(TOutputStream& stream, fmt::wformat_string<T...> pattern, T&&... args)
{
    return fmt::vprint<wchar_t>(stream, pattern, fmt::make_format_args<fmt::wformat_context>(args...));
}

} // namespace BaseKit

//! @cond INTERNALS

using namespace fmt::literals;

//! @endcond
