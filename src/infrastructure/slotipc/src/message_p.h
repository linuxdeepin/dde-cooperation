// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MESSAGE_P_H
#define MESSAGE_P_H


#include <QString>
#include <QGenericArgument>
#include <QList>

// 添加宏定义
#include "slotipc/argdefine.h"

#define DEBUG if (qgetenv("SLOTIPC_DEBUG") == "1") qDebug() << "SlotIPC:"

class SlotIPCMessage
{
  public:
    typedef QList<QGenericArgument> Arguments;

    enum MessageType
    {
      MessageCallWithReturn,
      MessageCallWithoutReturn,
      MessageResponse,
      MessageError,
      SignalConnectionRequest,
      SlotConnectionRequest,
      MessageSignal,
      AboutToCloseSocket,
      ConnectionInitialize
    };

    SlotIPCMessage(MessageType type,
                   const QString& method = QString(),
                   METHOD_ARG val0 = METHOD_ARG(),
                   METHOD_ARG val1 = METHOD_ARG(),
                   METHOD_ARG val2 = METHOD_ARG(),
                   METHOD_ARG val3 = METHOD_ARG(),
                   METHOD_ARG val4 = METHOD_ARG(),
                   METHOD_ARG val5 = METHOD_ARG(),
                   METHOD_ARG val6 = METHOD_ARG(),
                   METHOD_ARG val7 = METHOD_ARG(),
                   METHOD_ARG val8 = METHOD_ARG(),
                   METHOD_ARG val9 = METHOD_ARG(),
                   const QString& returnType = QString());

    SlotIPCMessage(MessageType type, const QString& method, const Arguments& arguments,
                   const QString& returnType = QString());


    const MessageType& messageType() const;
    const QString& method() const;
    const QString& returnType() const;
    const Arguments& arguments() const;

  private:
    QString m_method;
    Arguments m_arguments;
    MessageType m_messageType;
    QString m_returnType;
};

QDebug operator<<(QDebug dbg, const SlotIPCMessage& message);


#endif // MESSAGE_P_H
