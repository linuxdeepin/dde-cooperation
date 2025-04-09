// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slotipc//interface.h"
#include "interface_p.h"
#include "marshaller_p.h"
#include "interfaceconnection_p.h"
#include "message_p.h"
#include "signalhandler_p.h"
#include "interfaceworker.h"


#include <QThread>
#include <QEventLoop>
#include <QHostAddress>


/*!
    \class SlotIPCInterface

    \brief The SlotIPCInterface class provides an IPC client
    used to send remote call requests and set connections between remote
    signals and slots.

    It is based on [QLocalSocket](http://doc.qt.io/qt-5/qlocalsocket.html)
    and [QTcpSocket](http://doc.qt.io/qt-5/qtcpsocket.html).
    To connect to the server, call connectToServer() method.

    Use call() and callNoReply() methods to send method invoke requests to the server
    (which are synchronous and asynchronous respectively).
    The signature of these methods concurs with QMetaObject::invokeMethod() method signature.
    Thus, you can invoke remote methods the same way as you did it locally through the QMetaObject.

    You can also use a remoteConnect() to connect the remote signal to the slot or signal
    of some local object.

    Contrarily, you can connect the local signal to the remote slot, by using
    remoteSlotConnect().

    \sa SlotIPCService
*/

SlotIPCInterfacePrivate::SlotIPCInterfacePrivate()
  : m_workerThread(new QThread),
    m_worker(new SlotIPCInterfaceWorker)
{
  m_worker->moveToThread(m_workerThread);
  m_workerThread->start();
}


SlotIPCInterfacePrivate::~SlotIPCInterfacePrivate()
{
  m_workerThread->quit();
  m_workerThread->wait();

  delete m_worker;
  delete m_workerThread;
}


bool SlotIPCInterfacePrivate::checkConnectCorrection(const QString& signal, const QString& method)
{
  if (signal[0] != '2' || (method[0] != '1' && method[0] != '2'))
    return false;

  QString signalSignature = signal.mid(1);
  QString methodSignature = method.mid(1);

  if (!QMetaObject::checkConnectArgs(signalSignature.toLatin1(), methodSignature.toLatin1()))
  {
    qWarning() << "SlotIPC:" << "Error: incompatible signatures" << signalSignature << methodSignature;
    m_lastError = "Incompatible signatures: " + signalSignature + "," + methodSignature;
    return false;
  }
  return true;
}


bool SlotIPCInterfacePrivate::checkRemoteSlotExistance(const QString& slot)
{
  DEBUG << "Check remote slot existence" << slot;
  SlotIPCMessage message(SlotIPCMessage::SlotConnectionRequest, slot);
  QByteArray request = SlotIPCMarshaller::marshallMessage(message);
  return sendSynchronousRequest(request);
}


bool SlotIPCInterfacePrivate::sendRemoteConnectRequest(const QString& signalSignature)
{
  QString connectionId = m_worker->connectionId();

  DEBUG << "Requesting connection to signal" << signalSignature << "Worker connection ID: " << connectionId;

  SlotIPCMessage message(SlotIPCMessage::SignalConnectionRequest, signalSignature, Q_ARG(QString, connectionId));
  QByteArray request = SlotIPCMarshaller::marshallMessage(message);

  return sendSynchronousRequest(request);
}


bool SlotIPCInterfacePrivate::sendRemoteDisconnectRequest(const QString& signalSignature)
{
  DEBUG << "Requesting remote signal disconnect" << signalSignature;

  QString connectionId = m_worker->connectionId();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QGenericArgument genArg = Q_ARG(QString, connectionId);
#else
  auto arg = Q_ARG(QString, connectionId);
  QGenericArgument genArg("QString", arg.data);
#endif

  SlotIPCMessage::Arguments args;
  args.push_back(genArg);
  SlotIPCMessage message(SlotIPCMessage::SignalConnectionRequest, signalSignature, args, "disconnect");
  QByteArray request = SlotIPCMarshaller::marshallMessage(message);

  return sendSynchronousRequest(request);
}


