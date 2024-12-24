#ifndef CUTEIPCMESSAGE_P_H
#define CUTEIPCMESSAGE_P_H

// Qt
#include <QString>
#include <QGenericArgument>
#include <QList>

// 添加宏定义
#include "CuteIPCGlobal.h"

#define DEBUG if (qgetenv("CUTEIPC_DEBUG") == "1") qDebug() << "CuteIPC:"

class CuteIPCMessage
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

    CuteIPCMessage(MessageType type,
                   const QString& method = QString(),
                   CUTE_IPC_ARG val0 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val1 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val2 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val3 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val4 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val5 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val6 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val7 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val8 = CUTE_IPC_ARG(),
                   CUTE_IPC_ARG val9 = CUTE_IPC_ARG(),
                   const QString& returnType = QString());

    CuteIPCMessage(MessageType type, const QString& method, const Arguments& arguments,
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

QDebug operator<<(QDebug dbg, const CuteIPCMessage& message);


#endif // CUTEIPCMESSAGE_P_H
