// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICECONNECTION_P_H
#define SERVICECONNECTION_P_H


#include <QObject>
#include <QLocalSocket>
class QTcpSocket;


#include "slotipc/service.h"


class SlotIPCServiceConnection : public QObject
{
  Q_OBJECT

  public:
    SlotIPCServiceConnection(QLocalSocket* socket, SlotIPCService* parent);
    SlotIPCServiceConnection(QTcpSocket* socket, SlotIPCService* parent);

    ~SlotIPCServiceConnection();
    void setSubject(QObject* subject);

  signals:
    void signalRequest(QString signalSignature, const QString& connectionId, QObject* sender);
    void signalDisconnectRequest(QString signalSignature, const QString& connectionId, QObject* sender);
    void connectionInitializeRequest(const QString& connectionId, QObject* sender);

  public slots:
    void readyRead();

    void errorOccured(QLocalSocket::LocalSocketError); // local socket error
    void errorOccured(QAbstractSocket::SocketError);   // network socket error

    void sendSignal(const QByteArray& data);

    void sendErrorMessage(const QString& error);
    void sendResponseMessage(const QString& method, QGenericArgument arg = QGenericArgument());
    void sendAboutToQuit();

  private:
    QIODevice* m_socket;

    quint32 m_nextBlockSize;
    QByteArray m_block;
    QObject* m_subject;

    void processMessage();
    bool readMessageFromSocket();

    void sendResponse(const QByteArray& response);
};


#endif // SERVICECONNECTION_P_H
