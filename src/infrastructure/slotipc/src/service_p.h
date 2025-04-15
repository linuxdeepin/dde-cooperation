// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SERVICE_P_H
#define SERVICE_P_H


#include <QObject>
#include <QHash>
#include <QPointer>
class QLocalServer;
class QTcpServer;


#include "slotipc/service.h"
class SlotIPCSignalHandler;
class SlotIPCServiceConnection;


class SlotIPCServicePrivate
{
  Q_DECLARE_PUBLIC(SlotIPCService)

  public:
    SlotIPCServicePrivate();
    virtual ~SlotIPCServicePrivate();

    void registerLocalServer();
    void registerTcpServer();

    void _q_newLocalConnection();
    void _q_newTcpConnection();
    void _q_handleSignalRequest(const QString& signature, const QString& connectionId, QObject*);
    void _q_initializeConnection(QString, QObject*);
    void _q_connectionDestroyed(QObject*);
    void _q_handleSignalDisconnect(const QString& signature, const QString& connectionId, QObject*);
    void _q_removeSignalHandler(QString);

    QHash<QString, SlotIPCSignalHandler*> m_signalHandlers;
    QHash<QString, QObject*> m_longLivedConnections;
    QObject* m_subject;

    SlotIPCService* q_ptr;

  private:
    QPointer<QLocalServer> m_localServer;
    QPointer<QTcpServer> m_tcpServer;
};

#endif // SERVICE_P_H
