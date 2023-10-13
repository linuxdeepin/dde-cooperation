// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workspaceeventsender.h"

using namespace cooperation_workspace;

WorkspaceEventSender::WorkspaceEventSender(QObject *parent)
    : QObject(parent)
{
}

WorkspaceEventSender *WorkspaceEventSender::instance()
{
    static WorkspaceEventSender ins;
    return &ins;
}
