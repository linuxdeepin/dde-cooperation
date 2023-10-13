// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WORKSPACEEVENTSENDER_H
#define WORKSPACEEVENTSENDER_H

#include <QObject>

namespace cooperation_workspace {

class WorkspaceEventSender : public QObject
{
    Q_OBJECT
public:
    static WorkspaceEventSender *instance();

private:
    explicit WorkspaceEventSender(QObject *parent = nullptr);
};

}   // namespace cooperation_workspace

#endif   // WORKSPACEEVENTSENDER_H
