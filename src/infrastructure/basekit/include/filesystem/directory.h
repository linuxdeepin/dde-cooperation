// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_FILESYSTEM_DIRECTORY_H
#define BASEKIT_FILESYSTEM_DIRECTORY_H

#include "filesystem/directory_iterator.h"
#include "filesystem/file.h"
#include "filesystem/symlink.h"

namespace BaseKit {

//! Filesystem directory
/*!
    Filesystem directory wraps directory management operations (create, remove, iterate).

    Not thread-safe.
*/
class Directory : public Path
{
public:
    //! Default directory attributes (Normal)
    static const Flags<FileAttributes> DEFAULT_ATTRIBUTES;
    //! Default directory permissions (IRUSR | IWUSR | IXUSR | IRGRP | IXGRP | IROTH | IXOTH)
    static const Flags<FilePermissions> DEFAULT_PERMISSIONS;

    //! Initialize directory with an empty path
    Directory() : Path() {}
    //! Initialize directory with a given path
    /*!
        \param path - Directory path
    */
    Directory(const Path& path) : Path(path) {}
    Directory(const Directory&) = default;
    Directory(Directory&&) = default;
    ~Directory() = default;

    Directory& operator=(const Path& path)
    { Assign(path); return *this; }
    Directory& operator=(const Directory&) = default;
    Directory& operator=(Directory&&) = default;

    //! Check if the directory exist
    explicit operator bool() const noexcept { return IsDirectoryExists(); }

    //! Is the directory exists?
    bool IsDirectoryExists() const;
    //! Is the directory empty?
    bool IsDirectoryEmpty() const;

    //! Get the directory begin iterator
    DirectoryIterator begin() const;
    //! Get the directory end iterator
    DirectoryIterator end() const;

    //! Get the directory recursive begin iterator
    DirectoryIterator rbegin() const;
    //! Get the directory recursive end iterator
    DirectoryIterator rend() const;

    //! Get all entries (directories, files, symbolic links) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Entries collection
    */
    std::vector<Path> GetEntries(const std::string& pattern = "");
    //! Recursively get all entries (directories, files, symbolic links) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Entries collection
    */
    std::vector<Path> GetEntriesRecursive(const std::string& pattern = "");

    //! Get all directories (including symbolic link directories) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Directories collection
    */
    std::vector<Directory> GetDirectories(const std::string& pattern = "");
    //! Recursively get all directories (including symbolic link directories) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Directories collection
    */
    std::vector<Directory> GetDirectoriesRecursive(const std::string& pattern = "");

    //! Get all files (including symbolic link files) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Files collection
    */
    std::vector<File> GetFiles(const std::string& pattern = "");
    //! Recursively get all files (including symbolic link files) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Files collection
    */
    std::vector<File> GetFilesRecursive(const std::string& pattern = "");

    //! Get all symbolic links (including symbolic link directories) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Symbolic links collection
    */
    std::vector<Symlink> GetSymlinks(const std::string& pattern = "");
    //! Recursively get all symbolic links (including symbolic link directories) in the current directory
    /*!
        \param pattern - Regular expression pattern (default is "")
        \return Symbolic links collection
    */
    std::vector<Symlink> GetSymlinksRecursive(const std::string& pattern = "");

    //! Create directory from the given path
    /*!
        \param path - Directory path
        \param attributes - Directory attributes (default is Directory::DEFAULT_ATTRIBUTES)
        \param permissions - Directory permissions (default is Directory::DEFAULT_PERMISSIONS)
        \return Created directory
    */
    static Directory Create(const Path& path, const Flags<FileAttributes>& attributes = Directory::DEFAULT_ATTRIBUTES, const Flags<FilePermissions>& permissions = Directory::DEFAULT_PERMISSIONS);
    //! Create full directory tree of the given path
    /*!
        \param path - Directory path
        \param attributes - Directory attributes (default is Directory::DEFAULT_ATTRIBUTES)
        \param permissions - Directory permissions (default is Directory::DEFAULT_PERMISSIONS)
        \return Created full directory tree
    */
    static Directory CreateTree(const Path& path, const Flags<FileAttributes>& attributes = Directory::DEFAULT_ATTRIBUTES, const Flags<FilePermissions>& permissions = Directory::DEFAULT_PERMISSIONS);

    //! Swap two instances
    void swap(Directory& directory) noexcept;
    friend void swap(Directory& directory1, Directory& directory2) noexcept;
};


} // namespace BaseKit

#include "directory.inl"

#endif // BASEKIT_FILESYSTEM_DIRECTORY_H
