// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "AndroidMachineDBusAdaptor.h"

#include "AndroidMachine.h"

AndroidMachineDBusAdaptor::AndroidMachineDBusAdaptor(AndroidMachine *machine,
                                                     QDBusConnection bus,
                                                     const QString &path)
    : QDBusAbstractAdaptor(machine)
    , m_machine(machine)
    , m_path(path)
    , m_bus(bus) {
}

AndroidMachineDBusAdaptor::~AndroidMachineDBusAdaptor() {
}

void AndroidMachineDBusAdaptor::StartCast(const QDBusMessage &message) {
    if (!m_machine->connected()) {
        auto reply = message.createErrorReply(QDBusError::Failed, "connect first");
        QDBusConnection::sessionBus().send(reply);
        return;
    }

    m_machine->startCast();
}
