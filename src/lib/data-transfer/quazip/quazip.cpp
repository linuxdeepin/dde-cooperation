// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QFile>
#include <QFlags>
#include <QHash>
#include <QDebug>

#include "quazip.h"

/// All the internal stuff for the QuaZip class.
/**
  \internal

  This class keeps all the private stuff for the QuaZip class so it can
  be changed without breaking binary compatibility, according to the
  Pimpl idiom.
  */
class QuaZipPrivate {
  friend class QuaZip;
  private:
    Q_DISABLE_COPY(QuaZipPrivate)
    /// The pointer to the corresponding QuaZip instance.
    QuaZip *q;
    /// The codec for file names.
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec *fileNameCodec;
    /// The codec for comments.
    QTextCodec *commentCodec;
#else
    QStringConverter::Encoding fileNameEncoding = QStringConverter::Utf8;
    QStringConverter::Encoding commentEncoding = QStringConverter::Utf8;
#endif
    /// The archive file name.
    QString zipName;
    /// The device to access the archive.
    QIODevice *ioDevice;
    /// The global comment.
    QString comment;
    /// The open mode.
    QuaZip::Mode mode;
    union {
      /// The internal handle for UNZIP modes.
      unzFile unzFile_f;
      /// The internal handle for ZIP modes.
      zipFile zipFile_f;
    };
    /// Whether a current file is set.
    bool hasCurrentFile_f;
    /// The last error.
    int zipError;
    /// Whether \ref QuaZip::setDataDescriptorWritingEnabled() "the data descriptor writing mode" is enabled.
    bool dataDescriptorWritingEnabled;
    /// The zip64 mode.
    bool zip64;
    /// The auto-close flag.
    bool autoClose;
    inline QTextCodec *getDefaultFileNameCodec()
    {
        qInfo() << "Getting default file name codec";
        if (defaultFileNameCodec == NULL) {
            return QTextCodec::codecForLocale();
        } else {
            return defaultFileNameCodec;
        }
    }
    /// The constructor for the corresponding QuaZip constructor.
    inline QuaZipPrivate(QuaZip *q):
      q(q),
      fileNameCodec(getDefaultFileNameCodec()),
      commentCodec(QTextCodec::codecForLocale()),
      ioDevice(NULL),
      mode(QuaZip::mdNotOpen),
      hasCurrentFile_f(false),
      zipError(UNZ_OK),
      dataDescriptorWritingEnabled(true),
      zip64(false),
      autoClose(true)
    {
        qInfo() << "Constructing QuaZipPrivate with QuaZip*";
        unzFile_f = NULL;
        zipFile_f = NULL;
        lastMappedDirectoryEntry.num_of_file = 0;
        lastMappedDirectoryEntry.pos_in_zip_directory = 0;
    }
    /// The constructor for the corresponding QuaZip constructor.
    inline QuaZipPrivate(QuaZip *q, const QString &zipName):
      q(q),
      fileNameCodec(getDefaultFileNameCodec()),
      commentCodec(QTextCodec::codecForLocale()),
      zipName(zipName),
      ioDevice(NULL),
      mode(QuaZip::mdNotOpen),
      hasCurrentFile_f(false),
      zipError(UNZ_OK),
      dataDescriptorWritingEnabled(true),
      zip64(false),
      autoClose(true)
    {
        qInfo() << "Constructing QuaZipPrivate with QuaZip* and zipName:" << zipName;
        unzFile_f = NULL;
        zipFile_f = NULL;
        lastMappedDirectoryEntry.num_of_file = 0;
        lastMappedDirectoryEntry.pos_in_zip_directory = 0;
    }
    /// The constructor for the corresponding QuaZip constructor.
    inline QuaZipPrivate(QuaZip *q, QIODevice *ioDevice):
        q(q),
      fileNameCodec(getDefaultFileNameCodec()),
      commentCodec(QTextCodec::codecForLocale()),
      ioDevice(ioDevice),
      mode(QuaZip::mdNotOpen),
      hasCurrentFile_f(false),
      zipError(UNZ_OK),
      dataDescriptorWritingEnabled(true),
      zip64(false),
      autoClose(true)
    {
        qInfo() << "Constructing QuaZipPrivate with QuaZip* and ioDevice";
        unzFile_f = NULL;
        zipFile_f = NULL;
        lastMappedDirectoryEntry.num_of_file = 0;
        lastMappedDirectoryEntry.pos_in_zip_directory = 0;
    }
    /// Returns either a list of file names or a list of QuaZipFileInfo.
    template<typename TFileInfo>
        bool getFileInfoList(QList<TFileInfo> *result) const;

