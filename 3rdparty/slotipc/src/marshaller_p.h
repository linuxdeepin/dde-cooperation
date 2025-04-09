// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARSHALLER_P_H
#define MARSHALLER_P_H


#include <QString>
#include <QByteArray>
#include <QPair>
#include <QGenericArgument>

// local
#include "message_p.h"

class SlotIPCMarshaller
{
  public:
    static QByteArray marshallMessage(const SlotIPCMessage& message);
    static SlotIPCMessage demarshallMessage(QByteArray& call);
    static SlotIPCMessage demarshallResponse(QByteArray& call, QGenericReturnArgument arg);

    static SlotIPCMessage::MessageType demarshallMessageType(QByteArray& message);

    static void freeArguments(const SlotIPCMessage::Arguments&);
    static void freeArgument(QGenericArgument);

  private:
    static bool marshallArgumentToStream(QGenericArgument value, QDataStream& stream);
    static QGenericArgument demarshallArgumentFromStream(bool& ok, QDataStream& stream);

    static bool marshallQImageToStream(QGenericArgument value, QDataStream& stream);

    template <template<class QImage> class Container>
    static bool marshallContainerOfQImagesToStream(QGenericArgument value, QDataStream& stream);

    static bool loadQImage(QDataStream& stream, void* data);

    template <template<class QImage> class Container>
    static bool loadContainerOfQImages(QDataStream& stream, void* data);
};

#endif // MARSHALLER_P_H
