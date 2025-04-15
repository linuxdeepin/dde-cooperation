// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIGNALHANDLER_P_H
#define SIGNALHANDLER_P_H


#include <QObject>
class QMetaMethod;


class SlotIPCService;
class SlotIPCServiceConnection;


class SlotIPCSignalHandler : public QObject
{
  Q_OBJECT_FAKE

  public:
    explicit SlotIPCSignalHandler(const QString& signature, QObject* parent = 0);
    ~SlotIPCSignalHandler();

    //To send signals from client-side local objects, pass server's slot signature to constructor
    //and object's signal to setSignalParametersInfo.
    //Use this method to set server's signal owner also (with the same signature as passed in constructor)
    void setSignalParametersInfo(QObject* owner, const QString& signature);
    QString signature() const;

//  public slots:
    void relaySlot(void**);
    void addListener(SlotIPCServiceConnection* listener);
    void removeListener(SlotIPCServiceConnection* listener);
    void listenerDestroyed(QObject* listener);

  protected:
//  signals:
    void signalCaptured(const QByteArray& data);
    void destroyed(QString signature);

  private:
    QString m_signature;
    QList<QByteArray> m_signalParametersInfo;
    bool m_signalParametersInfoWasSet;
    QList<SlotIPCServiceConnection*> m_listeners;
};

#endif // SIGNALHANDLER_P_H
