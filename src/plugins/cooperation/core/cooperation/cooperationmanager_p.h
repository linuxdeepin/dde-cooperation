// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONMANAGER_P_H
#define COOPERATIONMANAGER_P_H

#include "info/deviceinfo.h"
#include "cooperationtaskdialog.h"

#include "ipc/bridge.h"

#include <QDBusInterface>

namespace cooperation_core {

class CooperationManager;
class CooperationManagerPrivate : public QObject
{
    Q_OBJECT
public:
    explicit CooperationManagerPrivate(CooperationManager *qq);

    void backendShareEvent(req_type_t type, const DeviceInfoPointer devInfo = nullptr, bool accepted = false);
    CooperationTaskDialog *taskDialog();
    void showCooperationResult(bool success, const QString &msg);
    uint notifyMessage(uint replacesId, const QString &body, const QStringList &actions, int expireTimeout);

public:
    void onActionTriggered(uint replacesId, const QString &action);

public:
    CooperationManager *q;
    QDBusInterface *notifyIfc { nullptr };
    CooperationTaskDialog *ctDialog { nullptr };
    bool isRecvMode { true };
    uint recvReplacesId { 0 };
};

}   // namespace cooperation_core

#endif   // COOPERATIONMANAGER_P_H
