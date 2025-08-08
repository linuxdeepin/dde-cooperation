// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "interfaceconnection_p.h"
#include "marshaller_p.h"
#include "message_p.h"


#include <QLocalSocket>
#include <QDataStream>
#include <QTime>
#include <QMetaType>


SlotIPCInterfaceConnection::SlotIPCInterfaceConnection(QLocalSocket* socket, QObject* parent)
  : QObject(parent),
    m_socket(socket),
    m_nextBlockSize(0),
    m_lastCallSuccessful(false)
{
  connect(socket, SIGNAL(disconnected()), SIGNAL(socketDisconnected()));
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  connect(socket, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)), SLOT(errorOccured(QLocalSocket::LocalSocketError)));
#else
  connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), SLOT(errorOccured(QLocalSocket::LocalSocketError)));
#endif
  connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
}


SlotIPCInterfaceConnection::SlotIPCInterfaceConnection(QTcpSocket* socket, QObject* parent)
  : QObject(parent),
    m_socket(socket),
    m_nextBlockSize(0),
    m_lastCallSuccessful(false)
{
  connect(socket, SIGNAL(disconnected()), SIGNAL(socketDisconnected()));
  // connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(errorOccured(QAbstractSocket::SocketError)));
  connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
}

bool SlotIPCInterfaceConnection::isConnected() {
  return m_socket && m_socket->isOpen();
}

void SlotIPCInterfaceConnection::sendCallRequest(const QByteArray& request)
{
  QDataStream stream(m_socket);
  stream << (quint32)request.size();
  int written = stream.writeRawData(request.constData(), request.size());

  if (written != request.size())
    qWarning() << "SlotIPC:" << "Warning:" << "Written bytes and request size doesn't match";

  if (QAbstractSocket* socket = qobject_cast<QAbstractSocket*>(m_socket))
    socket->flush();
  else if (QLocalSocket* socket = qobject_cast<QLocalSocket*>(m_socket))
    socket->flush();

  m_lastCallSuccessful = true;
}


void SlotIPCInterfaceConnection::readyRead()
{
  bool messageStreamFinished;

  do
  {
    messageStreamFinished = readMessageFromSocket();
  } while (!messageStreamFinished);
}


bool SlotIPCInterfaceConnection::readMessageFromSocket()
{
  QDataStream in(m_socket);

  bool callWasFinished = false;

  // Fetch next block size
  if (m_nextBlockSize == 0)
  {
    if (m_socket->bytesAvailable() < (int)sizeof(quint32))
      return true;

    in >> m_nextBlockSize;
  }

  if (in.atEnd())
    return true;

  qint64 bytesToFetch = m_nextBlockSize - m_block.size();
  m_block.append(m_socket->read(bytesToFetch));

  if (m_block.size() == (int)m_nextBlockSize)
  {
    // Fetched enough, need to parse
    SlotIPCMessage::MessageType type = SlotIPCMarshaller::demarshallMessageType(m_block);

    switch (type)
    {
      case SlotIPCMessage::MessageResponse:
      {
        SlotIPCMessage message = SlotIPCMarshaller::demarshallResponse(m_block, m_returnedObject);
        callWasFinished = true;
        SlotIPCMarshaller::freeArguments(message.arguments());
        break;
      }
      case SlotIPCMessage::MessageError:
      {
        m_lastCallSuccessful = false;
        callWasFinished = true;
        SlotIPCMessage message = SlotIPCMarshaller::demarshallMessage(m_block);
        qWarning() << "SlotIPC:" << "Error:" << message.method();
        emit errorOccured(message.method());
        SlotIPCMarshaller::freeArguments(message.arguments());
        break;
      }
      case SlotIPCMessage::AboutToCloseSocket:
      {
        DEBUG << "The server reports that the connection is closed";
        SlotIPCMessage message = SlotIPCMarshaller::demarshallMessage(m_block);
        SlotIPCMarshaller::freeArguments(message.arguments());
        m_lastCallSuccessful = false;

        // Разрываем соединение с сервером. При этом отправляется сигнал socketDiconnected
        if (QTcpSocket* socket = qobject_cast<QTcpSocket*>(m_socket))
          socket->disconnectFromHost();
        else if (QLocalSocket* socket = qobject_cast<QLocalSocket*>(m_socket))
          socket->disconnectFromServer();

        break;
      }
      case SlotIPCMessage::MessageSignal:
      {
        SlotIPCMessage message = SlotIPCMarshaller::demarshallMessage(m_block);
        emit invokeRemoteSignal(message.method(), message.arguments());
        break;
      }
      default:
      {
        break;
        callWasFinished = true;
      }
    }

    m_nextBlockSize = 0;
    m_block.clear();

    if (callWasFinished)
      emit callFinished();

    if (m_socket->bytesAvailable())
      return false;
  }

  return true;
}


void SlotIPCInterfaceConnection::errorOccured(QLocalSocket::LocalSocketError)
{
  qWarning() << "SlotIPC" << "Socket error: " << m_socket->errorString();
  emit errorOccured(m_socket->errorString());
}


void SlotIPCInterfaceConnection::errorOccured(QAbstractSocket::SocketError)
{
  qWarning() << "SlotIPC" << "Socket error: " << m_socket->errorString();
  emit errorOccured(m_socket->errorString());
}


void SlotIPCInterfaceConnection::setReturnedObject(QGenericReturnArgument returnedObject)
{
  m_returnedObject = returnedObject;
}


bool SlotIPCInterfaceConnection::lastCallSuccessful() const
{
  return m_lastCallSuccessful;
}
