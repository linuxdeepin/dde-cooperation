// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


ENUM_FLAGS(BaseKit::FileAttributes)
ENUM_FLAGS(BaseKit::FilePermissions)

namespace BaseKit {

inline bool Path::IsOther() const
{
    FileType t = type();
    return ((t != FileType::NONE) && (t != FileType::REGULAR) && (t != FileType::DIRECTORY) && (t != FileType::SYMLINK));
}

inline Path& Path::Assign(const Path& path)
{
    _path = path._path;
    return *this;
}

inline Path& Path::Concat(const Path& path)
{
    _path.append(path._path);
    return *this;
}

inline void Path::swap(Path& path) noexcept
{
    using std::swap;
    swap(_path, path._path);
}

inline void swap(Path& path1, Path& path2) noexcept
{
    path1.swap(path2);
}

} // namespace BaseKit

#if defined(FMT_VERSION)
template <>
struct fmt::formatter<BaseKit::Path> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(const BaseKit::Path& value, FormatContext& ctx) const
    {
        return formatter<string_view>::format(value.string(), ctx);
    }
};
#endif
