// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "interfaceworker.h"
#include "interfaceconnection_p.h"
#include "message_p.h"
#include "marshaller_p.h"


#include <QEventLoop>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QHostAddress>


SlotIPCInterfaceWorker::SlotIPCInterfaceWorker(QObject* parent)
  : QObject(parent)
{}


SlotIPCInterfaceWorker::~SlotIPCInterfaceWorker()
{
  if (m_socket)
    delete m_socket;
}


void SlotIPCInterfaceWorker::connectToServer(const QString& name, void* successful)
{
  // TODO: Add checking for existing connection

  QLocalSocket* socket = new QLocalSocket;
  socket->connectToServer(name);

  bool connected = socket->waitForConnected(5000);
  if (!connected)
  {
    socket->disconnectFromServer();
    delete socket;
  }
  else
  {
    m_socket = socket;
    m_connection = new SlotIPCInterfaceConnection(socket, this);
    connect(m_connection, SIGNAL(invokeRemoteSignal(QString, SlotIPCMessage::Arguments)),
            this, SIGNAL(invokeRemoteSignal(QString, SlotIPCMessage::Arguments)));
    connect(m_connection, SIGNAL(errorOccured(QString)), this, SIGNAL(setLastError(QString)));

    connect(m_connection, SIGNAL(socketDisconnected()), SIGNAL(disconnected()));
    connect(m_connection, SIGNAL(socketDisconnected()), m_connection, SLOT(deleteLater()));
    connect(m_connection, SIGNAL(socketDisconnected()), socket, SLOT(deleteLater()));

    DEBUG << "SlotIPC:" << "Connected:" << name << connected;

    // Register connection ID on the serverside
    QString id = connectionId();
    SlotIPCMessage message(SlotIPCMessage::ConnectionInitialize, "", Q_ARG(QString, id));
    QByteArray request = SlotIPCMarshaller::marshallMessage(message);

    DEBUG << "Send connection ID to the server:" << id;

    QEventLoop loop;
    QObject::connect(m_connection, SIGNAL(callFinished()), &loop, SLOT(quit()));
    QObject::connect(m_connection, SIGNAL(socketDisconnected()), &loop, SLOT(quit()));
    m_connection->sendCallRequest(request);
    loop.exec();

    bool ok = m_connection->lastCallSuccessful();
    if (!ok)
      qWarning() << "SlotIPC:" << "Error: send connection ID failed. Remote signal connections will be unsuccessful";
  }

  *reinterpret_cast<bool*>(successful) = connected;
  emit connectToServerFinished();
}


void SlotIPCInterfaceWorker::connectToTcpServer(const QHostAddress& host, const quint16 port, void *successful)
{
  QTcpSocket* socket = new QTcpSocket;
  socket->connectToHost(host, port);
  bool connected = socket->waitForConnected(5000);
  if (!connected)
  {
    socket->disconnectFromHost();
    delete socket;
  }
  else
  {
    m_socket = socket;
    m_connection = new SlotIPCInterfaceConnection(socket, this);
    connect(m_connection, SIGNAL(invokeRemoteSignal(QString, SlotIPCMessage::Arguments)),
            this, SIGNAL(invokeRemoteSignal(QString, SlotIPCMessage::Arguments)));
    connect(m_connection, SIGNAL(errorOccured(QString)), this, SIGNAL(setLastError(QString)));

    connect(m_connection, SIGNAL(socketDisconnected()), SIGNAL(disconnected()));
    connect(m_connection, SIGNAL(socketDisconnected()), m_connection, SLOT(deleteLater()));
    connect(m_connection, SIGNAL(socketDisconnected()), socket, SLOT(deleteLater()));

    DEBUG << "SlotIPC:" << "Connected over network:" << host << port << connected;

    // Register connection ID on the serverside
    QString id = connectionId();
    SlotIPCMessage message(SlotIPCMessage::ConnectionInitialize, "", Q_ARG(QString, id));
    QByteArray request = SlotIPCMarshaller::marshallMessage(message);

    DEBUG << "Send connection ID to the server:" << id;

    QEventLoop loop;
    QObject::connect(m_connection, SIGNAL(callFinished()), &loop, SLOT(quit()));
    QObject::connect(m_connection, SIGNAL(socketDisconnected()), &loop, SLOT(quit()));
    m_connection->sendCallRequest(request);
    loop.exec();

    bool ok = m_connection->lastCallSuccessful();
    if (!ok)
      qWarning() << "SlotIPC:" << "Error: send connection ID failed. Remote signal connections will be unsuccessful";
  }

  *reinterpret_cast<bool*>(successful) = connected;
  emit connectToServerFinished();
}


void SlotIPCInterfaceWorker::disconnectFromServer()
{
  if (!m_socket)
    return;

  if (QTcpSocket* socket = qobject_cast<QTcpSocket*>(m_socket))
    socket->disconnectFromHost();
  else if (QLocalSocket* socket = qobject_cast<QLocalSocket*>(m_socket))
    socket->disconnectFromServer();

  delete m_socket;
  emit disconnectFromServerFinished();
}


bool SlotIPCInterfaceWorker::isConnected() {
  return m_connection->isConnected();
}

void SlotIPCInterfaceWorker::sendCallRequest(const QByteArray& request)
{
  if (!m_connection)
    return;

  m_connection->sendCallRequest(request);
}


QString SlotIPCInterfaceWorker::connectionId() const
{
  return QString::number(reinterpret_cast<quintptr>(m_connection.data()));
}
