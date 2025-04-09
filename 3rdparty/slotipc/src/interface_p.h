// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INTERFACE_P_H
#define INTERFACE_P_H


#include <QObject>
#include <QMultiHash>
#include <QPointer>
#include <QHostAddress>
class QLocalSocket;


#include "slotipc/interface.h"
#include "message_p.h"
class SlotIPCInterfaceConnection;
class SlotIPCSignalHandler;
class SlotIPCInterfaceWorker;
class SlotIPCLoopVector;


class SlotIPCInterfacePrivate
{
  Q_DECLARE_PUBLIC(SlotIPCInterface)

  typedef QPair<QObject*,QString> MethodData;

  public:
    SlotIPCInterfacePrivate();
    virtual ~SlotIPCInterfacePrivate();

    bool checkConnectCorrection(const QString& signal, const QString& method);
    bool checkRemoteSlotExistance(const QString& slot);
    bool sendRemoteConnectRequest(const QString& signalSignature);
    bool sendRemoteDisconnectRequest(const QString& signalSignature);
    bool sendSynchronousRequest(const QByteArray& request, QGenericReturnArgument returnedObject = QGenericReturnArgument());

    void handleLocalSignalRequest(QObject* localObject, const QString& signalSignature, const QString& slotSignature);
    void registerConnection(const QString& signalSignature, QObject* reciever, const QString& methodSignature);

    void _q_sendAsynchronousRequest(const QByteArray& request);
    void _q_removeSignalHandlersOfObject(QObject*);
    void _q_setLastError(QString); //TODO: !!!!!!
    void _q_invokeRemoteSignal(const QString& signalSignature, const SlotIPCMessage::Arguments& arguments);
    void _q_removeRemoteConnectionsOfObject(QObject* destroyedObject);

    bool call(const QString& method, 
              const QGenericReturnArgument& ret,
              const SlotIPCMessage::Arguments& arguments);

    SlotIPCInterface* q_ptr;
    QMultiHash<MethodData, SlotIPCSignalHandler*> m_localSignalHandlers;
    QString m_lastError;

    QThread* m_workerThread;
    SlotIPCInterfaceWorker* m_worker;

    QString m_localServer;
    QPair<QHostAddress, quint16> m_tcpAddress;

    QMultiHash<QString, MethodData> m_connections;
};

#endif //INTERFACE_P_H
