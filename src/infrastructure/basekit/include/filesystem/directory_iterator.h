// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_FILESYSTEM_DIRECTORY_ITERATOR_H
#define BASEKIT_FILESYSTEM_DIRECTORY_ITERATOR_H

#include "filesystem/path.h"

#include <memory>

namespace BaseKit {

//! Filesystem directory iterator
/*!
    Filesystem directory iterator is used to iterate through directory
    content (directories, files, symlinks).

    No sort order is guarantied!

    Not thread-safe.
*/
class DirectoryIterator
{
    friend class Directory;

public:
    // Standard constant iterator type definitions
    typedef std::ptrdiff_t difference_type;
    typedef Path value_type;
    typedef const Path& reference;
    typedef const Path* pointer;
    typedef std::forward_iterator_tag iterator_category;

    DirectoryIterator();
    DirectoryIterator(DirectoryIterator& it);
    DirectoryIterator(DirectoryIterator&& it) noexcept;
    ~DirectoryIterator();

    DirectoryIterator& operator=(DirectoryIterator& it);
    DirectoryIterator& operator=(DirectoryIterator&& it) noexcept;

    friend bool operator==(const DirectoryIterator& it1, const DirectoryIterator& it2) noexcept;
    friend bool operator!=(const DirectoryIterator& it1, const DirectoryIterator& it2) noexcept;

    DirectoryIterator& operator++();
    DirectoryIterator operator++(int);

    const Path& operator*() const noexcept;
    const Path* operator->() const noexcept;

    //! Swap two instances
    void swap(DirectoryIterator& it) noexcept;
    friend void swap(DirectoryIterator& it1, DirectoryIterator& it2) noexcept;

private:
    class Impl;
    class SimpleImpl;
    class RecursiveImpl;
    std::unique_ptr<Impl> _pimpl;
    Path _current;

    DirectoryIterator(const Path& current);
    DirectoryIterator(const Path& parent, bool recursive);
};

} // namespace BaseKit

#include "directory_iterator.inl"

#endif // BASEKIT_FILESYSTEM_DIRECTORY_ITERATOR_H
