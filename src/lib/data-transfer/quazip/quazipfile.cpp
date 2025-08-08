// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quazipfile.h"

#include <QDebug>

using namespace std;

/// The implementation class for QuaZip.
/**
\internal

This class contains all the private stuff for the QuaZipFile class, thus
allowing to preserve binary compatibility between releases, the
technique known as the Pimpl (private implementation) idiom.
*/
class QuaZipFilePrivate {
  friend class QuaZipFile;
  private:
    Q_DISABLE_COPY(QuaZipFilePrivate)
    /// The pointer to the associated QuaZipFile instance.
    QuaZipFile *q;
    /// The QuaZip object to work with.
    QuaZip *zip;
    /// The file name.
    QString fileName;
    /// Case sensitivity mode.
    QuaZip::CaseSensitivity caseSensitivity;
    /// Whether this file is opened in the raw mode.
    bool raw;
    /// Write position to keep track of.
    /**
      QIODevice::pos() is broken for non-seekable devices, so we need
      our own position.
      */
    qint64 writePos;
    /// Uncompressed size to write along with a raw file.
    quint64 uncompressedSize;
    /// CRC to write along with a raw file.
    quint32 crc;
    /// Whether \ref zip points to an internal QuaZip instance.
    /**
      This is true if the archive was opened by name, rather than by
      supplying an existing QuaZip instance.
      */
    bool internal;
    /// The last error.
    int zipError;
    /// Resets \ref zipError.
    inline void resetZipError() const {setZipError(UNZ_OK);}
    /// Sets the zip error.
    /**
      This function is marked as const although it changes one field.
      This allows to call it from const functions that don't change
      anything by themselves.
      */
    void setZipError(int zipError) const;
    /// The constructor for the corresponding QuaZipFile constructor.
    inline QuaZipFilePrivate(QuaZipFile *q):
      q(q),
      zip(NULL),
      caseSensitivity(QuaZip::csDefault),
      raw(false),
      writePos(0),
      uncompressedSize(0),
      crc(0),
      internal(true),
      zipError(UNZ_OK) {}
    /// The constructor for the corresponding QuaZipFile constructor.
    inline QuaZipFilePrivate(QuaZipFile *q, const QString &zipName):
      q(q),
      caseSensitivity(QuaZip::csDefault),
      raw(false),
      writePos(0),
      uncompressedSize(0),
      crc(0),
      internal(true),
      zipError(UNZ_OK)
      {
        zip=new QuaZip(zipName);
      }
    /// The constructor for the corresponding QuaZipFile constructor.
    inline QuaZipFilePrivate(QuaZipFile *q, const QString &zipName, const QString &fileName,
        QuaZip::CaseSensitivity cs):
      q(q),
      raw(false),
      writePos(0),
      uncompressedSize(0),
      crc(0),
      internal(true),
      zipError(UNZ_OK)
      {
        zip=new QuaZip(zipName);
        this->fileName=fileName;
        if (this->fileName.startsWith('/'))
            this->fileName = this->fileName.mid(1);
        this->caseSensitivity=cs;
      }
    /// The constructor for the QuaZipFile constructor accepting a file name.
    inline QuaZipFilePrivate(QuaZipFile *q, QuaZip *zip):
      q(q),
      zip(zip),
      raw(false),
      writePos(0),
      uncompressedSize(0),
      crc(0),
      internal(false),
      zipError(UNZ_OK) {}
    /// The destructor.
    inline ~QuaZipFilePrivate()
    {
      if (internal)
        delete zip;
    }
};

QuaZipFile::QuaZipFile():
  p(new QuaZipFilePrivate(this))
{
    qInfo() << "QuaZipFile constructor (default)";
}

QuaZipFile::QuaZipFile(QObject *parent):
  QIODevice(parent),
  p(new QuaZipFilePrivate(this))
{
    qInfo() << "QuaZipFile constructor (with parent)";
}

