// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MachineDBusAdaptor.h"

#include "Manager.h"
#include "Machine.h"

static const QString machineInterface{"org.deepin.dde.Cooperation1.Machine"};

MachineDBusAdaptor::MachineDBusAdaptor(Manager *manager,
                                       Machine *machine,
                                       QDBusConnection bus,
                                       const QString &path)
    : QDBusAbstractAdaptor(machine)
    , m_path(path)
    , m_manager(manager)
    , m_machine(machine)
    , m_bus(bus) {
}

MachineDBusAdaptor::~MachineDBusAdaptor() {
    m_bus.unregisterObject(m_path);
}

QString MachineDBusAdaptor::getUUID() const {
    return QString::fromStdString(m_machine->m_uuid);
}

QString MachineDBusAdaptor::getName() const {
    return QString::fromStdString(m_machine->m_name);
}

QString MachineDBusAdaptor::getIP() const {
    return QString::fromStdString(m_machine->ip());
}

quint16 MachineDBusAdaptor::getPort() const {
    return m_machine->m_port;
}

quint32 MachineDBusAdaptor::getOS() const {
    return m_machine->m_os;
}

quint32 MachineDBusAdaptor::getCompositor() const {
    return m_machine->m_compositor;
}

bool MachineDBusAdaptor::getConnected() const {
    return m_machine->m_connected;
}

bool MachineDBusAdaptor::getDeviceSharing() const {
    return m_machine->m_deviceSharing;
}

quint16 MachineDBusAdaptor::getDirection() const {
    return m_machine->m_direction;
}

bool MachineDBusAdaptor::getSharedClipboard() const {
    return m_machine->m_sharedClipboard;
}

void MachineDBusAdaptor::Connect(const QDBusMessage &message) const {
    if (m_machine->connected()) {
        return;
    }

    if ((m_machine->isPcMachine() && m_machine->manager()->hasPcMachinePaired()) ||
        (m_machine->isAndroid() && m_machine->manager()->hasAndroidPaired())) {
        // TODO tips
        m_bus.send(message.createErrorReply(
            {QDBusError::Failed,
             QStringLiteral("This machine is cooperating with another machine!")}));
        return;
    }

    m_machine->connect();
}

void MachineDBusAdaptor::Disconnect() const {
    if (!m_machine->connected()) {
        return;
    }

    m_machine->disconnect();
}

void MachineDBusAdaptor::RequestDeviceSharing(const QDBusMessage &message) const {
    if (m_machine->m_deviceSharing) {
        m_bus.send(message.createErrorReply(
            {QDBusError::Failed, QStringLiteral("already sharing devices")}));
        return;
    }

    if (!m_machine->connected()) {
        m_bus.send(
            message.createErrorReply({QDBusError::AccessDenied, QStringLiteral("connect first")}));
        return;
    }

    m_machine->requestDeviceSharing();
}

void MachineDBusAdaptor::StopDeviceSharing(const QDBusMessage &message) const {
    if (!m_machine->m_deviceSharing) {
        m_bus.send(message.createErrorReply({QDBusError::Failed, QStringLiteral("")}));
        return;
    }

    m_machine->stopDeviceSharing();
}

void MachineDBusAdaptor::SetFlowDirection(quint16 direction, const QDBusMessage &message) const {
    if (direction > FLOW_DIRECTION_LEFT) {
        m_bus.send(
            message.createErrorReply({QDBusError::InvalidArgs, QStringLiteral("invalid args")}));
        return;
    }

    m_machine->setFlowDirection(static_cast<FlowDirection>(direction));
}

void MachineDBusAdaptor::SendFiles(const QStringList &paths, const QDBusMessage &message) const {
    if (paths.empty()) {
        m_bus.send(
            message.createErrorReply({QDBusError::InvalidArgs, QStringLiteral("paths is empty")}));
        return;
    }

    if (!m_machine->connected()) {
        m_bus.send(message.createErrorReply({QDBusError::Failed, QStringLiteral("connect first")}));
        return;
    }

    m_machine->sendFiles(paths);
}

void MachineDBusAdaptor::updateName(const QString &name) {
    propertiesChanged("Name", name);
}

void MachineDBusAdaptor::updateConnected(bool connected) {
    propertiesChanged("Connected", connected);
}

void MachineDBusAdaptor::updateDeviceSharing(bool on) {
    propertiesChanged("DeviceSharing", on);
}

void MachineDBusAdaptor::updateDirection(quint16 direction) {
    propertiesChanged("Direction", direction);
}

void MachineDBusAdaptor::propertiesChanged(const QString &property, const QVariant &value) {
    QList<QVariant> arguments;
    arguments.push_back(machineInterface);

    QVariantMap changedProps;
    changedProps.insert(property, value);

    arguments.push_back(changedProps);
    arguments.push_back(QStringList());

    QDBusMessage msg = QDBusMessage::createSignal(m_path,
                                                  "org.freedesktop.DBus.Properties",
                                                  "PropertiesChanged");
    msg.setArguments(arguments);
    m_bus.send(msg);
}