// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quaziodevice.h"
#include <QDebug>

#define QUAZIO_INBUFSIZE 4096
#define QUAZIO_OUTBUFSIZE 4096

/// \cond internal
class QuaZIODevicePrivate {
    friend class QuaZIODevice;
    QuaZIODevicePrivate(QIODevice *io);
    ~QuaZIODevicePrivate();
    QIODevice *io;
    z_stream zins;
    z_stream zouts;
    char *inBuf;
    int inBufPos;
    int inBufSize;
    char *outBuf;
    int outBufPos;
    int outBufSize;
    bool zBufError;
    bool atEnd;
    int doFlush(QString &error);
};

QuaZIODevicePrivate::QuaZIODevicePrivate(QIODevice *io):
  io(io),
  inBuf(NULL),
  inBufPos(0),
  inBufSize(0),
  outBuf(NULL),
  outBufPos(0),
  outBufSize(0),
  zBufError(false),
  atEnd(false)
{
  qInfo() << "Constructing QuaZIODevicePrivate";
  zins.zalloc = (alloc_func) NULL;
  zins.zfree = (free_func) NULL;
  zins.opaque = NULL;
  zouts.zalloc = (alloc_func) NULL;
  zouts.zfree = (free_func) NULL;
  zouts.opaque = NULL;
  inBuf = new char[QUAZIO_INBUFSIZE];
  outBuf = new char[QUAZIO_OUTBUFSIZE];
#ifdef ZIODEVICE_DEBUG_OUTPUT
  debug.setFileName("debug.out");
  debug.open(QIODevice::WriteOnly);
#endif
#ifdef ZIODEVICE_DEBUG_INPUT
  indebug.setFileName("debug.in");
  indebug.open(QIODevice::WriteOnly);
#endif
}

QuaZIODevicePrivate::~QuaZIODevicePrivate()
{
  qInfo() << "Destructing QuaZIODevicePrivate";
#ifdef ZIODEVICE_DEBUG_OUTPUT
  debug.close();
#endif
#ifdef ZIODEVICE_DEBUG_INPUT
  indebug.close();
#endif
  if (inBuf != NULL)
    delete[] inBuf;
  if (outBuf != NULL)
    delete[] outBuf;
}

int QuaZIODevicePrivate::doFlush(QString &error)
{
  qInfo() << "Flushing internal buffer to underlying IO device";
  int flushed = 0;
  while (outBufPos < outBufSize) {
    int more = io->write(outBuf + outBufPos, outBufSize - outBufPos);
    if (more == -1) {
      error = io->errorString();
      qInfo() << "write error:" << error;
      return -1;
    }
    if (more == 0) {
      qInfo() << "write returned 0";
      break;
    }
    outBufPos += more;
    flushed += more;
  }
  if (outBufPos == outBufSize) {
    outBufPos = outBufSize = 0;
  }
  return flushed;
}

/// \endcond

// #define ZIODEVICE_DEBUG_OUTPUT
// #define ZIODEVICE_DEBUG_INPUT
#ifdef ZIODEVICE_DEBUG_OUTPUT
#include <QFile>
static QFile debug;
#endif
#ifdef ZIODEVICE_DEBUG_INPUT
#include <QFile>
static QFile indebug;
#endif

QuaZIODevice::QuaZIODevice(QIODevice *io, QObject *parent):
    QIODevice(parent),
    d(new QuaZIODevicePrivate(io))
{
  qInfo() << "Constructing QuaZIODevice";
  connect(io, SIGNAL(readyRead()), SIGNAL(readyRead()));
}

QuaZIODevice::~QuaZIODevice()
{
    qInfo() << "Destructing QuaZIODevice";
    if (isOpen())
        close();
    delete d;
}

QIODevice *QuaZIODevice::getIoDevice() const
{
    qInfo() << "Getting underlying IO device";
    return d->io;
}

