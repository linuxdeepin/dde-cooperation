// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONMANAGER_P_H
#define COOPERATIONMANAGER_P_H

#include "discover/deviceinfo.h"
#include "dialogs/cooperationtaskdialog.h"

#include "net/cooconstrants.h"

#ifdef linux
#include <QDBusInterface>
#endif
#include <QTimer>

namespace cooperation_core {

class ShareHelper;
class ShareHelperPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ShareHelperPrivate(ShareHelper *qq);

    CooperationTaskDialog *taskDialog();
    uint notifyMessage(uint replacesId, const QString &body, const QStringList &actions, int expireTimeout);
    void reportConnectionData();

public Q_SLOTS:
    void onActionTriggered(uint replacesId, const QString &action);
    void stopCooperation();
    void onCancelCooperApply();

public:
    ShareHelper *q;
#ifdef linux
    QDBusInterface *notifyIfc { nullptr };
#endif
    CooperationTaskDialog *ctDialog { nullptr };
    uint recvReplacesId { 0 };
    bool isRecvMode { true };
    bool isReplied { false };
    bool isTimeout { false };
    QTimer confirmTimer;

    // 作为接收方时，发送方的ip
    QString senderDeviceIp;
    // 作为发送方时，为接收方设备信息；作为接收方时，为发送方设备信息
    DeviceInfoPointer targetDeviceInfo { nullptr };
    QString targetDevName;
};

}   // namespace cooperation_core

#endif   // COOPERATIONMANAGER_P_H