    /// Stores map of filenames and file locations for unzipping
      inline void clearDirectoryMap();
      inline void addCurrentFileToDirectoryMap(const QString &fileName);
      bool goToFirstUnmappedFile();
      QHash<QString, unz64_file_pos> directoryCaseSensitive;
      QHash<QString, unz64_file_pos> directoryCaseInsensitive;
      unz64_file_pos lastMappedDirectoryEntry;
      static QTextCodec *defaultFileNameCodec;
};

QTextCodec *QuaZipPrivate::defaultFileNameCodec = NULL;

void QuaZipPrivate::clearDirectoryMap()
{
    qInfo() << "Clearing directory map";
    directoryCaseInsensitive.clear();
    directoryCaseSensitive.clear();
    lastMappedDirectoryEntry.num_of_file = 0;
    lastMappedDirectoryEntry.pos_in_zip_directory = 0;
}

void QuaZipPrivate::addCurrentFileToDirectoryMap(const QString &fileName)
{
    qInfo() << "Adding current file to directory map, fileName:" << fileName;
    if (!hasCurrentFile_f || fileName.isEmpty()) {
        qInfo() << "No current file or empty filename";
        return;
    }
    // Adds current file to filename map as fileName
    unz64_file_pos fileDirectoryPos;
    unzGetFilePos64(unzFile_f, &fileDirectoryPos);
    directoryCaseSensitive.insert(fileName, fileDirectoryPos);
    // Only add lowercase to directory map if not already there
    // ensures only map the first one seen
    QString lower = fileName.toLower();
    if (!directoryCaseInsensitive.contains(lower)) {
        qInfo() << "Adding to case-insensitive map";
        directoryCaseInsensitive.insert(lower, fileDirectoryPos);
    }
    // Mark last one
    if (fileDirectoryPos.pos_in_zip_directory > lastMappedDirectoryEntry.pos_in_zip_directory) {
        qInfo() << "Updating last mapped directory entry";
        lastMappedDirectoryEntry = fileDirectoryPos;
    }
}

bool QuaZipPrivate::goToFirstUnmappedFile()
{
    qInfo() << "Going to first unmapped file";
    zipError = UNZ_OK;
    if (mode != QuaZip::mdUnzip) {
        qWarning("QuaZipPrivate::goToNextUnmappedFile(): ZIP is not open in mdUnzip mode");
        qInfo() << "Not in mdUnzip mode";
        return false;
    }
    // If not mapped anything, go to beginning
    if (lastMappedDirectoryEntry.pos_in_zip_directory == 0) {
        qInfo() << "Going to first file";
        unzGoToFirstFile(unzFile_f);
    } else {
        // Goto the last one mapped, plus one
        qInfo() << "Going to next unmapped file";
        unzGoToFilePos64(unzFile_f, &lastMappedDirectoryEntry);
        unzGoToNextFile(unzFile_f);
    }
    hasCurrentFile_f=zipError==UNZ_OK;
    if(zipError==UNZ_END_OF_LIST_OF_FILE) {
      qInfo() << "End of file list";
      zipError=UNZ_OK;
    }
    return hasCurrentFile_f;
}

/**
 * 默认构造函数
 * 需要调用setZipName()或setIoDevice()设置ZIP文件后才能使用
 */
