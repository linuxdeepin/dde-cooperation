// Local
#include "CuteIPCMessage_p.h"

// Qt
#include <QDebug>

CuteIPCMessage::CuteIPCMessage(MessageType type, const QString& method,
                               CUTE_IPC_ARG val0,
                               CUTE_IPC_ARG val1,
                               CUTE_IPC_ARG val2,
                               CUTE_IPC_ARG val3,
                               CUTE_IPC_ARG val4,
                               CUTE_IPC_ARG val5,
                               CUTE_IPC_ARG val6,
                               CUTE_IPC_ARG val7,
                               CUTE_IPC_ARG val8,
                               CUTE_IPC_ARG val9,
                               const QString& returnType)
{
  m_messageType = type;
  m_method = method;
  m_returnType = returnType;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  // Qt6: 需要转换参数类型
  const CUTE_IPC_ARG args[] = {
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


CuteIPCMessage::CuteIPCMessage(MessageType type, const QString& method,
                               const Arguments& arguments, const QString& returnType)
{
  m_method = method;
  m_arguments = arguments;
  m_messageType = type;
  m_returnType = returnType;
}


const QString& CuteIPCMessage::method() const
{
  return m_method;
}


const CuteIPCMessage::Arguments& CuteIPCMessage::arguments() const
{
  return m_arguments;
}


const CuteIPCMessage::MessageType& CuteIPCMessage::messageType() const
{
  return m_messageType;
}


const QString& CuteIPCMessage::returnType() const
{
  return m_returnType;
}


QDebug operator<<(QDebug dbg, const CuteIPCMessage& message)
{
  QString type;
  switch (message.messageType())
  {
    case CuteIPCMessage::MessageCallWithReturn:
      type = "CallWithReturn";
      break;
    case CuteIPCMessage::MessageCallWithoutReturn:
      type = "CallWithoutReturn";
      break;
    case CuteIPCMessage::MessageResponse:
      type = "Response";
      break;
    case CuteIPCMessage::MessageError:
      type = "Error";
      break;
    case CuteIPCMessage::SignalConnectionRequest:
      type = "SignalConnectionRequest";
      break;
    case CuteIPCMessage::MessageSignal:
      type = "Signal";
      break;
    case CuteIPCMessage::SlotConnectionRequest:
      type = "SlotConnectionRequest";
      break;
    case CuteIPCMessage::AboutToCloseSocket:
      type = "AboutToCloseSocket";
      break;
    case CuteIPCMessage::ConnectionInitialize:
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
