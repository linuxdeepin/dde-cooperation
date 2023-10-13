// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WORKSPACEEVENTRECEIVER_H
#define WORKSPACEEVENTRECEIVER_H

#include <QObject>

namespace cooperation_workspace {

class WorkspaceEventReceiver : public QObject
{
    Q_OBJECT

public:
    static WorkspaceEventReceiver *instance();

    void handleRequestRefresh();
    void handleRegisterOperation(const QVariantMap &map);

private:
    explicit WorkspaceEventReceiver(QObject *parent = nullptr);
};

}   // namespace cooperation_workspace

#endif   // WORKSPACEEVENTRECEIVER_H
