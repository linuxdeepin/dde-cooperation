// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreeventsender.h"

#include <dde-cooperation-framework/dpf.h>

using namespace cooperation_core;

CooperationCoreEventSender::CooperationCoreEventSender(QObject *parent)
    : QObject(parent)
{
}

CooperationCoreEventSender *CooperationCoreEventSender::instance()
{
    static CooperationCoreEventSender ins;
    return &ins;
}

void CooperationCoreEventSender::sendRequestRefresh()
{
    dpfSignalDispatcher->publish("cooperation_core", "signal_Request_Refresh");
}
