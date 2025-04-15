// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

inline void Symlink::swap(Symlink& symlink) noexcept
{
    using std::swap;
    Path::swap(symlink);
}

inline void swap(Symlink& symlink1, Symlink& symlink2) noexcept
{
    symlink1.swap(symlink2);
}

} // namespace BaseKit