bool SlotIPCInterfacePrivate::sendSynchronousRequest(const QByteArray& request, QGenericReturnArgument returnedObject)
{
  Q_Q(SlotIPCInterface);

  if (!m_localServer.isEmpty())
  {
    QLocalSocket socket;
    socket.connectToServer(m_localServer);
    bool connected = socket.waitForConnected(5000);
    if (!connected)
    {
      socket.disconnectFromServer();
      QString error("SlotIPC: Could not connect to the server when the synchronous method was called");
      qWarning() << error;
      _q_setLastError(error);
      return false;
    }

    SlotIPCInterfaceConnection connection(&socket);
    QObject::connect(&connection, SIGNAL(errorOccured(QString)), q, SLOT(_q_setLastError(QString)));
    connection.setReturnedObject(returnedObject);

    QEventLoop loop;
    QObject::connect(&connection, SIGNAL(callFinished()), &loop, SLOT(quit()));

    QObject::connect(&connection, SIGNAL(socketDisconnected()), q_ptr, SIGNAL(disconnected()));
    QObject::connect(&connection, SIGNAL(socketDisconnected()), &loop, SLOT(quit()));
    connection.sendCallRequest(request);
    loop.exec();

    return connection.lastCallSuccessful();
  }
  else if (!m_tcpAddress.first.isNull())
  {
    QTcpSocket socket;
    socket.connectToHost(m_tcpAddress.first, m_tcpAddress.second);
    bool connected = socket.waitForConnected(5000);
    if (!connected)
    {
      socket.disconnectFromHost();
      QString error("SlotIPC: Could not connect to the server when the synchronous method was called");
      qWarning() << error;
      _q_setLastError(error);
      return false;
    }

    SlotIPCInterfaceConnection connection(&socket);
    QObject::connect(&connection, SIGNAL(errorOccured(QString)), q, SLOT(_q_setLastError(QString)));
    connection.setReturnedObject(returnedObject);

    QEventLoop loop;
    QObject::connect(&connection, SIGNAL(callFinished()), &loop, SLOT(quit()));
    QObject::connect(&connection, SIGNAL(socketDisconnected()), &loop, SLOT(quit()));
    connection.sendCallRequest(request);
    loop.exec();

    return connection.lastCallSuccessful();
  }

  return false;
}


void SlotIPCInterfacePrivate::_q_setLastError(QString lastError)
{
  this->m_lastError = lastError;
}


void SlotIPCInterfacePrivate::_q_invokeRemoteSignal(const QString& signalSignature, const SlotIPCMessage::Arguments& arguments)
{
  QList<MethodData> recieversData = m_connections.values(signalSignature);
  foreach (const MethodData& data, recieversData)
  {
    if (!data.first)
    {
      SlotIPCMarshaller::freeArguments(arguments);
      return;
    }

    DEBUG << "Invoke local method: " << data.second;

    QString methodName = data.second;
    methodName = methodName.left(methodName.indexOf("("));

    SlotIPCMessage::Arguments args = arguments;
    while (args.size() < 10)
      args.append(QGenericArgument());

    QMetaObject::invokeMethod(data.first, methodName.toLatin1(), Qt::QueuedConnection,
                              args.at(0), args.at(1), args.at(2), args.at(3), args.at(4), args.at(5), args.at(6),
                              args.at(7), args.at(8), args.at(9));
  }

  SlotIPCMarshaller::freeArguments(arguments);
}


void SlotIPCInterfacePrivate::handleLocalSignalRequest(QObject* localObject, const QString& signalSignature,
                                                       const QString& slotSignature)
{
  Q_Q(SlotIPCInterface);

  MethodData data(localObject, signalSignature);

  QList<SlotIPCSignalHandler*> handlers = m_localSignalHandlers.values(data);
  SlotIPCSignalHandler* handler = 0;
  foreach (SlotIPCSignalHandler* existingHandler, handlers)
  {
    if (existingHandler->signature() == slotSignature)
      handler = existingHandler;
  }

  if (!handler)
  {
    handler = new SlotIPCSignalHandler(slotSignature, q);
    handler->setSignalParametersInfo(localObject, signalSignature);

    m_localSignalHandlers.insert(data, handler);

    QMetaObject::connect(localObject,
        localObject->metaObject()->indexOfSignal("destroyed(QObject*)"),
        q, q->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("_q_removeSignalHandlersOfObject(QObject*)")));

    QMetaObject::connect(localObject,
        localObject->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(signalSignature.toLatin1())),
        handler, handler->metaObject()->indexOfSlot("relaySlot()"));

    QMetaObject::connect(
        handler, handler->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("signalCaptured(QByteArray)")),
        q, q->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("_q_sendAsynchronousRequest(QByteArray)")));
  }
}