QuaZip::QuaZip():
  p(new QuaZipPrivate(this))
{
    qInfo() << "Constructing QuaZip (default)";
}

/**
 * 通过ZIP文件名构造
 * @param zipName ZIP文件路径
 */
QuaZip::QuaZip(const QString& zipName):
  p(new QuaZipPrivate(this, zipName))
{
    qInfo() << "Constructing QuaZip with zipName:" << zipName;
}

/**
 * 通过IO设备构造
 * @param ioDevice 用于读写ZIP的IO设备，必须支持随机访问
 */
QuaZip::QuaZip(QIODevice *ioDevice):
  p(new QuaZipPrivate(this, ioDevice))
{
    qInfo() << "Constructing QuaZip with ioDevice";
}

QuaZip::~QuaZip()
{
  qInfo() << "Destructing QuaZip";
  if(isOpen())
    close();
  delete p;
}

bool QuaZip::open(Mode mode, zlib_filefunc_def* ioApi)
{
  qInfo() << "Opening zip file" << p->zipName << "in mode" << mode;
  p->zipError=UNZ_OK;
  if(isOpen()) {
    qWarning("QuaZip::open(): ZIP already opened");
    return false;
  }
  QIODevice *ioDevice = p->ioDevice;
  if (ioDevice == NULL) {
    if (p->zipName.isEmpty()) {
      qWarning("QuaZip::open(): set either ZIP file name or IO device first");
      qInfo() << "No zip name or IO device set";
      return false;
    } else {
      qInfo() << "Creating new QFile for" << p->zipName;
      ioDevice = new QFile(p->zipName);
    }
  }
  unsigned flags = 0;
  switch(mode) {
    case mdUnzip:
      qInfo() << "Opening in mdUnzip mode";
      if (ioApi == NULL) {
          if (p->autoClose)
              flags |= UNZ_AUTO_CLOSE;
          p->unzFile_f=unzOpenInternal(ioDevice, NULL, 1, flags);
      } else {
          // QuaZIP pre-zip64 compatibility mode
          qInfo() << "Using custom ioApi";
          p->unzFile_f=unzOpen2(ioDevice, ioApi);
          if (p->unzFile_f != NULL) {
              if (p->autoClose) {
                  unzSetFlags(p->unzFile_f, UNZ_AUTO_CLOSE);
              } else {
                  unzClearFlags(p->unzFile_f, UNZ_AUTO_CLOSE);
              }
          }
      }
      if(p->unzFile_f!=NULL) {
        if (ioDevice->isSequential()) {
            unzClose(p->unzFile_f);
            if (!p->zipName.isEmpty())
                delete ioDevice;
            qWarning("QuaZip::open(): "
                     "only mdCreate can be used with "
                     "sequential devices");
            qInfo() << "Sequential device with mdUnzip";
            return false;
        }
        p->mode=mode;
        p->ioDevice = ioDevice;
        return true;
      } else {
        p->zipError=UNZ_OPENERROR;
        if (!p->zipName.isEmpty())
          delete ioDevice;
        qInfo() << "unzOpen failed";
        return false;
      }
    case mdCreate:
    case mdAppend:
    case mdAdd:
      qInfo() << "Opening in create/append/add mode";
      if (ioApi == NULL) {
          if (p->autoClose)
              flags |= ZIP_AUTO_CLOSE;
          if (p->dataDescriptorWritingEnabled)
              flags |= ZIP_WRITE_DATA_DESCRIPTOR;
          p->zipFile_f=zipOpen3(ioDevice,
              mode==mdCreate?APPEND_STATUS_CREATE:
              mode==mdAppend?APPEND_STATUS_CREATEAFTER:
              APPEND_STATUS_ADDINZIP,
              NULL, NULL, flags);
      } else {
          // QuaZIP pre-zip64 compatibility mode
          qInfo() << "Using custom ioApi";
          p->zipFile_f=zipOpen2(ioDevice,
              mode==mdCreate?APPEND_STATUS_CREATE:
              mode==mdAppend?APPEND_STATUS_CREATEAFTER:
              APPEND_STATUS_ADDINZIP,
              NULL,
              ioApi);
          if (p->zipFile_f != NULL) {
              zipSetFlags(p->zipFile_f, flags);
          }
      }
      if(p->zipFile_f!=NULL) {
        if (ioDevice->isSequential()) {
            if (mode != mdCreate) {
                zipClose(p->zipFile_f, NULL);
                qWarning("QuaZip::open(): "
                        "only mdCreate can be used with "
                         "sequential devices");
                if (!p->zipName.isEmpty())
                    delete ioDevice;
                qInfo() << "Sequential device with non-create mode";
                return false;
            }
            zipSetFlags(p->zipFile_f, ZIP_SEQUENTIAL);
        }
        p->mode=mode;
        p->ioDevice = ioDevice;
        return true;
      } else {
        p->zipError=UNZ_OPENERROR;
        if (!p->zipName.isEmpty())
          delete ioDevice;
        qInfo() << "zipOpen failed";
        return false;
      }
    default:
      qWarning("QuaZip::open(): unknown mode: %d", (int)mode);
      if (!p->zipName.isEmpty())
        delete ioDevice;
      qInfo() << "Unknown mode";
      return false;
      break;
  }
}

