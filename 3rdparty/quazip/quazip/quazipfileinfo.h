// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUA_ZIPFILEINFO_H
#define QUA_ZIPFILEINFO_H

#include <QByteArray>
#include <QDateTime>
#include <QFile>

#include "quazip_global.h"

/// Information about a file inside archive.
/**
 * \deprecated Use QuaZipFileInfo64 instead. Not only it supports large files,
 * but also more convenience methods as well.
 *
 * Call QuaZip::getCurrentFileInfo() or QuaZipFile::getFileInfo() to
 * fill this structure. */
struct QUAZIP_EXPORT QuaZipFileInfo {
  /// File name.
  QString name;
  /// Version created by.
  quint16 versionCreated;
  /// Version needed to extract.
  quint16 versionNeeded;
  /// General purpose flags.
  quint16 flags;
  /// Compression method.
  quint16 method;
  /// Last modification date and time.
  QDateTime dateTime;
  /// CRC.
  quint32 crc;
  /// Compressed file size.
  quint32 compressedSize;
  /// Uncompressed file size.
  quint32 uncompressedSize;
  /// Disk number start.
  quint16 diskNumberStart;
  /// Internal file attributes.
  quint16 internalAttr;
  /// External file attributes.
  quint32 externalAttr;
  /// Comment.
  QString comment;
  /// Extra field.
  QByteArray extra;
  /// Get the file permissions.
  /**
    Returns the high 16 bits of external attributes converted to
    QFile::Permissions.
    */
  QFile::Permissions getPermissions() const;
};

/// Information about a file inside archive (with zip64 support).
/** Call QuaZip::getCurrentFileInfo() or QuaZipFile::getFileInfo() to
 * fill this structure. */
struct QUAZIP_EXPORT QuaZipFileInfo64 {
  /// File name.
  QString name;
  /// Version created by.
  quint16 versionCreated;
  /// Version needed to extract.
  quint16 versionNeeded;
  /// General purpose flags.
  quint16 flags;
  /// Compression method.
  quint16 method;
  /// Last modification date and time.
  /**
   * This is the time stored in the standard ZIP header. This format only allows
   * to store time with 2-second precision, so the seconds will always be even
   * and the milliseconds will always be zero. If you need more precise
   * date and time, you can try to call the getNTFSmTime() function or
   * its siblings, provided that the archive itself contains these NTFS times.
   */
  QDateTime dateTime;
  /// CRC.
  quint32 crc;
  /// Compressed file size.
  quint64 compressedSize;
  /// Uncompressed file size.
  quint64 uncompressedSize;
  /// Disk number start.
  quint16 diskNumberStart;
  /// Internal file attributes.
  quint16 internalAttr;
  /// External file attributes.
  quint32 externalAttr;
  /// Comment.
  QString comment;
  /// Extra field.
  QByteArray extra;
  /// Get the file permissions.
  /**
    Returns the high 16 bits of external attributes converted to
    QFile::Permissions.
    */
  QFile::Permissions getPermissions() const;
  /// Converts to QuaZipFileInfo
  /**
    If any of the fields are greater than 0xFFFFFFFFu, they are set to
    0xFFFFFFFFu exactly, not just truncated. This function should be mainly used
    for compatibility with the old code expecting QuaZipFileInfo, in the cases
    when it's impossible or otherwise unadvisable (due to ABI compatibility
    reasons, for example) to modify that old code to use QuaZipFileInfo64.

    \return \c true if all fields converted correctly, \c false if an overflow
    occured.
    */
  bool toQuaZipFileInfo(QuaZipFileInfo &info) const;
  /// Returns the NTFS modification time
  /**
   * The getNTFS*Time() functions only work if there is an NTFS extra field
   * present. Otherwise, they all return invalid null timestamps.
   * @param fineTicks If not NULL, the fractional part of milliseconds returned
   *                  there, measured in 100-nanosecond ticks. Will be set to
   *                  zero if there is no NTFS extra field.
   * @sa dateTime
   * @sa getNTFSaTime()
   * @sa getNTFScTime()
   * @return The NTFS modification time, UTC
   */
  QDateTime getNTFSmTime(int *fineTicks = NULL) const;
  /// Returns the NTFS access time
  /**
   * The getNTFS*Time() functions only work if there is an NTFS extra field
   * present. Otherwise, they all return invalid null timestamps.
   * @param fineTicks If not NULL, the fractional part of milliseconds returned
   *                  there, measured in 100-nanosecond ticks. Will be set to
   *                  zero if there is no NTFS extra field.
   * @sa dateTime
   * @sa getNTFSmTime()
   * @sa getNTFScTime()
   * @return The NTFS access time, UTC
   */
  QDateTime getNTFSaTime(int *fineTicks = NULL) const;
  /// Returns the NTFS creation time
  /**
   * The getNTFS*Time() functions only work if there is an NTFS extra field
   * present. Otherwise, they all return invalid null timestamps.
   * @param fineTicks If not NULL, the fractional part of milliseconds returned
   *                  there, measured in 100-nanosecond ticks. Will be set to
   *                  zero if there is no NTFS extra field.
   * @sa dateTime
   * @sa getNTFSmTime()
   * @sa getNTFSaTime()
   * @return The NTFS creation time, UTC
   */
  QDateTime getNTFScTime(int *fineTicks = NULL) const;
  /// Checks whether the file is encrypted.
  bool isEncrypted() const {return (flags & 1) != 0;}
};

#endif
