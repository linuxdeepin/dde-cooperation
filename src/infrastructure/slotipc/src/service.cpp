// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "slotipc/service.h"
#include "service_p.h"
#include "marshaller_p.h"
#include "serviceconnection_p.h"
#include "signalhandler_p.h"


#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTime>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

#include <QTcpServer>
#include <QTcpSocket>

#define CLIENTS_DISCONNECT_TIMEOUT 100

/*!
    \class SlotIPCService

    \brief The SlotIPCService class provides an IPC server
    used to receive call requests and handle remote signals connections.

    Based on [QLocalServer](http://doc.qt.io/qt-5/qlocalserver.html)
    and [QTcpServer](http://doc.qt.io/qt-5/qtcpserver.html), it
    can be used locally or over TCP.
    To start local server identified by its name,
    use listen() method. Use listenTcp() method to start TCP server.

    You can specify a particular QObject to
    invoke its methods and slots and handle its signals, by specifying this object
    in the listen() or listenTcp() methods. In this case, the SlotIPCService
    object behaves only as an adaptor.
    Alternatively, you can inherit the SlotIPCService directly and invoke any method
    or slot of your subclassed object.

    You can connect multiple clients to the same CuteICPService object.

    \note It's possible (but not recommended) to listen on both local socket and
    TCP socket. More native way in this case will be using TCP connection with
    both remote and local connections from clients.

    \sa SlotIPCClient
*/


SlotIPCServicePrivate::SlotIPCServicePrivate()
  : m_subject(0)
{}


SlotIPCServicePrivate::~SlotIPCServicePrivate()
{
  // Inform long-lived connections about connection closing
  QList<QObject*> connections = m_longLivedConnections.values();
  foreach (QObject* connection, connections)
  {
    qobject_cast<SlotIPCServiceConnection*>(connection)->sendAboutToQuit();
  }

  // Clients reaction timeout
  if ( QCoreApplication::instance() ) {
      QEventLoop loop;
      QTimer timer;
      QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
      timer.start(CLIENTS_DISCONNECT_TIMEOUT);
      loop.exec();
  }
}


void SlotIPCServicePrivate::registerLocalServer()
{
  Q_Q(SlotIPCService);
  if (!m_localServer)
  {
    m_localServer = new QLocalServer(q);
    QObject::connect(m_localServer, SIGNAL(newConnection()), q, SLOT(_q_newLocalConnection()), Qt::UniqueConnection);
  }
}


void SlotIPCServicePrivate::registerTcpServer()
{
  Q_Q(SlotIPCService);
  if (!m_tcpServer)
  {
    m_tcpServer = new QTcpServer(q);
    QObject::connect(m_tcpServer, SIGNAL(newConnection()), q, SLOT(_q_newTcpConnection()), Qt::UniqueConnection);
  }
}


void SlotIPCServicePrivate::_q_newLocalConnection()
{
  Q_Q(SlotIPCService);
  QLocalSocket* socket = m_localServer->nextPendingConnection();
  Q_ASSERT(socket != 0);

  SlotIPCServiceConnection* connection = new SlotIPCServiceConnection(socket, q);
  connection->setSubject(m_subject);
}


void SlotIPCServicePrivate::_q_newTcpConnection()
{
  Q_Q(SlotIPCService);
  QTcpSocket* socket = m_tcpServer->nextPendingConnection();
  Q_ASSERT(socket != 0);

  SlotIPCServiceConnection* connection = new SlotIPCServiceConnection(socket, q);
  connection->setSubject(m_subject);
}


void SlotIPCServicePrivate::_q_handleSignalRequest(const QString& signalSignature, const QString& connectionId, QObject* sender)
{
  Q_Q(SlotIPCService);
  SlotIPCServiceConnection* senderConnection = qobject_cast<SlotIPCServiceConnection*>(sender);

  QObject* subject = m_subject ? m_subject : q;
  int signalIndex = subject->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(signalSignature.toLatin1()));

  if (signalIndex == -1)
  {
    senderConnection->sendErrorMessage("Signal doesn't exist:" + signalSignature);
    return;
  }

  if (!m_longLivedConnections.contains(connectionId))
  {
    senderConnection->sendErrorMessage("Connection ID is not registered: " + connectionId);
    return;
  }

  SlotIPCSignalHandler* handler = m_signalHandlers.value(signalSignature);
  if (!handler)
  {
    //create a new signal handler
    handler = new SlotIPCSignalHandler(signalSignature, q);
    handler->setSignalParametersInfo(subject,signalSignature);
    m_signalHandlers.insert(signalSignature, handler);
    QMetaObject::connect(subject, subject->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(signalSignature.toLatin1())),
                         handler, handler->metaObject()->indexOfSlot("relaySlot()"));
  }

