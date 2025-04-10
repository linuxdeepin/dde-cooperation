// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "marshaller_p.h"
#include "message_p.h"


#include <QDataStream>
#include <QPair>
#include <QMetaType>
#include <QtGui/QImage>
#include <QBuffer>
#include <QDebug>
#include <QList>
#include <QLinkedList>
#include <QStack>
#include <QVector>


QByteArray SlotIPCMarshaller::marshallMessage(const SlotIPCMessage& message)
{
  QByteArray result;
  QDataStream stream(&result, QIODevice::WriteOnly);

  stream << message.messageType();
  stream << message.method();
  stream << message.returnType();
  stream << (quint32)message.arguments().size();

  bool successfullyMarshalled;
  foreach (const auto& arg, message.arguments())
  {
    successfullyMarshalled = marshallArgumentToStream(arg, stream);

    if (!successfullyMarshalled)
        return QByteArray();
  }

  return result;
}


SlotIPCMessage SlotIPCMarshaller::demarshallMessage(QByteArray& call)
{
  QDataStream stream(&call, QIODevice::ReadOnly);

  // Call type
  SlotIPCMessage::MessageType type;
  int buffer;
  stream >> buffer;
  type = SlotIPCMessage::MessageType(buffer);

  // Method
  QString method;
  stream >> method;

  QString returnType;
  stream >> returnType;

  // Arguments
  quint32 argc = 0;
  stream >> argc;
  Q_ASSERT(argc <= 10);

  SlotIPCMessage::Arguments args;
  for (int i = 0; i < argc; ++i)
  {
    bool ok;
    QGenericArgument argument = demarshallArgumentFromStream(ok, stream);
    if (!ok)
    {
      qWarning() << "SlotIPC:" << "Failed to deserialize argument" << i;
      break;
    }
    args.append(argument);
  }

  return SlotIPCMessage(type, method, args, returnType);
}


SlotIPCMessage::MessageType SlotIPCMarshaller::demarshallMessageType(QByteArray& message)
{
  QDataStream stream(&message, QIODevice::ReadOnly);
  // Call type
  int buffer;
  stream >> buffer;
  return SlotIPCMessage::MessageType(buffer);
}


SlotIPCMessage SlotIPCMarshaller::demarshallResponse(QByteArray& call, QGenericReturnArgument arg)
{
  QDataStream stream(&call, QIODevice::ReadOnly);

  SlotIPCMessage::MessageType type;
  int buffer;
  stream >> buffer;
  type = SlotIPCMessage::MessageType(buffer);

  QString method;
  stream >> method;

  QString returnType;
  stream >> returnType;

  SlotIPCMessage::Arguments args; //construct message with empty arguments

  int argc;
  stream >> argc;
  if (argc != 0) // load returned value to the arg (not to the message)
  {
    QString typeName;
    stream >> typeName;

    // Check type
    int type = QMetaType::type(typeName.toLatin1());
    if (type == 0)
      qWarning() << "SlotIPC:" << "Unsupported type of argument " << ":" << typeName;

    if (arg.name() && arg.data())
    {
      if (type != QMetaType::type(arg.name()))
        qWarning() << "SlotIPC:" << "Type doesn't match:" << typeName << "Expected:" << arg.name();

      bool dataLoaded = false;

      if (type == QMetaType::QImage)
        dataLoaded = loadQImage(stream, arg.data());
      else if (type == QMetaType::type("QLinkedList<QImage>"))
        dataLoaded = loadContainerOfQImages<QLinkedList>(stream, arg.data());
      else if (type == QMetaType::type("QList<QImage>"))
        dataLoaded = loadContainerOfQImages<QList>(stream, arg.data());
      else if (type == QMetaType::type("QStack<QImage>"))
        dataLoaded = loadContainerOfQImages<QStack>(stream, arg.data());
      else if (type == QMetaType::type("QVector<QImage>"))
        dataLoaded = loadContainerOfQImages<QVector>(stream, arg.data());
      else
        dataLoaded = QMetaType::load(stream, type, arg.data());

      if (!dataLoaded)
        qWarning() << "SlotIPC:" << "Failed to deserialize argument value" << "of type" << typeName;
    }
  }

  return SlotIPCMessage(type, method, args, returnType);
}


