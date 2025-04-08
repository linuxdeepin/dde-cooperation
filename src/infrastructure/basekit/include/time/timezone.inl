// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline void Timezone::swap(Timezone& timezone) noexcept
{
    using std::swap;
    swap(_name, timezone._name);
    swap(_offset, timezone._offset);
    swap(_dstoffset, timezone._dstoffset);
}

inline void swap(Timezone& timezone1, Timezone& timezone2) noexcept
{
    timezone1.swap(timezone2);
}

} // namespace BaseKit

//! \cond DOXYGEN_SKIP
template <>
struct std::hash<BaseKit::Timezone>
{
    typedef BaseKit::Timezone argument_type;
    typedef size_t result_type;

    result_type operator() (const argument_type& value) const
    {
        result_type result = 17;
        result = result * 31 + std::hash<std::string>()(value.name());
        result = result * 31 + std::hash<BaseKit::Timespan>()(value.offset());
        result = result * 31 + std::hash<BaseKit::Timespan>()(value.daylight());
        return result;
    }
};
//! \endcond
