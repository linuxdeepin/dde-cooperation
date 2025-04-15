// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUA_ZIP_H
#define QUA_ZIP_H

#include <QString>
#include <QStringList>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif

#include "zip.h"
#include "unzip.h"

#include "exportdefine.h"
#include "quazipfileinfo.h"

// Additional error code for ZIP/UNZIP operations
#ifndef UNZ_OPENERROR
#define UNZ_OPENERROR -1000
#endif

class QuaZipPrivate;

// Main class for ZIP archive operations
// Handles opening, reading and writing ZIP files
class DLL_EXPORT QuaZip {
  friend class QuaZipPrivate;
  public:
    // Constants for ZIP operations
    enum Constants {
      MAX_FILE_NAME_LENGTH=256 // Maximum file name length
    };
    // ZIP file open modes
    enum Mode {
      mdNotOpen, // Not opened
      mdUnzip,   // Opened for reading files
      mdCreate,  // Created new archive
      mdAppend,  // Opened in append mode
      mdAdd      // Opened for adding files
    };
    // Case sensitivity settings for file names
    enum CaseSensitivity {
      csDefault=0,    // Platform default
      csSensitive=1,  // Case sensitive
      csInsensitive=2 // Case insensitive
    };
    // Convert case sensitivity to Qt standard
    static Qt::CaseSensitivity convertCaseSensitivity(CaseSensitivity cs);
  private:
    QuaZipPrivate *p;
    // not (and will not be) implemented
    QuaZip(const QuaZip& that);
    // not (and will not be) implemented
    QuaZip& operator=(const QuaZip& that);
  public:
    // Default constructor
    QuaZip();
    
    // Constructor with ZIP filename
    QuaZip(const QString& zipName);
    
    // Constructor with IO device
    QuaZip(QIODevice *ioDevice);
    // Destructor - closes file if open
    ~QuaZip();
    // Open ZIP file
    bool open(Mode mode, zlib_filefunc_def *ioApi =NULL);
    // Close ZIP file
    void close();
    // Set filename encoding
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setFileNameCodec(QTextCodec *fileNameCodec);
    void setFileNameCodec(const char *fileNameCodecName);
    QTextCodec* getFileNameCodec() const;
    void setCommentCodec(QTextCodec *commentCodec);
    void setCommentCodec(const char *commentCodecName);
    QTextCodec* getCommentCodec() const;
#else
    void setFileNameEncoding(QStringConverter::Encoding encoding);
    QStringConverter::Encoding getFileNameEncoding() const;
    void setCommentEncoding(QStringConverter::Encoding encoding);
    QStringConverter::Encoding getCommentEncoding() const;
#endif
    // Get ZIP filename
    QString getZipName() const;
    // Set ZIP filename
    void setZipName(const QString& zipName);
    // Get IO device
    QIODevice *getIoDevice() const;
    // Set IO device
    void setIoDevice(QIODevice *ioDevice);
    // Get current mode
    Mode getMode() const;
    // Check if open
    bool isOpen() const;
    // Get last error code
    int getZipError() const;
    // Get entry count
    int getEntriesCount() const;
    // Get ZIP comment
    QString getComment() const;
    // Set ZIP comment
    void setComment(const QString& comment);
    // Go to first file
    bool goToFirstFile();
    // Go to next file
    bool goToNextFile();
    // Set current file
    bool setCurrentFile(const QString& fileName, CaseSensitivity cs =csDefault);
    // Check if has current file
    bool hasCurrentFile() const;
    // Get current file info (32-bit)
    bool getCurrentFileInfo(QuaZipFileInfo* info)const;
    // Get current file info (64-bit)
    bool getCurrentFileInfo(QuaZipFileInfo64* info)const;
    // Get current filename
    QString getCurrentFileName()const;
    // Get unzFile handle
    unzFile getUnzFile();
    // Get zipFile handle
    zipFile getZipFile();
    // Enable/disable data descriptor
    void setDataDescriptorWritingEnabled(bool enabled);
    // Check data descriptor status
    bool isDataDescriptorWritingEnabled() const;
    // Get all filenames
    QStringList getFileNameList() const;
    // Get file info list (32-bit)
    QList<QuaZipFileInfo> getFileInfoList() const;
    // Get file info list (64-bit)
    QList<QuaZipFileInfo64> getFileInfoList64() const;
    // Enable/disable ZIP64
    void setZip64Enabled(bool zip64);
    // Check ZIP64 status
    bool isZip64Enabled() const;
    // Check auto-close status
    bool isAutoClose() const;
    // Set auto-close
    void setAutoClose(bool autoClose) const;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Set default filename codec (Qt5 version)
    static void setDefaultFileNameCodec(QTextCodec *codec);
    // Set default filename codec by name
    static void setDefaultFileNameCodec(const char *codecName);
#else
    // Set default filename encoding (Qt6 version)
    static void setDefaultFileNameEncoding(QStringConverter::Encoding encoding);
#endif
};

#endif