void SlotIPCInterfacePrivate::_q_removeSignalHandlersOfObject(QObject* destroyedObject)
{
  auto it = m_localSignalHandlers.begin();
  while (it != m_localSignalHandlers.end())
  {
    MethodData data = it.key();
    if (data.first == destroyedObject)
      it = m_localSignalHandlers.erase(it);
    else
      ++it;
  }
}


void SlotIPCInterfacePrivate::_q_sendAsynchronousRequest(const QByteArray& request)
{
  QMetaObject::invokeMethod(m_worker, "sendCallRequest", Q_ARG(QByteArray, request));
}


void SlotIPCInterfacePrivate::registerConnection(const QString& signalSignature, QObject* reciever, const QString& methodSignature)
{
  Q_Q(SlotIPCInterface);
  m_connections.insert(signalSignature, MethodData(reciever, methodSignature));
  QObject::connect(reciever, SIGNAL(destroyed(QObject*)), q, SLOT(_q_removeRemoteConnectionsOfObject(QObject*)));
}


void SlotIPCInterfacePrivate::_q_removeRemoteConnectionsOfObject(QObject* destroyedObject)
{
  auto it = m_connections.begin();
  while (it != m_connections.end())
  {
    MethodData data = it.value();
    if (data.first == destroyedObject)
      it = m_connections.erase(it);
    else
      ++it;
  }
}

bool SlotIPCInterfacePrivate::call(const QString& method,
                                  const QGenericReturnArgument& ret,
                                  const SlotIPCMessage::Arguments& arguments)
{
    QString retType = ret.name() ? QString::fromLatin1(ret.name()) : QString();
    SlotIPCMessage message(SlotIPCMessage::MessageCallWithReturn, 
                          method,
                          arguments,
                          retType);
    
    // 序列化并发送请求
    QByteArray request = SlotIPCMarshaller::marshallMessage(message);
    DEBUG << "Remote call" << method;
    return sendSynchronousRequest(request, ret);
}


/*!
    Creates a new SlotIPCInterface object with the given \a parent.

    \sa connectToServer()
 */
SlotIPCInterface::SlotIPCInterface(QObject* parent)
  : QObject(parent),
    d_ptr(new SlotIPCInterfacePrivate())
{
  Q_D(SlotIPCInterface);
  d->q_ptr = this;

  connect(d->m_worker, SIGNAL(disconnected()), SIGNAL(disconnected()));
  connect(d->m_worker, SIGNAL(setLastError(QString)), SLOT(_q_setLastError(QString)));
  connect(d->m_worker, SIGNAL(invokeRemoteSignal(QString, SlotIPCMessage::Arguments)),
          SLOT(_q_invokeRemoteSignal(QString, SlotIPCMessage::Arguments)));

  qRegisterMetaType<QGenericReturnArgument>("QGenericReturnArgument");
  qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
  qRegisterMetaType<SlotIPCMessage::Arguments>("SlotIPCMessage::Arguments");
  qRegisterMetaType<QHostAddress>("QHostAddress");
}


SlotIPCInterface::SlotIPCInterface(SlotIPCInterfacePrivate& dd, QObject* parent)
  : QObject(parent),
    d_ptr(&dd)
{
  Q_D(SlotIPCInterface);
  d->q_ptr = this;

  connect(d->m_worker, SIGNAL(setLastError(QString)), SLOT(_q_setLastError(QString)));
  connect(d->m_worker, SIGNAL(invokeRemoteSignal(QString, SlotIPCMessage::Arguments)),
          SLOT(_q_invokeRemoteSignal(QString, SlotIPCMessage::Arguments)));

  qRegisterMetaType<QGenericReturnArgument>("QGenericReturnArgument");
  qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
  qRegisterMetaType<SlotIPCMessage::Arguments>("SlotIPCMessage::Arguments");
  qRegisterMetaType<QHostAddress>("QHostAddress");
}


bool SlotIPCInterface::isConnected() {
  Q_D(SlotIPCInterface);
  return d->m_worker->isConnected();
}

/*!
    Destroyes the object.
 */
