// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INTERFACEWORKER_H
#define INTERFACEWORKER_H

#include <QObject>
#include <QPointer>
class QIODevice;
class QHostAddress;


#include "message_p.h"

class SlotIPCInterfaceConnection;
class SlotIPCLoopVector;


class SlotIPCInterfaceWorker : public QObject
{
  Q_OBJECT

  public:
    explicit SlotIPCInterfaceWorker(QObject* parent = 0);
    ~SlotIPCInterfaceWorker();

    bool isConnected();
  signals:
    void setLastError(const QString& error);
    void disconnected();

    // slot finish signals
    void connectToServerFinished();
    void sendConnectionIdFinished();
    void disconnectFromServerFinished();
    void invokeRemoteSignal(const QString& signalSignature, const SlotIPCMessage::Arguments& arguments);

  public slots:
    void connectToServer(const QString& name, void* successful);
    void connectToTcpServer(const QHostAddress& host, const quint16 port, void* successful);

    void disconnectFromServer();

    void sendCallRequest(const QByteArray& request);
    QString connectionId() const;

  private:
    void sendRemoteConnectionRequest(const QString& signal);
    void sendSignalDisconnectRequest(const QString& signal);

    QPointer<SlotIPCInterfaceConnection> m_connection;
    QPointer<QIODevice> m_socket;
};

#endif // INTERFACEWORKER_H
