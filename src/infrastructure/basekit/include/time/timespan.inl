// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline void Timespan::swap(Timespan& timespan) noexcept
{
    using std::swap;
    swap(_duration, timespan._duration);
}

inline void swap(Timespan& timespan1, Timespan& timespan2) noexcept
{
    timespan1.swap(timespan2);
}

} // namespace BaseKit

//! \cond DOXYGEN_SKIP
template <>
struct std::hash<BaseKit::Timespan>
{
    typedef BaseKit::Timespan argument_type;
    typedef size_t result_type;

    result_type operator() (const argument_type& value) const
    {
        result_type result = 17;
        result = result * 31 + std::hash<int64_t>()(value.total());
        return result;
    }
};
//! \endcond
