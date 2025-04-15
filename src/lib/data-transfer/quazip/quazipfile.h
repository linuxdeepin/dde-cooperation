// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUA_ZIPFILE_H
#define QUA_ZIPFILE_H

#include <QIODevice>

#include "exportdefine.h"
#include "quazip.h"
#include "quazipnewinfo.h"

class QuaZipFilePrivate;

// Class representing a file inside ZIP archive
// Provides QIODevice interface for ZIP files access
class DLL_EXPORT QuaZipFile: public QIODevice {
  friend class QuaZipFilePrivate;
  Q_OBJECT
  private:
    QuaZipFilePrivate *p;
    // these are not supported nor implemented
    QuaZipFile(const QuaZipFile& that);
    QuaZipFile& operator=(const QuaZipFile& that);
  protected:
    // Read data implementation
    qint64 readData(char *data, qint64 maxSize);
    // Write data implementation
    qint64 writeData(const char *data, qint64 maxSize);
  public:
    // Default constructor
    QuaZipFile();
    // Constructor with parent object
    QuaZipFile(QObject *parent);
    // Constructor with zip file name (read-only)
    QuaZipFile(const QString& zipName, QObject *parent =NULL);
    // Constructor with zip and file names (read-only)
    QuaZipFile(const QString& zipName, const QString& fileName,
        QuaZip::CaseSensitivity cs =QuaZip::csDefault, QObject *parent =NULL);
    // Constructor with existing QuaZip instance
    // Warning: changing current file in QuaZip will invalidate this object
    QuaZipFile(QuaZip *zip, QObject *parent =NULL);
    // Destructor - closes file and cleans up
    virtual ~QuaZipFile();
    // Get associated ZIP archive file name
    QString getZipName()const;
    // Get pointer to associated QuaZip object
    QuaZip* getZip()const;
    // Get file name inside ZIP archive
    QString getFileName() const;
    // Get file name case sensitivity setting
    QuaZip::CaseSensitivity getCaseSensitivity() const;
    // Get actual file name in archive (may differ in case)
    QString getActualFileName()const;
    // Set ZIP archive file name (creates internal QuaZip object)
    void setZipName(const QString& zipName);
    // Check if file is in raw mode
    bool isRaw() const;
    // Bind to existing QuaZip instance
    void setZip(QuaZip *zip);
    // Set file name inside ZIP (with case sensitivity)
    void setFileName(const QString& fileName, QuaZip::CaseSensitivity cs =QuaZip::csDefault);
    // Open file for reading
    virtual bool open(OpenMode mode);
    // Open file for reading with password
    inline bool open(OpenMode mode, const char *password)
    {return open(mode, NULL, NULL, false, password);}
    // Open file for reading with advanced options
    bool open(OpenMode mode, int *method, int *level, bool raw, const char *password =NULL);
    // Open file for writing with file info
    bool open(OpenMode mode, const QuaZipNewInfo& info,
        const char *password =NULL, quint32 crc =0,
        int method =Z_DEFLATED, int level =Z_DEFAULT_COMPRESSION, bool raw =false,
        int windowBits =-MAX_WBITS, int memLevel =DEF_MEM_LEVEL, int strategy =Z_DEFAULT_STRATEGY);
    // Check if file is sequential
    virtual bool isSequential()const;
    // Get current position in file
    virtual qint64 pos()const;
    // Check if at end of file
    virtual bool atEnd()const;
    // Get file size
    virtual qint64 size()const;
    // Get compressed file size
    qint64 csize()const;
    // Get uncompressed file size
    qint64 usize()const;
    // Get file info (no zip64 support)
    bool getFileInfo(QuaZipFileInfo *info);
    // Get file info with zip64 support
    bool getFileInfo(QuaZipFileInfo64 *info);
    // Close the file
    virtual void close();
    // Get last ZIP operation error code
    int getZipError() const;
    // Get available bytes for reading
    virtual qint64 bytesAvailable() const;
};

#endif