SlotIPCInterface::~SlotIPCInterface()
{
  delete d_ptr;
}


/*!
  Attempts to make a connection to the local server with a given \a name.
  Returns true on success, otherwise false.
  \param name Local server name
  \sa SlotIPCInterface::connectToServer(const QHostAddress &host, quint16 port)
  \sa SlotIPCService::serverName()
 */
bool SlotIPCInterface::connectToServer(const QString& name)
{
  Q_D(SlotIPCInterface);
  bool isConnected;

  QEventLoop loop;
  connect(d->m_worker, SIGNAL(connectToServerFinished()), &loop, SLOT(quit()));
  QMetaObject::invokeMethod(d->m_worker, "connectToServer", Q_ARG(QString, name), Q_ARG(void*, &isConnected));
  loop.exec();

  d->m_localServer = name;
  if (isConnected)
      emit connected();

  return isConnected;
}

/*!
  Attempts to make a connection to the TCP server with a given \a host and \a port.
  Returns true on success, otherwise false.
  \param host Server host
  \param port Server tcp port
  \sa SlotIPCInterface::connectToServer(const QString& name)
  \sa SlotIPCService::serverName()
 */
bool SlotIPCInterface::connectToServer(const QHostAddress& host, quint16 port)
{
  Q_D(SlotIPCInterface);
  bool isConnected;

  QEventLoop loop;
  connect(d->m_worker, SIGNAL(connectToServerFinished()), &loop, SLOT(quit()));
  QMetaObject::invokeMethod(d->m_worker, "connectToTcpServer", Q_ARG(QHostAddress, host), Q_ARG(quint16, port), Q_ARG(void*, &isConnected));
  loop.exec();

  d->m_tcpAddress = qMakePair(host, port);
  if (isConnected)
      emit connected();

  return isConnected;
}


/*!
    Disconnects from server by closing the socket.
 */
void SlotIPCInterface::disconnectFromServer()
{
  Q_D(SlotIPCInterface);
  QEventLoop loop;
  connect(d->m_worker, SIGNAL(disconnectFromServerFinished()), &loop, SLOT(quit()));
  QMetaObject::invokeMethod(d->m_worker, "disconnectFromServer");
  loop.exec();
}


/*!
    The method is used to connect the remote signal (on the server-side) to the slot or signal
    of some local object.
    It returns true on success. False otherwise (the slot doesn't exist,
    or signatures are incompatible, or the server replies the error).

    After the connection being established, signals will be delivered asynchronously.

    \note It is recommended to use this method the same way as you call QObject::connect() method
    (by using SIGNAL() and SLOT() macros).
    \par
    For example, to connect the remote \a exampleSignal() signal to the \a exampleSlot() of some local \a object,
    you can type:
    \code remoteConnect(SIGNAL(exampleSignal()), object, SLOT(exampleSlot())); \endcode

    \note This method doesn't establish the connection to the server, you must use connectToServer() first.

    \sa remoteSlotConnect()
 */
bool SlotIPCInterface::remoteConnect(const char* signal, QObject* object, const char* method)
{
  Q_D(SlotIPCInterface);

  if (!object)
  {
    d->m_lastError = "Object doesn't exist";
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << object;
    return false;
  }

  QString signalSignature = QString::fromLatin1(signal);
  QString methodSignature = QString::fromLatin1(method);

  if (!d->checkConnectCorrection(signalSignature, methodSignature))
    return false;

  signalSignature = signalSignature.mid(1);
  methodSignature = methodSignature.mid(1);


  int methodIndex = -1;

  if (method[0] == '1')
    methodIndex = object->metaObject()->indexOfSlot(QMetaObject::normalizedSignature(methodSignature.toLatin1()));
  else if (method[0] == '2')
    methodIndex = object->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(methodSignature.toLatin1()));

  if (methodIndex == -1)
  {
    d->m_lastError = "Method (slot or signal) doesn't exist:" + methodSignature;
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << object;
    return false;
  }

  bool ok = true;
  if (!(d->m_connections.contains(signalSignature)))
    ok = d->sendRemoteConnectRequest(signalSignature);

  if (ok)
    d->registerConnection(signalSignature, object, methodSignature);
  return ok;
}


