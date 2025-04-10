// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INTERFACECONNECTION_P_H
#define INTERFACECONNECTION_P_H


#include <QObject>
#include <QLocalSocket>
#include <QTcpSocket>


#include "slotipc/interface.h"
#include "message_p.h"


class SlotIPCInterfaceConnection : public QObject
{
  Q_OBJECT

  public:
    SlotIPCInterfaceConnection(QLocalSocket* socket, QObject* parent = 0);
    SlotIPCInterfaceConnection(QTcpSocket* socket, QObject* parent = 0);

    void sendCallRequest(const QByteArray& request);
    void setReturnedObject(QGenericReturnArgument returnedObject);
    bool lastCallSuccessful() const;
    bool isConnected();

  signals:
    void callFinished();
    void socketDisconnected();
    void invokeRemoteSignal(const QString& signalSignature, const SlotIPCMessage::Arguments& arguments);
    void errorOccured(const QString&);

  public slots:
    void readyRead();

    void errorOccured(QLocalSocket::LocalSocketError); // local socket error
    void errorOccured(QAbstractSocket::SocketError);   // network socket error

  private:
    QIODevice* m_socket;

    quint32 m_nextBlockSize;
    QByteArray m_block;

    bool m_lastCallSuccessful;
    QGenericReturnArgument m_returnedObject;

    bool readMessageFromSocket();
};

#endif // INTERFACECONNECTION_P_H
