// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreeventreceiver.h"
#include "utils/cooperationutil.h"

using namespace cooperation_core;

CooperationCoreEventReceiver::CooperationCoreEventReceiver(QObject *parent)
    : QObject(parent)
{
}

CooperationCoreEventReceiver *CooperationCoreEventReceiver::instance()
{
    static CooperationCoreEventReceiver ins;
    return &ins;
}

void CooperationCoreEventReceiver::handleRegisterOperation(const QVariantMap &map)
{
    CooperationUtil::instance()->registerDeviceOperation(map);
}