/*!
    The method is used to connect the signal of some local object (on the client-side) to the remote signal or slot
    of the server.

    It returns true on success. False otherwise (the local signal doesn't exist, or signatures are incompatible).

    After the connection being established, all signals will be delivered asynchronously.

    \note It is recommended to use this method the same way as you call QObject::connect() method
    (by using SIGNAL() and SLOT() macros).
    \par
    For example, to connect the exampleSignal() signal of some local \a object to the remote \a exampleSlot() slot,
    you can type:
    \code remoteSlotConnect(object, SIGNAL(exampleSignal()), SLOT(exampleSlot())); \endcode

    \warning The method doesn't check the existence of the remote signal/slot on the server-side.

    \sa remoteConnect(), call()
 */
bool SlotIPCInterface::remoteConnect(QObject* localObject, const char* localSignal, const char* remoteMethod)
{
  Q_D(SlotIPCInterface);

  if (!localObject)
  {
    d->m_lastError = "Object doesn't exist";
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << localObject;
    return false;
  }

  QString signalSignature = QString::fromLatin1(localSignal);
  QString remoteMethodSignature = QString::fromLatin1(remoteMethod);

  if (!d->checkConnectCorrection(signalSignature, remoteMethodSignature))
    return false;

  signalSignature = signalSignature.mid(1);
  QChar type = remoteMethodSignature[0];
  remoteMethodSignature = remoteMethodSignature.mid(1);

  int signalIndex = localObject->metaObject()->indexOfSignal(
      QMetaObject::normalizedSignature(signalSignature.toLatin1()));

  if (signalIndex == -1)
  {
    d->m_lastError = "Signal doesn't exist:" + signalSignature;
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << localObject;
    return false;
  }


  if (type == '1') // slot
  {
    if (!d->checkRemoteSlotExistance(remoteMethodSignature))
    {
      d->m_lastError = "Remote slot doesn't exist:" + remoteMethodSignature;
      return false;
    }
  }
  else if (type == '2') // signal
  {
    if (!d->sendRemoteConnectRequest(remoteMethodSignature))
    {
      d->m_lastError = "Remote signal doesn't exist:" + remoteMethodSignature;
      return false;
    }
  }

  d->handleLocalSignalRequest(localObject, signalSignature, remoteMethodSignature);
  return true;
}


/*!
    Disconnects remote signal of server
    from local slot in object receiver.
    Returns true if the connection is successfully broken;
    otherwise returns false.

    \sa remoteConnect
 */
bool SlotIPCInterface::disconnectSignal(const char* signal, QObject* object, const char* method)
{
  Q_D(SlotIPCInterface);

  if (!object)
  {
    d->m_lastError = "Object doesn't exist";
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << object;
    return false;
  }

  if (signal[0] != '2' || (method[0] != '1' && method[0] != '2'))
    return false;

  QString signalSignature = QString::fromLatin1(signal).mid(1);
  QString methodSignature = QString::fromLatin1(method).mid(1);

  d->m_connections.remove(signalSignature, QPair<QObject*, QString>(object, methodSignature));
  if (!d->m_connections.contains(signalSignature))
    d->sendRemoteDisconnectRequest(signalSignature);
  return true;
}


/*!
    Disconnects local signal from remote slot of server.
    Returns true if the connection is successfully broken;
    otherwise returns false.

    \sa remoteConnect
 */
bool SlotIPCInterface::disconnectRemoteMethod(QObject* localObject, const char* signal, const char* remoteMethod)
{
  Q_D(SlotIPCInterface);

  if (!localObject)
  {
    d->m_lastError = "Object doesn't exist";
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << localObject;
    return false;
  }

  if (signal[0] != '2' || (remoteMethod[0] != '1' && remoteMethod[0] != '2'))
    return false;

  QString signalSignature = QString::fromLatin1(signal).mid(1);
  QString methodSignature = QString::fromLatin1(remoteMethod).mid(1);
  SlotIPCInterfacePrivate::MethodData data(localObject, signalSignature);

  QList<SlotIPCSignalHandler*> handlers = d->m_localSignalHandlers.values(data);
  foreach (SlotIPCSignalHandler* handler, handlers)
  {
    if (handler->signature() == methodSignature)
    {
      delete handler;
      d->m_localSignalHandlers.remove(data, handler);
    }
  }
  return true;
}