//  handler->addListener(senderConnection);
  handler->addListener(qobject_cast<SlotIPCServiceConnection*>(m_longLivedConnections.value(connectionId)));
  senderConnection->sendResponseMessage(signalSignature);
}


void SlotIPCServicePrivate::_q_handleSignalDisconnect(const QString& signalSignature, const QString& connectionId, QObject* sender)
{
  SlotIPCServiceConnection* senderConnection = qobject_cast<SlotIPCServiceConnection*>(sender);

  SlotIPCSignalHandler* handler = m_signalHandlers.value(signalSignature);
  if (handler)
    handler->removeListener(qobject_cast<SlotIPCServiceConnection*>(m_longLivedConnections.value(connectionId)));

  senderConnection->sendResponseMessage(signalSignature);
}


void SlotIPCServicePrivate::_q_removeSignalHandler(QString key)
{
  m_signalHandlers.remove(key);
}


void SlotIPCServicePrivate::_q_initializeConnection(QString connectionId, QObject* sender)
{
  SlotIPCServiceConnection* senderConnection = qobject_cast<SlotIPCServiceConnection*>(sender);
  m_longLivedConnections.insert(connectionId, sender);
  senderConnection->sendResponseMessage("connectionInitialize_" + connectionId);
}


void SlotIPCServicePrivate::_q_connectionDestroyed(QObject* destroyedObject)
{
  QMutableHashIterator<QString, QObject*> i(m_longLivedConnections);
  while (i.hasNext())
  {
    i.next();
    if (i.value() == destroyedObject)
      i.remove();
  }
}


/*!
    Creates a new SlotIPCServer with the given \a parent.

    \sa listen()
 */
SlotIPCService::SlotIPCService(QObject* parent)
    : QObject(parent),
      d_ptr(new SlotIPCServicePrivate())
{
  Q_D(SlotIPCService);
  d->q_ptr = this;
}


SlotIPCService::SlotIPCService(SlotIPCServicePrivate& dd, QObject* parent)
    : QObject(parent),
      d_ptr(&dd)
{
  Q_D(SlotIPCService);
  d->q_ptr = this;
}


/*!
    Destroys the object.
 */
SlotIPCService::~SlotIPCService()
{
  delete d_ptr;
}


/*!
    Tells the server to listen for incoming TCP connections.
    The server is identified by a TCP \a port.
    Internally, the [QTcpServer](http://doc.qt.io/qt-5/qtcpserver.html) is used in this case,
    and parameters are comply with QTcpServer::listen method.

    \param address host address. It could be any of QHostAddress::​SpecialAddress, e.g. QHostAddress::Any.
    \param port TCP port.
    \param subject Sets the object which methods will be invoked. By default, it is SlotIPCService
    object itself. Previous subject will be replaced.
    Returns true on success, otherwise false.
 */
bool SlotIPCService::listenTcp(const QHostAddress& address, quint16 port, QObject* subject)
{
  Q_D(SlotIPCService);

  DEBUG << "Trying to listen" << address << "on port" << port;
  d->registerTcpServer();
  bool ok = d->m_tcpServer->listen(address, port);

  DEBUG << "SlotIPC:" << "Opened" << address << port << ok;
  d->m_subject = subject;
  return ok;
}


/*!
 * This is an overloaded member function.
 */
bool SlotIPCService::listenTcp(QObject* subject)
{
  return listenTcp(QHostAddress::Any, 0, subject);
}


/*!
    Returns the server TCP port if the server is listening for TCP connections;
    otherwise returns -1;
    \sa listenTcp()
 */
quint16 SlotIPCService::tcpPort() const
{
  Q_D(const SlotIPCService);
  if (!d->m_tcpServer)
  {
    qWarning() << "SlotIPC: trying to get TCP port without listening being established";
    return -1;
  }

  return d->m_tcpServer->serverPort();
}


/*!
    Returns the server host name if the server is listening for TCP connections;
    otherwise returns QHostAddress::Null.
    \sa listenTcp()
 */
QHostAddress SlotIPCService::tcpAddress() const
{
  Q_D(const SlotIPCService);
  if (!d->m_tcpServer)
  {
    qWarning() << "SlotIPC: trying to get TCP address without listening being established";
    return QHostAddress::Null;
  }

  return d->m_tcpServer->serverAddress();
}


/*!
    Tells the server to listen for incoming local connections.
    The server is identified by a \a serverName.
    Internally, the [QLocalServer](http://doc.qt.io/qt-5/qlocalserver.html) is used in this case.

    \param serverName Server name. If it's empty,
    the name will be generated based on class name and can be accessed by serverName() method.
    \param subject Sets the object which methods will be invoked. By default, it is SlotIPCService
    object itself. Previous subject will be replaced.
    Returns true on success, otherwise false.
 */
