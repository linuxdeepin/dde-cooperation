// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QtNetwork/QHostAddress>
#include <QLocalServer>


class SlotIPCServicePrivate;

class Q_DECL_EXPORT SlotIPCService : public QObject
{
  Q_OBJECT

  public:
    explicit SlotIPCService(QObject* parent = 0);
    ~SlotIPCService();

    bool listen(const QString& name = QString(), QObject* subject = 0, QLocalServer::SocketOptions options=QLocalServer::NoOptions);
    bool listen(QObject* subject, QLocalServer::SocketOptions options=QLocalServer::NoOptions);
    QString serverName() const;

    bool listenTcp(const QHostAddress& address = QHostAddress::Any, quint16 port = 0, QObject* subject = 0);
    bool listenTcp(QObject* subject);
    QHostAddress tcpAddress() const;
    quint16 tcpPort() const;

    void close();

  protected:
    SlotIPCServicePrivate* const d_ptr;
    SlotIPCService(SlotIPCServicePrivate& dd, QObject* parent);

  private:
    Q_DECLARE_PRIVATE(SlotIPCService)

    Q_PRIVATE_SLOT(d_func(),void _q_newLocalConnection())
    Q_PRIVATE_SLOT(d_func(),void _q_newTcpConnection())
    Q_PRIVATE_SLOT(d_func(),void _q_handleSignalRequest(QString,QString,QObject*))
    Q_PRIVATE_SLOT(d_func(),void _q_handleSignalDisconnect(QString,QString,QObject*))
    Q_PRIVATE_SLOT(d_func(),void _q_removeSignalHandler(QString))
    Q_PRIVATE_SLOT(d_func(),void _q_initializeConnection(QString,QObject*))
    Q_PRIVATE_SLOT(d_func(),void _q_connectionDestroyed(QObject*))
};

#endif // SERVICE_H
