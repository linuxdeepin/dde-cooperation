// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

inline void Directory::swap(Directory& directory) noexcept
{
    using std::swap;
    Path::swap(directory);
}

inline void swap(Directory& directory1, Directory& directory2) noexcept
{
    directory1.swap(directory2);
}

} // namespace BaseKit
