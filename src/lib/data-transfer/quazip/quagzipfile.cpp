// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QDebug>

#include "quagzipfile.h"

/// \cond internal
class QuaGzipFilePrivate {
    friend class QuaGzipFile;
    QString fileName;
    gzFile gzd;
    inline QuaGzipFilePrivate(): gzd(NULL) {}
    inline QuaGzipFilePrivate(const QString &fileName): 
        fileName(fileName), gzd(NULL) {}
    template<typename FileId> bool open(FileId id, 
        QIODevice::OpenMode mode, QString &error);
    gzFile open(int fd, const char *modeString);
    gzFile open(const QString &name, const char *modeString);
};

gzFile QuaGzipFilePrivate::open(const QString &name, const char *modeString)
{
    qInfo() << "Opening Gzip file by name:" << name << "with mode string:" << modeString;
    return gzopen(QFile::encodeName(name).constData(), modeString);
}

gzFile QuaGzipFilePrivate::open(int fd, const char *modeString)
{
    qInfo() << "Opening Gzip file by file descriptor:" << fd << "with mode string:" << modeString;
    return gzdopen(fd, modeString);
}

template<typename FileId>
bool QuaGzipFilePrivate::open(FileId id, QIODevice::OpenMode mode, 
                              QString &error)
{
    qInfo() << "Preparing to open Gzip file with mode:" << mode;
    char modeString[2];
    modeString[0] = modeString[1] = '\0';
    if ((mode & QIODevice::Append) != 0) {
        error = QuaGzipFile::tr("QIODevice::Append is not "
                "supported for GZIP");
        qInfo() << error;
        return false;
    }
    if ((mode & QIODevice::ReadOnly) != 0
            && (mode & QIODevice::WriteOnly) != 0) {
        error = QuaGzipFile::tr("Opening gzip for both reading"
            " and writing is not supported");
        qInfo() << error;
        return false;
    } else if ((mode & QIODevice::ReadOnly) != 0) {
        modeString[0] = 'r';
    } else if ((mode & QIODevice::WriteOnly) != 0) {
        modeString[0] = 'w';
    } else {
        error = QuaGzipFile::tr("You can open a gzip either for reading"
            " or for writing. Which is it?");
        qInfo() << error;
        return false;
    }
    gzd = open(id, modeString);
    if (gzd == NULL) {
        error = QuaGzipFile::tr("Could not gzopen() file");
        qInfo() << error;
        return false;
    }
    return true;
}
/// \endcond

QuaGzipFile::QuaGzipFile():
d(new QuaGzipFilePrivate())
{
    qInfo() << "Constructing QuaGzipFile (default)";
}

QuaGzipFile::QuaGzipFile(QObject *parent):
QIODevice(parent),
d(new QuaGzipFilePrivate())
{
    qInfo() << "Constructing QuaGzipFile with parent";
}

QuaGzipFile::QuaGzipFile(const QString &fileName, QObject *parent):
  QIODevice(parent),
d(new QuaGzipFilePrivate(fileName))
{
    qInfo() << "Constructing QuaGzipFile with fileName:" << fileName;
}

QuaGzipFile::~QuaGzipFile()
{
    qInfo() << "Destructing QuaGzipFile for file:" << d->fileName;
  if (isOpen()) {
    close();
  }
  delete d;
}

void QuaGzipFile::setFileName(const QString& fileName)
{
    qInfo() << "Setting file name to:" << fileName;
    d->fileName = fileName;
}

QString QuaGzipFile::getFileName() const
{
    qInfo() << "Getting file name";
    return d->fileName;
}

bool QuaGzipFile::isSequential() const
{
    qInfo() << "Checking if device is sequential";
  return true;
}

bool QuaGzipFile::open(QIODevice::OpenMode mode)
{
    qInfo() << "Opening file:" << d->fileName << "with mode:" << mode;
    QString error;
    if (!d->open(d->fileName, mode, error)) {
        setErrorString(error);
        qInfo() << error;
        return false;
    }
    return QIODevice::open(mode);
}

bool QuaGzipFile::open(int fd, QIODevice::OpenMode mode)
{
    qInfo() << "Opening file descriptor:" << fd << "with mode:" << mode;
    QString error;
    if (!d->open(fd, mode, error)) {
        setErrorString(error);
        qInfo() << error;
        return false;
    }
    return QIODevice::open(mode);
}

bool QuaGzipFile::flush()
{
    qInfo() << "Flushing Gzip stream for file:" << d->fileName;
    return gzflush(d->gzd, Z_SYNC_FLUSH) == Z_OK;
}

void QuaGzipFile::close()
{
    qInfo() << "Closing Gzip file:" << d->fileName;
    QIODevice::close();
    gzclose(d->gzd);
}

qint64 QuaGzipFile::readData(char *data, qint64 maxSize)
{
    // qInfo() << "Reading data, max size:" << maxSize;
    return gzread(d->gzd, (voidp)data, (unsigned)maxSize);
}

qint64 QuaGzipFile::writeData(const char *data, qint64 maxSize)
{
    // qInfo() << "Writing data, size:" << maxSize;
    if (maxSize == 0) {
        // qInfo() << "maxSize is 0";
        return 0;
    }
    int written = gzwrite(d->gzd, (voidp)data, (unsigned)maxSize);
    if (written == 0) {
        // qInfo() << "gzwrite returned 0";
        return -1;
    } else {
        return written;
    }
}