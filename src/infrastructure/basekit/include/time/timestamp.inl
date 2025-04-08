// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline void Timestamp::swap(Timestamp& timestamp) noexcept
{
    using std::swap;
    swap(_timestamp, timestamp._timestamp);
}

inline void swap(Timestamp& timestamp1, Timestamp& timestamp2) noexcept
{
    timestamp1.swap(timestamp2);
}

} // namespace BaseKit

//! \cond DOXYGEN_SKIP
template <>
struct std::hash<BaseKit::Timestamp>
{
    typedef BaseKit::Timestamp argument_type;
    typedef size_t result_type;

    result_type operator() (const argument_type& value) const
    {
        result_type result = 17;
        result = result * 31 + std::hash<int64_t>()(value.total());
        return result;
    }
};
//! \endcond
