// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "message_p.h"


#include <QDebug>

SlotIPCMessage::SlotIPCMessage(MessageType type, const QString& method,
                               METHOD_ARG val0,
                               METHOD_ARG val1,
                               METHOD_ARG val2,
                               METHOD_ARG val3,
                               METHOD_ARG val4,
                               METHOD_ARG val5,
                               METHOD_ARG val6,
                               METHOD_ARG val7,
                               METHOD_ARG val8,
                               METHOD_ARG val9,
                               const QString& returnType)
{
  m_messageType = type;
  m_method = method;
  m_returnType = returnType;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  // Qt6: 需要转换参数类型
  const METHOD_ARG args[] = {
    val0, val1, val2, val3, val4, val5, val6, val7, val8, val9
  };
  
  for(int i = 0; i < 10; ++i) {
    if(args[i].name) {
      // m_arguments.append(args[i]);
      QGenericArgument arg(qstrdup(args[i].name), args[i].data);
      m_arguments.append(arg);
    }
  }
#else
  // Qt5: 直接使用参数
  if (val0.name()) m_arguments.append(val0);
  if (val1.name()) m_arguments.append(val1);
  if (val2.name()) m_arguments.append(val2);
  if (val3.name()) m_arguments.append(val3);
  if (val4.name()) m_arguments.append(val4);
  if (val5.name()) m_arguments.append(val5);
  if (val6.name()) m_arguments.append(val6);
  if (val7.name()) m_arguments.append(val7);
  if (val8.name()) m_arguments.append(val8);
  if (val9.name()) m_arguments.append(val9);
#endif
}


SlotIPCMessage::SlotIPCMessage(MessageType type, const QString& method,
                               const Arguments& arguments, const QString& returnType)
{
  m_method = method;
  m_arguments = arguments;
  m_messageType = type;
  m_returnType = returnType;
}


const QString& SlotIPCMessage::method() const
{
  return m_method;
}


const SlotIPCMessage::Arguments& SlotIPCMessage::arguments() const
{
  return m_arguments;
}


const SlotIPCMessage::MessageType& SlotIPCMessage::messageType() const
{
  return m_messageType;
}


const QString& SlotIPCMessage::returnType() const
{
  return m_returnType;
}


QDebug operator<<(QDebug dbg, const SlotIPCMessage& message)
{
  QString type;
  switch (message.messageType())
  {
    case SlotIPCMessage::MessageCallWithReturn:
      type = "CallWithReturn";
      break;
    case SlotIPCMessage::MessageCallWithoutReturn:
      type = "CallWithoutReturn";
      break;
    case SlotIPCMessage::MessageResponse:
      type = "Response";
      break;
    case SlotIPCMessage::MessageError:
      type = "Error";
      break;
    case SlotIPCMessage::SignalConnectionRequest:
      type = "SignalConnectionRequest";
      break;
    case SlotIPCMessage::MessageSignal:
      type = "Signal";
      break;
    case SlotIPCMessage::SlotConnectionRequest:
      type = "SlotConnectionRequest";
      break;
    case SlotIPCMessage::AboutToCloseSocket:
      type = "AboutToCloseSocket";
      break;
    case SlotIPCMessage::ConnectionInitialize:
      type = "ConnectionInitialize";
      break;
    default: break;
  }

  dbg.nospace() << "MESSAGE of type: " << type << "  " << "Method: " << message.method();

  if (message.arguments().length())
  {
    dbg.nospace() << "  " << "Arguments of type: ";
    foreach (const auto& arg, message.arguments())
    {
      dbg.space() << arg.name();
    }
  }

  if (!message.returnType().isEmpty())
    dbg.space() << " " << "Return type:" << message.returnType();

  return dbg.space();
}