void QuaZip::close()
{
  qInfo() << "Closing zip file" << p->zipName;
  p->zipError=UNZ_OK;
  switch(p->mode) {
    case mdNotOpen:
      qWarning("QuaZip::close(): ZIP is not open");
      return;
    case mdUnzip:
      qInfo() << "Closing in mdUnzip mode";
      p->zipError=unzClose(p->unzFile_f);
      break;
    case mdCreate:
    case mdAppend:
    case mdAdd:
      qInfo() << "Closing in create/append/add mode";
      p->zipError=zipClose(p->zipFile_f, 
          p->comment.isNull() ? NULL
          : p->commentCodec->fromUnicode(p->comment).constData());
      break;
    default:
      qWarning("QuaZip::close(): unknown mode: %d", (int)p->mode);
      return;
  }
  // opened by name, need to delete the internal IO device
  if (!p->zipName.isEmpty()) {
      qInfo() << "Deleting internal IO device";
      delete p->ioDevice;
      p->ioDevice = NULL;
  }
  p->clearDirectoryMap();
  if(p->zipError==UNZ_OK)
    p->mode=mdNotOpen;
}

void QuaZip::setZipName(const QString& zipName)
{
  qInfo() << "Setting zip name to:" << zipName;
  if(isOpen()) {
    qWarning("QuaZip::setZipName(): ZIP is already open!");
    qInfo() << "ZIP already open";
    return;
  }
  p->zipName=zipName;
  p->ioDevice = NULL;
}

void QuaZip::setIoDevice(QIODevice *ioDevice)
{
  qInfo() << "Setting IO device";
  if(isOpen()) {
    qWarning("QuaZip::setIoDevice(): ZIP is already open!");
    return;
  }
  p->ioDevice = ioDevice;
  p->zipName = QString();
}

int QuaZip::getEntriesCount()const
{
  qInfo() << "Getting entries count";
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::getEntriesCount(): ZIP is not open in mdUnzip mode");
    return -1;
  }
  unz_global_info64 globalInfo;
  if((fakeThis->p->zipError=unzGetGlobalInfo64(p->unzFile_f, &globalInfo))!=UNZ_OK) {
    qInfo() << "unzGetGlobalInfo64 error:" << p->zipError;
    return p->zipError;
  }
  return (int)globalInfo.number_entry;
}