QuaZipFile::QuaZipFile(const QString& zipName, QObject *parent):
  QIODevice(parent),
  p(new QuaZipFilePrivate(this, zipName))
{
    qInfo() << "QuaZipFile constructor (with zipName):" << zipName;
}

QuaZipFile::QuaZipFile(const QString& zipName, const QString& fileName,
    QuaZip::CaseSensitivity cs, QObject *parent):
  QIODevice(parent),
  p(new QuaZipFilePrivate(this, zipName, fileName, cs))
{
    qInfo() << "QuaZipFile constructor (with zipName, fileName, cs):" << zipName << "," << fileName;
}

QuaZipFile::QuaZipFile(QuaZip *zip, QObject *parent):
  QIODevice(parent),
  p(new QuaZipFilePrivate(this, zip))
{
    qInfo() << "QuaZipFile constructor (with QuaZip instance)";
}

QuaZipFile::~QuaZipFile()
{
  qInfo() << "QuaZipFile destructor";
  if (isOpen())
    close();
  delete p;
}

QString QuaZipFile::getZipName() const
{
  qInfo() << "Getting zip name";
  return p->zip==NULL ? QString() : p->zip->getZipName();
}

QuaZip *QuaZipFile::getZip() const
{
    qInfo() << "Getting QuaZip instance";
    return p->internal ? NULL : p->zip;
}

QString QuaZipFile::getActualFileName()const
{
  qInfo() << "Getting actual file name";
  p->setZipError(UNZ_OK);
  if (p->zip == NULL || (openMode() & WriteOnly)) {
    qInfo() << "zip is NULL or in write-only mode";
    return QString();
  }
  QString name=p->zip->getCurrentFileName();
  if(name.isNull()) {
    qInfo() << "getCurrentFileName failed";
    p->setZipError(p->zip->getZipError());
  }
  return name;
}

void QuaZipFile::setZipName(const QString& zipName)
{
  qInfo() << "Setting zip name to:" << zipName;
  if(isOpen()) {
    qWarning("QuaZipFile::setZipName(): file is already open - can not set ZIP name");
    return;
  }
  if(p->zip!=NULL && p->internal)
    delete p->zip;
  p->zip=new QuaZip(zipName);
  p->internal=true;
}

void QuaZipFile::setZip(QuaZip *zip)
{
  qInfo() << "Setting QuaZip instance";
  if(isOpen()) {
    qWarning("QuaZipFile::setZip(): file is already open - can not set ZIP");
    return;
  }
  if(p->zip!=NULL && p->internal)
    delete p->zip;
  p->zip=zip;
  p->fileName=QString();
  p->internal=false;
}

void QuaZipFile::setFileName(const QString& fileName, QuaZip::CaseSensitivity cs)
{
  qInfo() << "Setting file name to:" << fileName << "with case sensitivity:" << cs;
  if(p->zip==NULL) {
    qWarning("QuaZipFile::setFileName(): call setZipName() first");
    return;
  }
  if(!p->internal) {
    qWarning("QuaZipFile::setFileName(): should not be used when not using internal QuaZip");
    return;
  }
  if(isOpen()) {
    qWarning("QuaZipFile::setFileName(): can not set file name for already opened file");
    return;
  }
  p->fileName=fileName;
  if (p->fileName.startsWith('/'))
      p->fileName = p->fileName.mid(1);
  p->caseSensitivity=cs;
}

void QuaZipFilePrivate::setZipError(int zipError) const
{
  qInfo() << "Setting zip error to:" << zipError;
  QuaZipFilePrivate *fakeThis = const_cast<QuaZipFilePrivate*>(this); // non-const
  fakeThis->zipError=zipError;
  if(zipError==UNZ_OK)
    q->setErrorString(QString());
  else {
    q->setErrorString(QuaZipFile::tr("ZIP/UNZIP API error %1").arg(zipError));
    qInfo() << "ZIP/UNZIP API error" << zipError;
  }
}

