// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

inline std::ostream& operator<<(std::ostream& os, const SourceLocation& source_location)
{
    if ((source_location.filename() == nullptr) || (source_location.line() == 0))
        return os;

    return os << source_location.filename() << ':' << source_location.line();
}

} // namespace BaseKit

#if defined(FMT_VERSION)
template <> struct fmt::formatter<BaseKit::SourceLocation> : ostream_formatter {};
#endif
