// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUAZIODEVICE_H
#define QUAZIODEVICE_H

#include <QIODevice>
#include "exportdefine.h"

#include <zlib.h>

class QuaZIODevicePrivate;

// Class for compressing/decompressing QIODevice streams
class DLL_EXPORT QuaZIODevice: public QIODevice {
  Q_OBJECT
public:
  // Constructor with IO device and optional parent
  QuaZIODevice(QIODevice *io, QObject *parent = NULL);
  // Destructor
  ~QuaZIODevice();
  // Flush buffered data (may need to call underlying device's flush)
  virtual bool flush();
  // Open device (ReadWrite and Append modes not supported)
  virtual bool open(QIODevice::OpenMode mode);
  // Close this device (underlying device remains open)
  virtual void close();
  // Get underlying IO device
  QIODevice *getIoDevice() const;
  // Check if device is sequential
  virtual bool isSequential() const;
  // Check if at end of compressed stream
  virtual bool atEnd() const;
  // Get number of buffered bytes available
  virtual qint64 bytesAvailable() const;
protected:
  // Read data implementation
  virtual qint64 readData(char *data, qint64 maxSize);
  // Write data implementation
  virtual qint64 writeData(const char *data, qint64 maxSize);
private:
  QuaZIODevicePrivate *d;
};
#endif // QUAZIODEVICE_H