bool QuaZipFile::open(OpenMode mode)
{
  qInfo() << "Opening file with mode:" << mode;
  return open(mode, NULL);
}

bool QuaZipFile::open(OpenMode mode, int *method, int *level, bool raw, const char *password)
{
  qInfo() << "Opening file with advanced parameters";
  p->resetZipError();
  if(isOpen()) {
    qWarning("QuaZipFile::open(): already opened");
    return false;
  }
  if(mode&Unbuffered) {
    qWarning("QuaZipFile::open(): Unbuffered mode is not supported");
    return false;
  }
  if((mode&ReadOnly)&&!(mode&WriteOnly)) {
    if(p->internal) {
      if(!p->zip->open(QuaZip::mdUnzip)) {
        p->setZipError(p->zip->getZipError());
        qInfo() << "zip->open failed";
        return false;
      }
      if(!p->zip->setCurrentFile(p->fileName, p->caseSensitivity)) {
        p->setZipError(p->zip->getZipError());
        qInfo() << "zip->setCurrentFile failed";
        p->zip->close();
        return false;
      }
    } else {
      if(p->zip==NULL) {
        qWarning("QuaZipFile::open(): zip is NULL");
        return false;
      }
      if(p->zip->getMode()!=QuaZip::mdUnzip) {
        qWarning("QuaZipFile::open(): file open mode %d incompatible with ZIP open mode %d",
            (int)mode, (int)p->zip->getMode());
        return false;
      }
      if(!p->zip->hasCurrentFile()) {
        qWarning("QuaZipFile::open(): zip does not have current file");
        return false;
      }
    }
    p->setZipError(unzOpenCurrentFile3(p->zip->getUnzFile(), method, level, (int)raw, password));
    if(p->zipError==UNZ_OK) {
      setOpenMode(mode);
      p->raw=raw;
      return true;
    } else {
      qInfo() << "unzOpenCurrentFile3 failed with error" << p->zipError;
      return false;
    }
  }
  qWarning("QuaZipFile::open(): open mode %d not supported by this function", (int)mode);
  return false;
}

bool QuaZipFile::open(OpenMode mode, const QuaZipNewInfo& info,
    const char *password, quint32 crc,
    int method, int level, bool raw,
    int windowBits, int memLevel, int strategy)
{
  qInfo() << "Opening file with new info";
  zip_fileinfo info_z;
  p->resetZipError();
  if(isOpen()) {
    qWarning("QuaZipFile::open(): already opened");
    return false;
  }
  if((mode&WriteOnly)&&!(mode&ReadOnly)) {
    if(p->internal) {
      qWarning("QuaZipFile::open(): write mode is incompatible with internal QuaZip approach");
      return false;
    }
    if(p->zip==NULL) {
      qWarning("QuaZipFile::open(): zip is NULL");
      return false;
    }
    if(p->zip->getMode()!=QuaZip::mdCreate&&p->zip->getMode()!=QuaZip::mdAppend&&p->zip->getMode()!=QuaZip::mdAdd) {
      qWarning("QuaZipFile::open(): file open mode %d incompatible with ZIP open mode %d",
          (int)mode, (int)p->zip->getMode());
      return false;
    }
    info_z.tmz_date.tm_year=info.dateTime.date().year();
    info_z.tmz_date.tm_mon=info.dateTime.date().month() - 1;
    info_z.tmz_date.tm_mday=info.dateTime.date().day();
    info_z.tmz_date.tm_hour=info.dateTime.time().hour();
    info_z.tmz_date.tm_min=info.dateTime.time().minute();
    info_z.tmz_date.tm_sec=info.dateTime.time().second();
    info_z.dosDate = 0;
    info_z.internal_fa=(uLong)info.internalAttr;
    info_z.external_fa=(uLong)info.externalAttr;
    if (p->zip->isDataDescriptorWritingEnabled())
        zipSetFlags(p->zip->getZipFile(), ZIP_WRITE_DATA_DESCRIPTOR);
    else
        zipClearFlags(p->zip->getZipFile(), ZIP_WRITE_DATA_DESCRIPTOR);
    p->setZipError(zipOpenNewFileInZip3_64(p->zip->getZipFile(),
          p->zip->getFileNameCodec()->fromUnicode(info.name).constData(), &info_z,
          info.extraLocal.constData(), info.extraLocal.length(),
          info.extraGlobal.constData(), info.extraGlobal.length(),
          p->zip->getCommentCodec()->fromUnicode(info.comment).constData(),
          method, level, (int)raw,
          windowBits, memLevel, strategy,
          password, (uLong)crc, p->zip->isZip64Enabled()));
    if(p->zipError==UNZ_OK) {
      p->writePos=0;
      setOpenMode(mode);
      p->raw=raw;
      if(raw) {
        p->crc=crc;
        p->uncompressedSize=info.uncompressedSize;
      }
      return true;
    } else {
      qInfo() << "zipOpenNewFileInZip3_64 failed with error" << p->zipError;
      return false;
    }
  }
  qWarning("QuaZipFile::open(): open mode %d not supported by this function", (int)mode);
  return false;
}