QString QuaZip::getComment()const
{
  qInfo() << "Getting zip comment";
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::getComment(): ZIP is not open in mdUnzip mode");
    return QString();
  }
  unz_global_info64 globalInfo;
  QByteArray comment;
  if((fakeThis->p->zipError=unzGetGlobalInfo64(p->unzFile_f, &globalInfo))!=UNZ_OK) {
    qInfo() << "unzGetGlobalInfo64 error:" << p->zipError;
    return QString();
  }
  comment.resize(globalInfo.size_comment);
  if((fakeThis->p->zipError=unzGetGlobalComment(p->unzFile_f, comment.data(), comment.size())) < 0) {
    qInfo() << "unzGetGlobalComment error:" << p->zipError;
    return QString();
  }
  fakeThis->p->zipError = UNZ_OK;
  return p->commentCodec->toUnicode(comment);
}

bool QuaZip::setCurrentFile(const QString& fileName, CaseSensitivity cs)
{
  qInfo() << "Setting current file to:" << fileName << "case sensitivity:" << cs;
  p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::setCurrentFile(): ZIP is not open in mdUnzip mode");
    return false;
  }
  if(fileName.isEmpty()) {
    qInfo() << "Empty filename";
    p->hasCurrentFile_f=false;
    return true;
  }
  // Unicode-aware reimplementation of the unzLocateFile function
  if(p->unzFile_f==NULL) {
    p->zipError=UNZ_PARAMERROR;
    qInfo() << "unzFile_f is NULL";
    return false;
  }
  if(fileName.length()>MAX_FILE_NAME_LENGTH) {
    p->zipError=UNZ_PARAMERROR;
    qInfo() << "Filename too long";
    return false;
  }
  // Find the file by name
  bool sens = convertCaseSensitivity(cs) == Qt::CaseSensitive;
  QString lower, current;
  if(!sens) lower=fileName.toLower();
  p->hasCurrentFile_f=false;

  // Check the appropriate Map
  unz64_file_pos fileDirPos;
  fileDirPos.pos_in_zip_directory = 0;
  if (sens) {
      if (p->directoryCaseSensitive.contains(fileName)) {
          qInfo() << "Found in case-sensitive map";
          fileDirPos = p->directoryCaseSensitive.value(fileName);
      }
  } else {
      if (p->directoryCaseInsensitive.contains(lower)) {
          qInfo() << "Found in case-insensitive map";
          fileDirPos = p->directoryCaseInsensitive.value(lower);
      }
  }

  if (fileDirPos.pos_in_zip_directory != 0) {
      p->zipError = unzGoToFilePos64(p->unzFile_f, &fileDirPos);
      p->hasCurrentFile_f = p->zipError == UNZ_OK;
  }

  if (p->hasCurrentFile_f)
      return p->hasCurrentFile_f;

  // Not mapped yet, start from where we have got to so far
  qInfo() << "Not in map, searching from first unmapped file";
  for(bool more=p->goToFirstUnmappedFile(); more; more=goToNextFile()) {
    current=getCurrentFileName();
    if(current.isEmpty()) {
        qInfo() << "getCurrentFileName returned empty string";
        return false;
    }
    if(sens) {
      if(current==fileName) break;
    } else {
      if(current.toLower()==lower) break;
    }
  }
  return p->hasCurrentFile_f;
}

bool QuaZip::goToFirstFile()
{
  qInfo() << "Going to first file";
  p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::goToFirstFile(): ZIP is not open in mdUnzip mode");
    return false;
  }
  p->zipError=unzGoToFirstFile(p->unzFile_f);
  p->hasCurrentFile_f=p->zipError==UNZ_OK;
  return p->hasCurrentFile_f;
}

bool QuaZip::goToNextFile()
{
  qInfo() << "Going to next file";
  p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::goToFirstFile(): ZIP is not open in mdUnzip mode");
    return false;
  }
  p->zipError=unzGoToNextFile(p->unzFile_f);
  p->hasCurrentFile_f=p->zipError==UNZ_OK;
  if(p->zipError==UNZ_END_OF_LIST_OF_FILE) {
    qInfo() << "End of file list";
    p->zipError=UNZ_OK;
  return p->hasCurrentFile_f;
}

