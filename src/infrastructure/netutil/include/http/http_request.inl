// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#if defined(FMT_VERSION)
template <> struct fmt::formatter<NetUtil::HTTP::HTTPRequest> : ostream_formatter {};
#endif

//! \cond DOXYGEN_SKIP
template <>
struct std::hash<NetUtil::HTTP::HTTPRequest>
{
    typedef NetUtil::HTTP::HTTPRequest argument_type;
    typedef size_t result_type;

    result_type operator() (const argument_type& value) const
    {
        result_type result = 17;
        result = result * 31 + std::hash<std::string>()(value.cache());
        return result;
    }
};
//! \endcond
