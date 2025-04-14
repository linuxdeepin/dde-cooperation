// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUA_ZIPFILEINFO_H
#define QUA_ZIPFILEINFO_H

#include <QByteArray>
#include <QDateTime>
#include <QFile>

#include "exportdefine.h"

// Deprecated file info structure (use QuaZipFileInfo64 instead)
struct DLL_EXPORT QuaZipFileInfo {
  // File name
  QString name;
  // Version created by
  quint16 versionCreated;
  // Version needed to extract
  quint16 versionNeeded;
  // General purpose flags
  quint16 flags;
  // Compression method
  quint16 method;
  /// Last modification date and time.
  QDateTime dateTime;
  // CRC
  quint32 crc;
  // Compressed file size
  quint32 compressedSize;
  // Uncompressed file size
  quint32 uncompressedSize;
  // Disk number start
  quint16 diskNumberStart;
  // Internal file attributes
  quint16 internalAttr;
  // External file attributes
  quint32 externalAttr;
  // Comment
  QString comment;
  // Extra field
  QByteArray extra;
  // Get file permissions from external attributes
  QFile::Permissions getPermissions() const;
};

// File info structure with zip64 support
struct DLL_EXPORT QuaZipFileInfo64 {
  // File name
  QString name;
  // Version created by
  quint16 versionCreated;
  // Version needed to extract
  quint16 versionNeeded;
  // General purpose flags
  quint16 flags;
  // Compression method
  quint16 method;
  // Last modification date and time (2-second precision)
  QDateTime dateTime;
  // CRC
  quint32 crc;
  // Compressed file size
  quint64 compressedSize;
  // Uncompressed file size
  quint64 uncompressedSize;
  // Disk number start
  quint16 diskNumberStart;
  // Internal file attributes
  quint16 internalAttr;
  // External file attributes
  quint32 externalAttr;
  // Comment
  QString comment;
  // Extra field
  QByteArray extra;
  // Get file permissions from external attributes
  QFile::Permissions getPermissions() const;
  // Convert to QuaZipFileInfo (for compatibility)
  bool toQuaZipFileInfo(QuaZipFileInfo &info) const;
  // Get NTFS modification time (UTC)
  QDateTime getNTFSmTime(int *fineTicks = NULL) const;
  // Get NTFS access time (UTC)
  QDateTime getNTFSaTime(int *fineTicks = NULL) const;
  // Get NTFS creation time (UTC)
  QDateTime getNTFScTime(int *fineTicks = NULL) const;
  // Check if file is encrypted
  bool isEncrypted() const {return (flags & 1) != 0;}
};

#endif