bool SlotIPCService::listen(const QString& serverName, QObject* subject, QLocalServer::SocketOptions options)
{
  Q_D(SlotIPCService);
  QString name = serverName;
  if (name.isEmpty())
    name = QString(QLatin1String("%1.%2")).arg(metaObject()->className()).arg(reinterpret_cast<quintptr>(this));

  DEBUG << "Trying to listen" << name;
  d->registerLocalServer();
  d->m_localServer->setSocketOptions(options);
  bool ok = d->m_localServer->listen(name);

  if (!ok)
  {
    DEBUG << "Trying to reuse existing pipe";
    ok = d->m_localServer->removeServer(name);
    if (ok)
    {
      DEBUG << "Server removed, connecting again";
      ok = d->m_localServer->listen(name);
    }
  }

  DEBUG << "SlotIPC:" << "Opened" << name << ok;
  d->m_subject = subject;

  return ok;
}


/*!
 * This is an overloaded member function.
 */
bool SlotIPCService::listen(QObject* subject, QLocalServer::SocketOptions options)
{
  return listen(QString(), subject, options);
}


/*!
    Closes the server (listening for both TCP and local connections).
    Internally, QLocalServer::close() and QTcpServer::close() are called.
 */
void SlotIPCService::close()
{
  Q_D(SlotIPCService);
  if (d->m_localServer)
  {
    d->m_localServer->removeServer(d->m_localServer->serverName());
    d->m_localServer->close();
  }
  else if (d->m_tcpServer)
    d->m_tcpServer->close();
}


/*!
    Returns the server name if the server is listening for local connections;
    otherwise returns QString().

    \sa listen()
 */
QString SlotIPCService::serverName() const
{
  Q_D(const SlotIPCService);
  if (!d->m_localServer)
  {
    qWarning() << "SlotIPC: trying to get local server name without listening being established";
    return QString();
  }

  return d->m_localServer->serverName();
}

/*! \mainpage SlotIPC
 *
 * \section intro_sec Intro
 *
 * The SlotIPC library adds a facility to use Qt signals and slots across
 * local processes and over TCP.
 * SlotIPCService and SlotIPCInterface library classes are based on
 * standard Qt sockets and provide cross platform IPC solution with native syntax for Qt programs.
 *
 * \section usage_sec Usage
 *
 * #### Connection
 * First, create a server and associate it with a desired \a QObject:
 *
 * \code{.cpp}
 * // local server identified by name
 * SlotIPCService* service = new SlotIPCService;
 * service->listen("serverName", myObject);
 * \endcode
 * \code{.cpp}
 * // tcp server identified by host and port
 * SlotIPCService* service = new SlotIPCService;
 * service->listenTcp(QHostAddress::Any, 31337, myObject);
 * \endcode
 * The signals and slots of the object myObject will be called in IPC.
 *
 * On the other side, create the interface and connect to the server:
 * \code{.cpp}
 * // connect locally
 * SlotIPCInterface* interface;
 * interface = new SlotIPCInterface;
 * interface->connectToServer("myServerName");
 * \endcode
 * \code{.cpp}
 * // connect over TCP
 * interface->connectToServer(QHostAddress("192.0.2.1"), 31337);
 * \endcode
 *
 * #### Direct calls
 * You can to directly invoke any [invokable](http://doc.qt.io/qt-5/qobject.html#Q_INVOKABLE) method or public slot of the corresponding object.
 * The syntax is concurs with [QMetaObject::invokeMethod](http://doc.qt.io/qt-5/qmetaobject.html#invokeMethod).
 *
 * Use SlotIPCInterface::call() method to invoke synchronously and wait for result in the event loop:
 * \code{.cpp}
 * int result;
 * interface->call("remoteMethod", Q_RETURN_ARG(int, result), Q_ARG(QString, methodParameter));
 * \endcode
 *
 * Use SlotIPCInterface::callNoReply() to send invoke request asynchronously and return immediately:
 * \code{.cpp}
 * interface->callNoReply("remoteMethod", Q_ARG(QString, methodParameter));
 * \endcode
 *
 * #### Signals and slots
 * You can natively connect the remote signal to a local slot:
 * \code{.cpp}
 * interface->remoteConnect(SIGNAL(remoteSignal(QString)), receiver, SLOT(receiverSlot(QString)));
 * \endcode
 *
 * or local signal to the remote slot:
 * \code{.cpp}
 * interface->remoteSlotConnect(senderObject, SIGNAL(localSignal(QString)), SLOT(remoteSlot(QString)));
 * \endcode
 *
 * \note To enable debug output, set SLOTIPC_DEBUG environment variable to 1;
 *
 */
#include "moc_service.cpp"
