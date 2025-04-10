// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INTERFACE_H
#define INTERFACE_H

#include "argdefine.h"

#include <QObject>

#if __cplusplus >= 201402L
#  define DECL_DEPRECATED(x) [[deprecated(x)]]
#elif defined(__GNUC__)
#  define DECL_DEPRECATED(x) __attribute__((deprecated(x)))
#elif defined(_MSC_VER)
// 暂时移除或注释掉这部分代码
// #  define DECL_DEPRECATED(x) __pragma deprecated(x)
#  define DECL_DEPRECATED(x)
#else
#  define DECL_DEPRECATED(x)
#endif

class QHostAddress;
class SlotIPCInterfacePrivate;
class Q_DECL_EXPORT SlotIPCInterface : public QObject
{
  Q_OBJECT

  public:
    SlotIPCInterface(QObject* parent = 0);
    ~SlotIPCInterface();

    bool connectToServer(const QString& name);
    bool connectToServer(const QHostAddress& host, quint16 port);

    void disconnectFromServer();
    bool isConnected();
    bool remoteConnect(const char* signal, QObject* object, const char* method);
    bool remoteConnect(QObject* localObject, const char* localSignal, const char* remoteMethod);

    DECL_DEPRECATED("Replaced by remoteConnect(), which can connect to both signals and slots")
    bool remoteSlotConnect(QObject* localObject, const char* signal, const char* remoteSlot);

    bool disconnectSignal(const char* signal, QObject* object, const char* method);
    bool disconnectRemoteMethod(QObject* localObject, const char* signal, const char* remoteMethod);

    DECL_DEPRECATED("Replaced by disconnectRemoteMethod(), which can disconnect from both signals and slots")
    bool disconnectSlot(QObject* localObject, const char* signal, const char* remoteSlot);

    bool call(const QString& method, METHOD_RE_ARG ret,
              METHOD_ARG val0 = METHOD_ARG(),
              METHOD_ARG val1 = METHOD_ARG(),
              METHOD_ARG val2 = METHOD_ARG(),
              METHOD_ARG val3 = METHOD_ARG(),
              METHOD_ARG val4 = METHOD_ARG(),
              METHOD_ARG val5 = METHOD_ARG(),
              METHOD_ARG val6 = METHOD_ARG(),
              METHOD_ARG val7 = METHOD_ARG(),
              METHOD_ARG val8 = METHOD_ARG(),
              METHOD_ARG val9 = METHOD_ARG());

    bool call(const QString& method,
              METHOD_ARG val0 = METHOD_ARG(),
              METHOD_ARG val1 = METHOD_ARG(),
              METHOD_ARG val2 = METHOD_ARG(),
              METHOD_ARG val3 = METHOD_ARG(),
              METHOD_ARG val4 = METHOD_ARG(),
              METHOD_ARG val5 = METHOD_ARG(),
              METHOD_ARG val6 = METHOD_ARG(),
              METHOD_ARG val7 = METHOD_ARG(),
              METHOD_ARG val8 = METHOD_ARG(),
              METHOD_ARG val9 = METHOD_ARG());

    void callNoReply(const QString& method,
              METHOD_ARG val0 = METHOD_ARG(),
              METHOD_ARG val1 = METHOD_ARG(),
              METHOD_ARG val2 = METHOD_ARG(),
              METHOD_ARG val3 = METHOD_ARG(),
              METHOD_ARG val4 = METHOD_ARG(),
              METHOD_ARG val5 = METHOD_ARG(),
              METHOD_ARG val6 = METHOD_ARG(),
              METHOD_ARG val7 = METHOD_ARG(),
              METHOD_ARG val8 = METHOD_ARG(),
              METHOD_ARG val9 = METHOD_ARG());


    QString lastError() const;

  signals:
    void connected();
    void disconnected();


  protected:
    SlotIPCInterfacePrivate* const d_ptr;
    SlotIPCInterface(SlotIPCInterfacePrivate& dd, QObject* parent);

    bool callImpl(const QString& method, 
                 const QGenericReturnArgument& ret,
                 const QGenericArgument* args,
                 int argc);

  private:
    Q_DECLARE_PRIVATE(SlotIPCInterface)
    Q_PRIVATE_SLOT(d_func(),void _q_sendAsynchronousRequest(QByteArray))
    Q_PRIVATE_SLOT(d_func(),void _q_removeSignalHandlersOfObject(QObject*))
    Q_PRIVATE_SLOT(d_func(), void _q_setLastError(QString))
    Q_PRIVATE_SLOT(d_func(), void _q_invokeRemoteSignal(QString, SlotIPCMessage::Arguments))
    Q_PRIVATE_SLOT(d_func(), void _q_removeRemoteConnectionsOfObject(QObject*))
};

#endif // INTERFACE_H