bool QuaZip::getCurrentFileInfo(QuaZipFileInfo *info)const
{
    qInfo() << "Getting current file info (32-bit)";
    QuaZipFileInfo64 info64;
    if (info == NULL) { // Very unlikely because of the overloads
        qInfo() << "info is NULL";
        return false;
    }
    if (getCurrentFileInfo(&info64)) {
        info64.toQuaZipFileInfo(*info);
        return true;
    } else {
        qInfo() << "getCurrentFileInfo(&info64) failed";
        return false;
    }
}

bool QuaZip::getCurrentFileInfo(QuaZipFileInfo64 *info)const
{
  qInfo() << "Getting current file info (64-bit)";
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::getCurrentFileInfo(): ZIP is not open in mdUnzip mode");
    qInfo() << "Not in mdUnzip mode";
    return false;
  }
  unz_file_info64 info_z;
  QByteArray fileName;
  QByteArray extra;
  QByteArray comment;
  if(info==NULL) return false;
  if(!isOpen()||!hasCurrentFile()) return false;
  if((fakeThis->p->zipError=unzGetCurrentFileInfo64(p->unzFile_f, &info_z, NULL, 0, NULL, 0, NULL, 0))!=UNZ_OK)
    return false;
  fileName.resize(info_z.size_filename);
  extra.resize(info_z.size_file_extra);
  comment.resize(info_z.size_file_comment);
  if((fakeThis->p->zipError=unzGetCurrentFileInfo64(p->unzFile_f, NULL,
      fileName.data(), fileName.size(),
      extra.data(), extra.size(),
      comment.data(), comment.size()))!=UNZ_OK)
    return false;
  info->versionCreated=info_z.version;
  info->versionNeeded=info_z.version_needed;
  info->flags=info_z.flag;
  info->method=info_z.compression_method;
  info->crc=info_z.crc;
  info->compressedSize=info_z.compressed_size;
  info->uncompressedSize=info_z.uncompressed_size;
  info->diskNumberStart=info_z.disk_num_start;
  info->internalAttr=info_z.internal_fa;
  info->externalAttr=info_z.external_fa;
  info->name=p->fileNameCodec->toUnicode(fileName);
  info->comment=p->commentCodec->toUnicode(comment);
  info->extra=extra;
  info->dateTime=QDateTime(
      QDate(info_z.tmu_date.tm_year, info_z.tmu_date.tm_mon+1, info_z.tmu_date.tm_mday),
      QTime(info_z.tmu_date.tm_hour, info_z.tmu_date.tm_min, info_z.tmu_date.tm_sec));
  // Add to directory map
  p->addCurrentFileToDirectoryMap(info->name);
  return true;
}

