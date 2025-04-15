// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


namespace BaseKit {

inline bool operator==(const DirectoryIterator& it1, const DirectoryIterator& it2) noexcept
{
    return it1._current == it2._current;
}

inline bool operator!=(const DirectoryIterator& it1, const DirectoryIterator& it2) noexcept
{
    return it1._current != it2._current;
}

inline const Path& DirectoryIterator::operator*() const noexcept
{
    return _current;
}

inline const Path* DirectoryIterator::operator->() const noexcept
{
    return &_current;
}

inline void DirectoryIterator::swap(DirectoryIterator& it) noexcept
{
    using std::swap;
    _pimpl.swap(it._pimpl);
}

inline void swap(DirectoryIterator& it1, DirectoryIterator& it2) noexcept
{
    it1.swap(it2);
}

} // namespace BaseKit