bool QuaZIODevice::open(QIODevice::OpenMode mode)
{
    qInfo() << "Opening QuaZIODevice with mode:" << mode;
    if ((mode & QIODevice::Append) != 0) {
        setErrorString(tr("QIODevice::Append is not supported for"
                    " QuaZIODevice"));
        qInfo() << "Append mode not supported";
        return false;
    }
    if ((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
        setErrorString(tr("QIODevice::ReadWrite is not supported for"
                    " QuaZIODevice"));
        qInfo() << "ReadWrite mode not supported";
        return false;
    }
    if ((mode & QIODevice::ReadOnly) != 0) {
        if (inflateInit(&d->zins) != Z_OK) {
            setErrorString(d->zins.msg);
            qInfo() << "inflateInit failed:" << d->zins.msg;
            return false;
        }
    }
    if ((mode & QIODevice::WriteOnly) != 0) {
        if (deflateInit(&d->zouts, Z_DEFAULT_COMPRESSION) != Z_OK) {
            setErrorString(d->zouts.msg);
            qInfo() << "deflateInit failed:" << d->zouts.msg;
            return false;
        }
    }
    return QIODevice::open(mode);
}

void QuaZIODevice::close()
{
    qInfo() << "Closing QuaZIODevice";
    if ((openMode() & QIODevice::ReadOnly) != 0) {
        if (inflateEnd(&d->zins) != Z_OK) {
            setErrorString(d->zins.msg);
            qInfo() << "inflateEnd failed:" << d->zins.msg;
        }
    }
    if ((openMode() & QIODevice::WriteOnly) != 0) {
        flush();
        if (deflateEnd(&d->zouts) != Z_OK) {
            setErrorString(d->zouts.msg);
            qInfo() << "deflateEnd failed:" << d->zouts.msg;
        }
    }
    QIODevice::close();
}

qint64 QuaZIODevice::readData(char *data, qint64 maxSize)
{
  qInfo() << "Reading data, max size:" << maxSize;
  int read = 0;
  while (read < maxSize) {
    if (d->inBufPos == d->inBufSize) {
      d->inBufPos = 0;
      d->inBufSize = d->io->read(d->inBuf, QUAZIO_INBUFSIZE);
      if (d->inBufSize == -1) {
        d->inBufSize = 0;
        setErrorString(d->io->errorString());
        qInfo() << "read error:" << d->io->errorString();
        return -1;
      }
      if (d->inBufSize == 0) {
        qInfo() << "read returned 0";
        break;
      }
    }
    while (read < maxSize && d->inBufPos < d->inBufSize) {
      d->zins.next_in = (Bytef *) (d->inBuf + d->inBufPos);
      d->zins.avail_in = d->inBufSize - d->inBufPos;
      d->zins.next_out = (Bytef *) (data + read);
      d->zins.avail_out = (uInt) (maxSize - read); // hope it's less than 2GB
      int more = 0;
      switch (inflate(&d->zins, Z_SYNC_FLUSH)) {
      case Z_OK:
        read = (char *) d->zins.next_out - data;
        d->inBufPos = (char *) d->zins.next_in - d->inBuf;
        break;
      case Z_STREAM_END:
        read = (char *) d->zins.next_out - data;
        d->inBufPos = (char *) d->zins.next_in - d->inBuf;
        d->atEnd = true;
        return read;
      case Z_BUF_ERROR: // this should never happen, but just in case
        if (!d->zBufError) {
          qWarning("Z_BUF_ERROR detected with %d/%d in/out, weird",
              d->zins.avail_in, d->zins.avail_out);
          qInfo() << "Z_BUF_ERROR";
          d->zBufError = true;
        }
        memmove(d->inBuf, d->inBuf + d->inBufPos, d->inBufSize - d->inBufPos);
        d->inBufSize -= d->inBufPos;
        d->inBufPos = 0;
        more = d->io->read(d->inBuf + d->inBufSize, QUAZIO_INBUFSIZE - d->inBufSize);
        if (more == -1) {
          setErrorString(d->io->errorString());
          qInfo() << "read error:" << d->io->errorString();
          return -1;
        }
        if (more == 0) {
          qInfo() << "read returned 0";
          return read;
        }
        d->inBufSize += more;
        break;
      default:
        setErrorString(QString::fromLocal8Bit(d->zins.msg));
        qInfo() << "inflate error:" << d->zins.msg;
        return -1;
      }
    }
  }
#ifdef ZIODEVICE_DEBUG_INPUT
  indebug.write(data, read);
#endif
  return read;
}

qint64 QuaZIODevice::writeData(const char *data, qint64 maxSize)
{
  qInfo() << "Writing data, size:" << maxSize;
  int written = 0;
  QString error;
  if (d->doFlush(error) == -1) {
    setErrorString(error);
    qInfo() << "flush error:" << error;
    return -1;
  }
  while (written < maxSize) {
      // there is some data waiting in the output buffer
    if (d->outBufPos < d->outBufSize) {
      qInfo() << "output buffer not empty";
      return written;
    }
    d->zouts.next_in = (Bytef *) (data + written);
    d->zouts.avail_in = (uInt) (maxSize - written); // hope it's less than 2GB
    d->zouts.next_out = (Bytef *) d->outBuf;
    d->zouts.avail_out = QUAZIO_OUTBUFSIZE;
    switch (deflate(&d->zouts, Z_NO_FLUSH)) {
    case Z_OK:
      written = (char *) d->zouts.next_in - data;
      d->outBufSize = (char *) d->zouts.next_out - d->outBuf;
      break;
    default:
      setErrorString(QString::fromLocal8Bit(d->zouts.msg));
      qInfo() << "deflate error:" << d->zouts.msg;
      return -1;
    }
    if (d->doFlush(error) == -1) {
      setErrorString(error);
      qInfo() << "flush error:" << error;
      return -1;
    }
  }
#ifdef ZIODEVICE_DEBUG_OUTPUT
  debug.write(data, written);
#endif
  return written;
}

bool QuaZIODevice::flush()
{
    qInfo() << "Flushing QuaZIODevice";
    QString error;
    if (d->doFlush(error) < 0) {
        setErrorString(error);
        qInfo() << "flush error:" << error;
        return false;
    }
    // can't flush buffer, some data is still waiting
    if (d->outBufPos < d->outBufSize) {
        qInfo() << "output buffer not empty";
        return true;
    }
    Bytef c = 0;
    d->zouts.next_in = &c; // fake input buffer
    d->zouts.avail_in = 0; // of zero size
    do {
        d->zouts.next_out = (Bytef *) d->outBuf;
        d->zouts.avail_out = QUAZIO_OUTBUFSIZE;
        switch (deflate(&d->zouts, Z_SYNC_FLUSH)) {
        case Z_OK:
          d->outBufSize = (char *) d->zouts.next_out - d->outBuf;
          if (d->doFlush(error) < 0) {
              setErrorString(error);
              qInfo() << "flush error:" << error;
              return false;
          }
          if (d->outBufPos < d->outBufSize) {
              qInfo() << "output buffer not empty";
              return true;
          }
          break;
        case Z_BUF_ERROR: // nothing to write?
          qInfo() << "Z_BUF_ERROR";
          return true;
        default:
          setErrorString(QString::fromLocal8Bit(d->zouts.msg));
          qInfo() << "deflate error:" << d->zouts.msg;
          return false;
        }
    } while (d->zouts.avail_out == 0);
    return true;
}

bool QuaZIODevice::isSequential() const
{
    qInfo() << "Checking if device is sequential";
    return true;
}

bool QuaZIODevice::atEnd() const
{
    qInfo() << "Checking if at end of stream";
    // Here we MUST check QIODevice::bytesAvailable() because WE
    // might have reached the end, but QIODevice didn't--
    // it could have simply pre-buffered all remaining data.
    return (openMode() == NotOpen) || (QIODevice::bytesAvailable() == 0 && d->atEnd);
}

qint64 QuaZIODevice::bytesAvailable() const
{
    qInfo() << "Checking bytes available";
    // If we haven't recevied Z_STREAM_END, it means that
    // we have at least one more input byte available.
    // Plus whatever QIODevice has buffered.
    return (d->atEnd ? 0 : 1) + QIODevice::bytesAvailable();
}