QString QuaZip::getCurrentFileName()const
{
  qInfo() << "Getting current file name";
  QuaZip *fakeThis=(QuaZip*)this; // non-const
  fakeThis->p->zipError=UNZ_OK;
  if(p->mode!=mdUnzip) {
    qWarning("QuaZip::getCurrentFileName(): ZIP is not open in mdUnzip mode");
    qInfo() << "Not in mdUnzip mode";
    return QString();
  }
  if(!isOpen()||!hasCurrentFile()) {
      qInfo() << "Not open or no current file";
      return QString();
  }
  QByteArray fileName(MAX_FILE_NAME_LENGTH, 0);
  if((fakeThis->p->zipError=unzGetCurrentFileInfo64(p->unzFile_f, NULL, fileName.data(), fileName.size(),
      NULL, 0, NULL, 0))!=UNZ_OK) {
    qInfo() << "unzGetCurrentFileInfo64 error:" << p->zipError;
    return QString();
  }
  QString result = p->fileNameCodec->toUnicode(fileName.constData());
  if (result.isEmpty()) {
      qInfo() << "Empty filename";
      return result;
  }
  // Add to directory map
  p->addCurrentFileToDirectoryMap(result);
  return result;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void QuaZip::setFileNameCodec(QTextCodec *fileNameCodec)
{
  qInfo() << "Setting file name codec by pointer";
  p->fileNameCodec=fileNameCodec;
}

void QuaZip::setFileNameCodec(const char *fileNameCodecName)
{
  qInfo() << "Setting file name codec by name:" << fileNameCodecName;
  p->fileNameCodec=QTextCodec::codecForName(fileNameCodecName);
}

QTextCodec *QuaZip::getFileNameCodec()const
{
  return p->fileNameCodec;
}

void QuaZip::setCommentCodec(QTextCodec *commentCodec)
{
  qInfo() << "Setting comment codec by pointer";
  p->commentCodec=commentCodec;
}

void QuaZip::setCommentCodec(const char *commentCodecName)
{
  qInfo() << "Setting comment codec by name:" << commentCodecName;
  p->commentCodec=QTextCodec::codecForName(commentCodecName);
}

QTextCodec *QuaZip::getCommentCodec()const
{
  qInfo() << "Getting comment codec";
  return p->commentCodec;
}
#else
void QuaZip::setFileNameEncoding(QStringConverter::Encoding encoding)
{
    qInfo() << "Setting file name encoding to:" << encoding;
    p->fileNameEncoding = encoding;
}

QStringConverter::Encoding QuaZip::getFileNameEncoding() const
{
    qInfo() << "Getting file name encoding";
    return p->fileNameEncoding;
}

void QuaZip::setCommentEncoding(QStringConverter::Encoding encoding)
{
    qInfo() << "Setting comment encoding to:" << encoding;
    p->commentEncoding = encoding;
}

QStringConverter::Encoding QuaZip::getCommentEncoding() const
{
    qInfo() << "Getting comment encoding";
    return p->commentEncoding;
}
#endif

QString QuaZip::getZipName() const
{
  qInfo() << "Getting zip name";
  return p->zipName;
}

QIODevice *QuaZip::getIoDevice() const
{
  qInfo() << "Getting IO device";
  if (!p->zipName.isEmpty()) // opened by name, using an internal QIODevice
    return NULL;
  return p->ioDevice;
}

QuaZip::Mode QuaZip::getMode()const
{
  qInfo() << "Getting mode";
  return p->mode;
}

bool QuaZip::isOpen()const
{
  qInfo() << "Checking if open";
  return p->mode!=mdNotOpen;
}

int QuaZip::getZipError() const
{
  qInfo() << "Getting zip error";
  return p->zipError;
}

void QuaZip::setComment(const QString& comment)
{
  qInfo() << "Setting comment:" << comment;
  p->comment=comment;
}

bool QuaZip::hasCurrentFile()const
{
  qInfo() << "Checking for current file";
  return p->hasCurrentFile_f;
}

unzFile QuaZip::getUnzFile()
{
  qInfo() << "Getting unzFile handle";
  return p->unzFile_f;
}

zipFile QuaZip::getZipFile()
{
  qInfo() << "Getting zipFile handle";
  return p->zipFile_f;
}

void QuaZip::setDataDescriptorWritingEnabled(bool enabled)
{
    qInfo() << "Setting data descriptor writing enabled to:" << enabled;
    p->dataDescriptorWritingEnabled = enabled;
}

bool QuaZip::isDataDescriptorWritingEnabled() const
{
    qInfo() << "Checking if data descriptor writing is enabled";
    return p->dataDescriptorWritingEnabled;
}

template<typename TFileInfo>
TFileInfo QuaZip_getFileInfo(QuaZip *zip, bool *ok);

template<>
QuaZipFileInfo QuaZip_getFileInfo(QuaZip *zip, bool *ok)
{
    qInfo() << "Template helper: Getting QuaZipFileInfo";
    QuaZipFileInfo info;
    *ok = zip->getCurrentFileInfo(&info);
    return info;
}

template<>
QuaZipFileInfo64 QuaZip_getFileInfo(QuaZip *zip, bool *ok)
{
    qInfo() << "Template helper: Getting QuaZipFileInfo64";
    QuaZipFileInfo64 info;
    *ok = zip->getCurrentFileInfo(&info);
    return info;
}

template<>
QString QuaZip_getFileInfo(QuaZip *zip, bool *ok)
{
    qInfo() << "Template helper: Getting file name as QString";
    QString name = zip->getCurrentFileName();
    *ok = !name.isEmpty();
    return name;
}

template<typename TFileInfo>
bool QuaZipPrivate::getFileInfoList(QList<TFileInfo> *result) const
{
  qInfo() << "Getting file info list";
  QuaZipPrivate *fakeThis=const_cast<QuaZipPrivate*>(this);
  fakeThis->zipError=UNZ_OK;
  if (mode!=QuaZip::mdUnzip) {
    qWarning("QuaZip::getFileNameList/getFileInfoList(): "
            "ZIP is not open in mdUnzip mode");
    return false;
  }
  QString currentFile;
  if (q->hasCurrentFile()) {
      currentFile = q->getCurrentFileName();
  }
  if (q->goToFirstFile()) {
      do {
          bool ok;
          result->append(QuaZip_getFileInfo<TFileInfo>(q, &ok));
          if (!ok) {
              qInfo() << "QuaZip_getFileInfo failed";
              return false;
          }
      } while (q->goToNextFile());
  }
  if (zipError != UNZ_OK) {
      qInfo() << "zipError is not UNZ_OK:" << zipError;
      return false;
  }
  if (currentFile.isEmpty()) {
      if (!q->goToFirstFile()) {
          qInfo() << "goToFirstFile failed";
          return false;
      }
  } else {
      if (!q->setCurrentFile(currentFile)) {
          qInfo() << "setCurrentFile failed";
          return false;
      }
  }
  return true;
}

QStringList QuaZip::getFileNameList() const
{
    qInfo() << "Getting file name list";
    QStringList list;
    if (p->getFileInfoList(&list))
        return list;
    else {
        qInfo() << "getFileInfoList failed";
        return QStringList();
    }
}

QList<QuaZipFileInfo> QuaZip::getFileInfoList() const
{
    qInfo() << "Getting file info list (32-bit)";
    QList<QuaZipFileInfo> list;
    if (p->getFileInfoList(&list))
        return list;
    else {
        qInfo() << "getFileInfoList failed";
        return QList<QuaZipFileInfo>();
    }
}

QList<QuaZipFileInfo64> QuaZip::getFileInfoList64() const
{
    qInfo() << "Getting file info list (64-bit)";
    QList<QuaZipFileInfo64> list;
    if (p->getFileInfoList(&list))
        return list;
    else {
        qInfo() << "getFileInfoList failed";
        return QList<QuaZipFileInfo64>();
    }
}

Qt::CaseSensitivity QuaZip::convertCaseSensitivity(QuaZip::CaseSensitivity cs)
{
  qInfo() << "Converting case sensitivity enum";
  if (cs == csDefault) {
#ifdef Q_OS_WIN
      return Qt::CaseInsensitive;
#else
      return Qt::CaseSensitive;
#endif
  } else {
      return cs == csSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
  }
}

void QuaZip::setDefaultFileNameCodec(QTextCodec *codec)
{
    qInfo() << "Setting default file name codec by pointer";
    QuaZipPrivate::defaultFileNameCodec = codec;
}

void QuaZip::setDefaultFileNameCodec(const char *codecName)
{
    qInfo() << "Setting default file name codec by name:" << codecName;
    setDefaultFileNameCodec(QTextCodec::codecForName(codecName));
}

void QuaZip::setZip64Enabled(bool zip64)
{
    qInfo() << "Setting zip64 enabled to:" << zip64;
    p->zip64 = zip64;
}

bool QuaZip::isZip64Enabled() const
{
    qInfo() << "Checking if zip64 is enabled";
    return p->zip64;
}

bool QuaZip::isAutoClose() const
{
    qInfo() << "Checking if auto-close is enabled";
    return p->autoClose;
}

void QuaZip::setAutoClose(bool autoClose) const
{
    qInfo() << "Setting auto-close to:" << autoClose;
    p->autoClose = autoClose;
}