/*!
    The method is used to connect the signal of some local object (on the client-side) to the remote slot
    of the server.

    It returns true on success. False otherwise (the local signal doesn't exist, or signatures are incompatible).

    After the connection being established, all signals will be delivered asynchronously.

    \note It is recommended to use this method the same way as you call QObject::connect() method
    (by using SIGNAL() and SLOT() macros).
    \par
    For example, to connect the exampleSignal() signal of some local \a object to the remote \a exampleSlot() slot,
    you can type:
    \code remoteSlotConnect(object, SIGNAL(exampleSignal()), SLOT(exampleSlot())); \endcode

    \warning The method doesn't check the existence of the remote slot on the server-side.

    \sa remoteConnect(), call()
 */
bool SlotIPCInterface::remoteSlotConnect(QObject* localObject, const char* signal, const char* remoteSlot)
{
  Q_D(SlotIPCInterface);

  if (!localObject)
  {
    d->m_lastError = "Object doesn't exist";
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << localObject;
    return false;
  }

  QString signalSignature = QString::fromLatin1(signal);
  QString slotSignature = QString::fromLatin1(remoteSlot);

  if (!d->checkConnectCorrection(signalSignature, slotSignature))
    return false;

  signalSignature = signalSignature.mid(1);
  slotSignature = slotSignature.mid(1);

  int signalIndex = localObject->metaObject()->indexOfSignal(
      QMetaObject::normalizedSignature(signalSignature.toLatin1()));

  if (signalIndex == -1)
  {
    d->m_lastError = "Signal doesn't exist:" + signalSignature;
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << localObject;
    return false;
  }

  if (!d->checkRemoteSlotExistance(slotSignature))
  {
    d->m_lastError = "Remote slot doesn't exist:" + slotSignature;
    return false;
  }

  d->handleLocalSignalRequest(localObject, signalSignature, slotSignature);
  return true;
}


/*!
    Disconnects local signal from remote slot of server.
    Returns true if the connection is successfully broken;
    otherwise returns false.

    \sa remoteSlotConnect
 */
bool SlotIPCInterface::disconnectSlot(QObject* localObject, const char* signal, const char* remoteSlot)
{
  Q_D(SlotIPCInterface);

  if (!localObject)
  {
    d->m_lastError = "Object doesn't exist";
    qWarning() << "SlotIPC:" << "Error: " + d->m_lastError + "; object:" << localObject;
    return false;
  }

  if (signal[0] != '2' || remoteSlot[0] != '1')
    return false;

  QString signalSignature = QString::fromLatin1(signal).mid(1);
  QString slotSignature = QString::fromLatin1(remoteSlot).mid(1);
  SlotIPCInterfacePrivate::MethodData data(localObject, signalSignature);

  QList<SlotIPCSignalHandler*> handlers = d->m_localSignalHandlers.values(data);
  foreach (SlotIPCSignalHandler* handler, handlers)
  {
    if (handler->signature() == slotSignature)
    {
      delete handler;
      d->m_localSignalHandlers.remove(data, handler);
    }
  }
  return true;
}


/*!
    Invokes the remote \a method (of the server). Returns true if the invokation was successful, false otherwise.
    The invokation is synchronous (which means that client will be waiting for the response in the event loop).
    See callNoReply() method for asynchronous invokation.

    The signature of this method is completely concurs with QMetaObject::invokeMethod() Qt method signature.
    Thus, you can use it the same way as you did it locally, with invokeMethod().

    The return value of the member function call is placed in \a ret.
    You can pass up to ten arguments (val0, val1, val2, val3, val4, val5, val6, val7, val8, and val9)
    to the member function.

    \note To set arguments, you must enclose them using Q_ARG and Q_RETURN_ARG macros.
    \note This method doesn't establish the connection to the server, you must use connectToServer() first.
    \sa callNoReply()
 */
bool SlotIPCInterface::callImpl(const QString& method, 
                               const QGenericReturnArgument& ret,
                               const QGenericArgument* args,
                               int argc)
{
    Q_D(SlotIPCInterface);
    
    // 构建参数列表
    SlotIPCMessage::Arguments arguments;
    for(int i = 0; i < argc; ++i) {
        if(args[i].name()) {
            arguments.push_back(args[i]);
        }
    }
    
    return d->call(method, ret, arguments);
}