bool QuaZipFile::isSequential()const
{
  qInfo() << "Checking if file is sequential";
  return true;
}

qint64 QuaZipFile::pos()const
{
  qInfo() << "Getting current position";
  if(p->zip==NULL) {
    qWarning("QuaZipFile::pos(): call setZipName() or setZip() first");
    return -1;
  }
  if(!isOpen()) {
    qWarning("QuaZipFile::pos(): file is not open");
    return -1;
  }
  if(openMode()&ReadOnly)
      // QIODevice::pos() is broken for sequential devices,
      // but thankfully bytesAvailable() returns the number of
      // bytes buffered, so we know how far ahead we are.
    return unztell64(p->zip->getUnzFile()) - QIODevice::bytesAvailable();
  else
    return p->writePos;
}

bool QuaZipFile::atEnd()const
{
  qInfo() << "Checking if at end of file";
  if(p->zip==NULL) {
    qWarning("QuaZipFile::atEnd(): call setZipName() or setZip() first");
    return false;
  }
  if(!isOpen()) {
    qWarning("QuaZipFile::atEnd(): file is not open");
    return false;
  }
  if(openMode()&ReadOnly)
      // the same problem as with pos()
    return QIODevice::bytesAvailable() == 0
        && unzeof(p->zip->getUnzFile())==1;
  else
    return true;
}

qint64 QuaZipFile::size()const
{
  qInfo() << "Getting file size";
  if(!isOpen()) {
    qWarning("QuaZipFile::atEnd(): file is not open");
    return -1;
  }
  if(openMode()&ReadOnly)
    return p->raw?csize():usize();
  else
    return p->writePos;
}

qint64 QuaZipFile::csize()const
{
  qInfo() << "Getting compressed size";
  unz_file_info64 info_z;
  p->setZipError(UNZ_OK);
  if(p->zip==NULL||p->zip->getMode()!=QuaZip::mdUnzip) {
      qInfo() << "zip is NULL or not in unzip mode";
      return -1;
  }
  p->setZipError(unzGetCurrentFileInfo64(p->zip->getUnzFile(), &info_z, NULL, 0, NULL, 0, NULL, 0));
  if(p->zipError!=UNZ_OK) {
    qInfo() << "unzGetCurrentFileInfo64 failed with error" << p->zipError;
    return -1;
  }
  return info_z.compressed_size;
}

qint64 QuaZipFile::usize()const
{
  qInfo() << "Getting uncompressed size";
  unz_file_info64 info_z;
  p->setZipError(UNZ_OK);
  if(p->zip==NULL||p->zip->getMode()!=QuaZip::mdUnzip) {
      qInfo() << "zip is NULL or not in unzip mode";
      return -1;
  }
  p->setZipError(unzGetCurrentFileInfo64(p->zip->getUnzFile(), &info_z, NULL, 0, NULL, 0, NULL, 0));
  if(p->zipError!=UNZ_OK) {
    qInfo() << "unzGetCurrentFileInfo64 failed with error" << p->zipError;
    return -1;
  }
  return info_z.uncompressed_size;
}

