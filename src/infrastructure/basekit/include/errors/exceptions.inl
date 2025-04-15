// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#if defined(FMT_VERSION)
template <>
struct fmt::formatter<BaseKit::Exception> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const BaseKit::Exception& value, FormatContext& ctx) const
    {
        return formatter<string_view>::format(value.string(), ctx);
    }
};
#endif