bool SlotIPCMarshaller::marshallArgumentToStream(QGenericArgument value, QDataStream& stream)
{
  // Detect and check type
  int type = QMetaType::type(value.name());
  if (type == 0)
  {
    qWarning() << "SlotIPC:" << "Type" << value.name() << "have not been registered in Qt metaobject system";
    return false;
  }
  if (type == QMetaType::QImage)
    return marshallQImageToStream(value, stream);
  else if (type == QMetaType::type("QLinkedList<QImage>"))
    return marshallContainerOfQImagesToStream<QLinkedList>(value, stream);
  else if (type == QMetaType::type("QList<QImage>"))
    return marshallContainerOfQImagesToStream<QList>(value, stream);
  else if (type == QMetaType::type("QStack<QImage>"))
    return marshallContainerOfQImagesToStream<QStack>(value, stream);
  else if (type == QMetaType::type("QVector<QImage>"))
    return marshallContainerOfQImagesToStream<QVector>(value, stream);

  stream << QString::fromLatin1(value.name());
  bool ok = QMetaType::save(stream, type, value.data());
  if (!ok)
  {
    qWarning() << "SlotIPC:" << "Failed to serialize" << value.name()
               << "to data stream. Call qRegisterMetaTypeStreamOperators to"
                  "register stream operators for this metatype";
    return false;
  }

  return true;
}


QGenericArgument SlotIPCMarshaller::demarshallArgumentFromStream(bool& ok, QDataStream& stream)
{
  // Load type
  QString typeName;
  stream >> typeName;

  // Check type
  int type = QMetaType::type(typeName.toLatin1());
  if (type == 0)
  {
    qWarning() << "SlotIPC:" << "Unsupported type of argument " << ":" << typeName;
    ok = false;
    return QGenericArgument();
  }

  // Read argument data from stream
#if QT_VERSION >= 0x050000
  void* data = QMetaType::create(type);
#else
  void* data = QMetaType::construct(type);
#endif

  bool dataLoaded = false;

  if (type == QMetaType::QImage)
    dataLoaded = loadQImage(stream, data);
  else if (type == QMetaType::type("QLinkedList<QImage>"))
    dataLoaded = loadContainerOfQImages<QLinkedList>(stream, data);
  else if (type == QMetaType::type("QList<QImage>"))
    dataLoaded = loadContainerOfQImages<QList>(stream, data);
  else if (type == QMetaType::type("QStack<QImage>"))
    dataLoaded = loadContainerOfQImages<QStack>(stream, data);
  else if (type == QMetaType::type("QVector<QImage>"))
    dataLoaded = loadContainerOfQImages<QVector>(stream, data);
  else
    dataLoaded = QMetaType::load(stream, type, data);

  if (!dataLoaded)
  {
    qWarning() << "SlotIPC:" << "Failed to deserialize argument value" << "of type" << typeName;
    QMetaType::destroy(type, data);
    ok = false;
    return QGenericArgument();
  }

  ok = true;
  return QGenericArgument(qstrdup(typeName.toLatin1()), data);
}


bool SlotIPCMarshaller::marshallQImageToStream(QGenericArgument value, QDataStream& stream)
{
  QImage* image = static_cast<QImage*>(value.data());
  const uchar* imageData = image->constBits();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  const int size = image->byteCount();
#else
  const int size = image->sizeInBytes();
#endif

  stream << QString::fromLatin1(value.name());

  stream << image->width();
  stream << image->height();
  stream << image->bytesPerLine();
  stream << image->format();

  stream << image->dotsPerMeterX();
  stream << image->dotsPerMeterY();

  stream << image->colorTable();

  stream << size;
  stream.writeRawData(reinterpret_cast<const char*>(imageData), size);

  return true;
}