bool QuaZipFile::getFileInfo(QuaZipFileInfo *info)
{
    qInfo() << "Getting file info (QuaZipFileInfo)";
    QuaZipFileInfo64 info64;
    if (getFileInfo(&info64)) {
        info64.toQuaZipFileInfo(*info);
        return true;
    } else {
        qInfo() << "getFileInfo(&info64) failed";
        return false;
    }
}

bool QuaZipFile::getFileInfo(QuaZipFileInfo64 *info)
{
    qInfo() << "Getting file info (QuaZipFileInfo64)";
    if(p->zip==NULL||p->zip->getMode()!=QuaZip::mdUnzip) {
        qInfo() << "zip is NULL or not in unzip mode";
        return false;
    }
    p->zip->getCurrentFileInfo(info);
    p->setZipError(p->zip->getZipError());
    return p->zipError==UNZ_OK;
}

void QuaZipFile::close()
{
  qInfo() << "Closing QuaZipFile";
  p->resetZipError();
  if(p->zip==NULL||!p->zip->isOpen()) {
      qInfo() << "zip is NULL or not open";
      return;
  }
  if(!isOpen()) {
    qWarning("QuaZipFile::close(): file isn't open");
    return;
  }
  if(openMode()&ReadOnly)
    p->setZipError(unzCloseCurrentFile(p->zip->getUnzFile()));
  else if(openMode()&WriteOnly)
    if(isRaw()) p->setZipError(zipCloseFileInZipRaw64(p->zip->getZipFile(), p->uncompressedSize, p->crc));
    else p->setZipError(zipCloseFileInZip(p->zip->getZipFile()));
  else {
    qWarning("Wrong open mode: %d", (int)openMode());
    return;
  }
  if(p->zipError==UNZ_OK) setOpenMode(QIODevice::NotOpen);
  else {
      qInfo() << "zip/unzip close error:" << p->zipError;
      return;
  }
  if(p->internal) {
    p->zip->close();
    p->setZipError(p->zip->getZipError());
  }
}

qint64 QuaZipFile::readData(char *data, qint64 maxSize)
{
  qInfo() << "Reading data, max size:" << maxSize;
  p->setZipError(UNZ_OK);
  qint64 bytesRead=unzReadCurrentFile(p->zip->getUnzFile(), data, (unsigned)maxSize);
  if (bytesRead < 0) {
    p->setZipError((int) bytesRead);
    qInfo() << "unzReadCurrentFile failed with error" << bytesRead;
    return -1;
  }
  return bytesRead;
}

qint64 QuaZipFile::writeData(const char* data, qint64 maxSize)
{
  qInfo() << "Writing data, max size:" << maxSize;
  p->setZipError(ZIP_OK);
  p->setZipError(zipWriteInFileInZip(p->zip->getZipFile(), data, (uint)maxSize));
  if(p->zipError!=ZIP_OK) {
      qInfo() << "zipWriteInFileInZip failed with error" << p->zipError;
      return -1;
  } else {
    p->writePos+=maxSize;
    return maxSize;
  }
}

QString QuaZipFile::getFileName() const
{
  qInfo() << "Getting file name";
  return p->fileName;
}

QuaZip::CaseSensitivity QuaZipFile::getCaseSensitivity() const
{
  qInfo() << "Getting case sensitivity";
  return p->caseSensitivity;
}

bool QuaZipFile::isRaw() const
{
  qInfo() << "Checking if file is raw";
  return p->raw;
}

int QuaZipFile::getZipError() const
{
  qInfo() << "Getting zip error";
  return p->zipError;
}

qint64 QuaZipFile::bytesAvailable() const
{
    qInfo() << "function called";
    return size() - pos();
}
