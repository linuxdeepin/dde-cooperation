// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef JLCOMPRESSFOLDER_H_
#define JLCOMPRESSFOLDER_H_

#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QFile>

// Utility class for typical operations with static functions for ZIP operations
class DLL_EXPORT CompressUtil {
private:
    static QStringList extractDir(QuaZip &zip, const QString &dir);
    static QStringList getFileList(QuaZip *zip);
    static QString extractFile(QuaZip &zip, QString fileName, QString fileDest);
    static QStringList extractFiles(QuaZip &zip, const QStringList &files, const QString &dir);
    // Compress single file to opened zip, returns success status
    static bool compressFile(QuaZip* zip, QString fileName, QString fileDest);
    // Compress subdirectory to parent zip, can be recursive
    static bool compressSubDir(QuaZip* parentZip, QString dir, QString parentDir, bool recursive,
                               QDir::Filters filters);
    // Extract single file from opened zip, returns success status
    static bool extractFile(QuaZip* zip, QString fileName, QString fileDest);
    // Remove list of files, returns success status
    static bool removeFile(QStringList listFile);

public:
    // Compress single file to archive, returns success status
    static bool compressFile(QString fileCompressed, QString file);
    // Compress list of files to archive, returns success status
    static bool compressFiles(QString fileCompressed, QStringList files);
    // Compress whole directory to archive, can be recursive
    static bool compressDir(QString fileCompressed, QString dir = QString(), bool recursive = true);
    // Compress whole directory with filters, returns success status
    static bool compressDir(QString fileCompressed, QString dir,
                            bool recursive, QDir::Filters filters);

public:
    // Extract single file from archive, returns extracted file path
    static QString extractFile(QString fileCompressed, QString fileName, QString fileDest = QString());
    // Extract list of files from archive, returns list of extracted paths
    static QStringList extractFiles(QString fileCompressed, QStringList files, QString dir = QString());
    // Extract whole archive, returns list of extracted paths
    static QStringList extractDir(QString fileCompressed, QString dir = QString());
    // Get list of files in archive
    static QStringList getFileList(QString fileCompressed);
    // Extract single file from IO device, returns extracted file path
    static QString extractFile(QIODevice *ioDevice, QString fileName, QString fileDest = QString());
    // Extract list of files from IO device, returns list of extracted paths
    static QStringList extractFiles(QIODevice *ioDevice, QStringList files, QString dir = QString());
    // Extract whole archive from IO device, returns list of extracted paths
    static QStringList extractDir(QIODevice *ioDevice, QString dir = QString());
    // Get list of files from IO device
    static QStringList getFileList(QIODevice *ioDevice); 
};

#endif /* JLCOMPRESSFOLDER_H_ */
