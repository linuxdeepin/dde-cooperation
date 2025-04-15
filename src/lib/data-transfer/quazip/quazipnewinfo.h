// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUA_ZIPNEWINFO_H
#define QUA_ZIPNEWINFO_H

#include <QDateTime>
#include <QFile>
#include <QString>

#include "exportdefine.h"

#include "quazipfileinfo.h"

// Structure for new file information in ZIP archive
struct DLL_EXPORT QuaZipNewInfo {
  // File name inside archive (including path)
  QString name;
  // Last modification date and time
  QDateTime dateTime;
  // Internal file attributes
  quint16 internalAttr;
  // External file attributes (includes permissions)
  quint32 externalAttr;
  // File comment
  QString comment;
  // Local extra field
  QByteArray extraLocal;
  // Global extra field
  QByteArray extraGlobal;
  // Uncompressed file size (for raw mode)
  ulong uncompressedSize;
  // Constructor with file name
  QuaZipNewInfo(const QString& name);
  // Constructor with file name and source file
  QuaZipNewInfo(const QString& name, const QString& file);
  // Constructor from QuaZipFileInfo
  QuaZipNewInfo(const QuaZipFileInfo &existing);
  // Constructor from QuaZipFileInfo64
  QuaZipNewInfo(const QuaZipFileInfo64 &existing);
  // Set file timestamp from existing file
  void setFileDateTime(const QString& file);
  // Set file permissions from existing file
  void setFilePermissions(const QString &file);
  // Set file permissions directly
  void setPermissions(QFile::Permissions permissions);
  // Set NTFS times from existing file
  void setFileNTFSTimes(const QString &fileName);
  // Set NTFS modification time
  void setFileNTFSmTime(const QDateTime &mTime, int fineTicks = 0);
  // Set NTFS access time
  void setFileNTFSaTime(const QDateTime &aTime, int fineTicks = 0);
  // Set NTFS creation time
  void setFileNTFScTime(const QDateTime &cTime, int fineTicks = 0);
};

#endif