template <template<class QImage> class Container>
bool SlotIPCMarshaller::marshallContainerOfQImagesToStream(QGenericArgument value, QDataStream& stream)
{
  const Container<QImage>* imgContainer = static_cast<Container<QImage>*>(value.data());

  Container<QByteArray> dataContainer;
  for (typename Container<QImage>::const_iterator it = imgContainer->begin(); it != imgContainer->end(); ++it)
  {
    QByteArray dt;
    QDataStream dtStream(&dt, QIODevice::WriteOnly);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGenericArgument genArg = Q_ARG(QImage, *it);
#else
    auto arg = Q_ARG(QImage, *it);
    QGenericArgument genArg("QImage", arg.data);
#endif

    if (!marshallQImageToStream(genArg, dtStream))
      return false;
    dataContainer << dt;
  }

  stream << QString::fromLatin1(value.name());
  stream << dataContainer.size();
  for (typename Container<QByteArray>::iterator it = dataContainer.begin(); it != dataContainer.end(); ++it)
    stream << *it;

  return true;
}


bool SlotIPCMarshaller::loadQImage(QDataStream& stream, void* data)
{
  // Image parameters
  int width;
  stream >> width;
  int height;
  stream >> height;
  int bytesPerLine;
  stream >> bytesPerLine;
  int format;
  stream >> format;

  int dpmX;
  stream >> dpmX;
  int dpmY;
  stream >> dpmY;

  // Color table
  QVector<QRgb> colorTable;
  stream >> colorTable;

  // Read image bytes
  int byteCount;
  stream >> byteCount;

  uchar* bits = new uchar[byteCount];
  if (stream.readRawData(reinterpret_cast<char*>(bits), byteCount) != byteCount)
  {
    qWarning() << "SlotIPC:" << "Failed to deserialize argument value" << "of type" << "QImage";
    return false;
  }

  QImage image(const_cast<const uchar*>(bits), width, height, bytesPerLine, QImage::Format(format));
  image.setDotsPerMeterX(dpmX);
  image.setDotsPerMeterY(dpmY);
  image.setColorTable(colorTable);
  delete[] bits;

  *static_cast<QImage*>(data) = image;
  return true;
}


template <template<class QImage> class Container>
bool SlotIPCMarshaller::loadContainerOfQImages(QDataStream& stream, void* data)
{
  Container<QByteArray> dataContainer;
  int dataContainerSize = 0;
  stream >> dataContainerSize;
  for (int i = 0; i < dataContainerSize; ++i)
  {
    QByteArray dt;
    stream >> dt;
    dataContainer << dt;
    if (stream.atEnd())
      break;
  }

  Container<QImage> imgContainer;
  for (typename Container<QByteArray>::iterator it = dataContainer.begin(); it != dataContainer.end(); ++it)
  {
    QImage img;
    QDataStream imgStream(&(*it), QIODevice::ReadOnly);

    QString typeName;
    imgStream >> typeName;

    if (QMetaType::type(typeName.toLatin1()) != QMetaType::QImage)
      return false;

    if (!loadQImage(imgStream, static_cast<void*>(&img)))
      return false;

    imgContainer << img;
  }

  *static_cast<Container<QImage>*>(data) = imgContainer;
  return true;
}


void SlotIPCMarshaller::freeArguments(const SlotIPCMessage::Arguments& args)
{
  // Free allocated memory
  for (int i = 0; i < args.size(); ++i)
    freeArgument(args.at(i));
}


void SlotIPCMarshaller::freeArgument(QGenericArgument arg)
{
  if (arg.data())
    QMetaType::destroy(QMetaType::type(arg.name()), arg.data());

  delete[] arg.name();
}