bool SlotIPCInterface::call(const QString& method, 
                           METHOD_RE_ARG ret,
                           METHOD_ARG val0,
                           METHOD_ARG val1,
                           METHOD_ARG val2,
                           METHOD_ARG val3,
                           METHOD_ARG val4,
                           METHOD_ARG val5,
                           METHOD_ARG val6,
                           METHOD_ARG val7,
                           METHOD_ARG val8,
                           METHOD_ARG val9)
{
    const METHOD_ARG args[] = {
        val0, val1, val2, val3, val4, val5, val6, val7, val8, val9
    };

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5: 直接使用参数
    if (ret.name()) {
        // 如果有返回值，需要正确处理返回值类型
        QString retType = QString::fromLatin1(ret.name());
        return callImpl(method, ret, args, 10);
    } else {
        // 无返回值的情况
        return callImpl(method, QGenericReturnArgument(), args, 10);
    }
#else
    QGenericArgument aargs[10] = {};

    for(int i = 0; i < 10; ++i) {
        if(args[i].name) {
            aargs[i] = QGenericArgument(args[i].name, args[i].data);
        }
    }
    if (ret.name) {
        return callImpl(method, QGenericReturnArgument(ret.name, ret.data), aargs, 10);
    } else {
        // 无返回值的情况
        return callImpl(method, QGenericReturnArgument(), aargs, 10);
    }
#endif
}

bool SlotIPCInterface::call(const QString& method,
                           METHOD_ARG val0,
                           METHOD_ARG val1,
                           METHOD_ARG val2,
                           METHOD_ARG val3,
                           METHOD_ARG val4,
                           METHOD_ARG val5,
                           METHOD_ARG val6,
                           METHOD_ARG val7,
                           METHOD_ARG val8,
                           METHOD_ARG val9)
{
    return call(method, METHOD_RE_ARG(), 
               val0, val1, val2, val3, val4,
               val5, val6, val7, val8, val9);
}

/*!
    Invokes the remote \a method (of the server). Returns true if the invokation was successful, false otherwise.
    Unlike the process of call() method, the invokation is asynchronous
    (which means that the client will not waiting for the response).

    The signature of this method is completely concurs with QMetaObject::invokeMethod() Qt method signature
    (without return value).
    Thus, you can use it the same way as you did it locally, with invokeMethod().

    You can pass up to ten arguments (val0, val1, val2, val3, val4, val5, val6, val7, val8, and val9)
    to the member function.

    \note To set arguments, you must enclose them using Q_ARG macro.
    \note This method doesn't establish the connection to the server, you must use connectToServer() first.
    \sa call(), connectToServer()
 */
void SlotIPCInterface::callNoReply(const QString& method, 
                                  METHOD_ARG val0,
                                  METHOD_ARG val1,
                                  METHOD_ARG val2,
                                  METHOD_ARG val3,
                                  METHOD_ARG val4,
                                  METHOD_ARG val5,
                                  METHOD_ARG val6,
                                  METHOD_ARG val7,
                                  METHOD_ARG val8,
                                  METHOD_ARG val9)
{
    Q_D(SlotIPCInterface);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6: 需要转换参数类型
    SlotIPCMessage::Arguments arguments;
    arguments.reserve(10);
    
    const METHOD_ARG args[] = {
        val0, val1, val2, val3, val4, val5, val6, val7, val8, val9
    };
    
    for(int i = 0; i < 10; ++i) {
        if(args[i].name) {
            arguments.append(QGenericArgument(args[i].name, args[i].data));
        }
    }
    
    SlotIPCMessage message(SlotIPCMessage::MessageCallWithoutReturn, method, arguments);
#else
    // Qt5: 直接使用参数
    SlotIPCMessage message(SlotIPCMessage::MessageCallWithoutReturn, method, 
                          val0, val1, val2, val3, val4,
                          val5, val6, val7, val8, val9);
#endif

    QByteArray request = SlotIPCMarshaller::marshallMessage(message);
    DEBUG << "Remote call (asynchronous)" << method;
    d->_q_sendAsynchronousRequest(request);
}


/*!
    Returns the last occured error.
 */
QString SlotIPCInterface::lastError() const
{
  Q_D(const SlotIPCInterface);
  return d->m_lastError;
}

#include "moc_interface.cpp"

