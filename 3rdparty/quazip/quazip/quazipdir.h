// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZIPDIR_H
#define ZIPDIR_H

class QuaZipDirPrivate;

#include "quazip.h"
#include "quazipfileinfo.h"
#include <QDir>
#include <QList>
#include <QSharedDataPointer>

// Class for ZIP archive navigation
// Provides QDir-like interface for ZIP files
class DLL_EXPORT QuaZipDir {
private:
    QSharedDataPointer<QuaZipDirPrivate> d;
public:
    // Copy constructor
    QuaZipDir(const QuaZipDir &that);
    // Constructor with QuaZip instance and optional directory path
    QuaZipDir(QuaZip *zip, const QString &dir = QString());
    // Destructor
    ~QuaZipDir();
    // Assignment operator
    bool operator==(const QuaZipDir &that);
    // Inequality operator
    inline bool operator!=(const QuaZipDir &that) {return !operator==(that);}
    // Equality operator
    QuaZipDir& operator=(const QuaZipDir &that);
    // Get entry name by position
    QString operator[](int pos) const;
    // Get case sensitivity mode
    QuaZip::CaseSensitivity caseSensitivity() const;
    // Change current directory
    bool cd(const QString &dirName);
    // Go to parent directory
    bool cdUp();
    // Get entry count
    uint count() const;
    // Get directory name
    QString dirName() const;
    // Get entry info list with filters
    QList<QuaZipFileInfo> entryInfoList(const QStringList &nameFilters,
        QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    // Get entry info list
    QList<QuaZipFileInfo> entryInfoList(QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    // Get entry info list with zip64 support
    QList<QuaZipFileInfo64> entryInfoList64(const QStringList &nameFilters,
        QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    // Get entry info list with zip64 support (no filters)
    QList<QuaZipFileInfo64> entryInfoList64(QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    // Get entry name list with filters
    QStringList entryList(const QStringList &nameFilters,
        QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    // Get entry name list
    QStringList entryList(QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    // Check if entry exists
    bool exists(const QString &fileName) const;
    // Check if directory exists
    bool exists() const;
    // Get full file path
    QString filePath(const QString &fileName) const;
    // Get default filter
    QDir::Filters filter();
    // Check if at root directory
    bool isRoot() const;
    // Get default name filters
    QStringList nameFilters() const;
    // Get current path
    QString path() const;
    // Get relative file path
    QString relativeFilePath(const QString &fileName) const;
    // Set case sensitivity mode
    void setCaseSensitivity(QuaZip::CaseSensitivity caseSensitivity);
    // Set default filter
    void setFilter(QDir::Filters filters);
    // Set default name filters
    void setNameFilters(const QStringList &nameFilters);
    // Set path without checking existence
    void setPath(const QString &path);
    // Set default sorting mode
    void setSorting(QDir::SortFlags sort);
    // Get default sorting mode
    QDir::SortFlags sorting() const;
};

#endif
