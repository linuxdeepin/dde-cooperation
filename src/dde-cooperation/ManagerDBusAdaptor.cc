// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ManagerDBusAdaptor.h"

#include <filesystem>

#include "Manager.h"

#define DDE_PROTO_VER 13

static const QString managerService{"org.deepin.dde.Cooperation1"};
static const QString managerPath{"/org/deepin/dde/Cooperation1"};
static const QString managerInterface{"org.deepin.dde.Cooperation1"};

ManagerDBusAdaptor::ManagerDBusAdaptor(Manager *manager, QDBusConnection bus)
    : QObject(manager)
    , m_manager(manager)
    , m_bus(bus) {
    m_bus.registerService(managerService);
    m_bus.registerObject(managerPath,
                         this,
                         QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllProperties);
}

QVector<QDBusObjectPath> ManagerDBusAdaptor::getMachines() const {
    return m_manager->getMachinePaths();
}

QStringList ManagerDBusAdaptor::getCooperatedMachines() const {
    return m_manager->m_cooperatedMachines;
}

bool ManagerDBusAdaptor::getDeviceSharingSwitch() const {
    return m_manager->m_deviceSharingSwitch;
}

QString ManagerDBusAdaptor::getFilesStoragePath() const {
    return m_manager->m_fileStoragePath;
}

bool ManagerDBusAdaptor::getSharedClipboard() const {
    return m_manager->m_sharedClipboard;
}

bool ManagerDBusAdaptor::getSharedDevices() const {
    return m_manager->m_sharedDevices;
}

void ManagerDBusAdaptor::Scan(const QDBusMessage &message) const {
    if (!m_manager->m_deviceSharingSwitch) {
        message.createErrorReply({QDBusError::Failed, "DeviceSharing Switch close!"});
        qWarning() << "DeviceSharing Switch close";
        return;
    }

    m_manager->scan();
}

void ManagerDBusAdaptor::Knock(const QString &ip, quint16 port) const {
    m_manager->ping(ip.toStdString(), port);
}

void ManagerDBusAdaptor::ConnectAndroidDevice() const {
    m_manager->connectNewAndroidDevice();
}

void ManagerDBusAdaptor::SendFile(const QStringList &files,
                                  int osType,
                                  const QDBusMessage &message) const {
    if (files.empty()) {
        message.createErrorReply({QDBusError::InvalidArgs, "filepath param has error!"});
        return;
    }

    if (!m_manager->sendFile(files, osType)) {
        message.createErrorReply({QDBusError::Failed, "Target machine not found!"});
        return;
    }
}

void ManagerDBusAdaptor::SetFilesStoragePath(const QString &path,
                                             const QDBusMessage &message) const {
    if (path.isEmpty()) {
        message.createErrorReply({QDBusError::InvalidArgs, QStringLiteral("empty path")});
        return;
    }

    if (!QDir(path).exists()) {
        message.createErrorReply({QDBusError::Failed, QStringLiteral("path is not a directory")});
        return;
    }

    m_manager->setFileStoragePath(path);
}

void ManagerDBusAdaptor::OpenSharedClipboard(bool on) const {
    m_manager->openSharedClipboard(on);
}

void ManagerDBusAdaptor::OpenSharedDevices(bool on) const {
    m_manager->openSharedDevices(on);
}

void ManagerDBusAdaptor::SetDeviceSharingSwitch(bool value) const {
    m_manager->setDeviceSharingSwitch(value);
}

void ManagerDBusAdaptor::updateMachines(const QVector<QDBusObjectPath> &machines) {
    QList<QDBusObjectPath> list;
    for (const QDBusObjectPath &path : machines) {
        list.append(path);
    }

    propertiesChanged("Machines", QVariant::fromValue(list));
}

void ManagerDBusAdaptor::updateCooperatedMachines(const QStringList &UUIDs) {
    propertiesChanged("CooperatedMachines", UUIDs);
}

void ManagerDBusAdaptor::updateDeviceSharingSwitch(bool on) {
    propertiesChanged("DeviceSharingSwitch", on);
}

void ManagerDBusAdaptor::updateFileStoragePath(const QString &path) {
    propertiesChanged("FilesStoragePath", path);
}

void ManagerDBusAdaptor::updateSharedClipboard(bool on) {
    propertiesChanged("SharedClipboard", on);
}

void ManagerDBusAdaptor::updateSharedDevices(bool on) {
    propertiesChanged("SharedDevices", on);
}

void ManagerDBusAdaptor::propertiesChanged(const QString &property, const QVariant &value) {
    QList<QVariant> arguments;
    arguments.push_back(managerInterface);

    QVariantMap changedProps;
    changedProps.insert(property, value);

    arguments.push_back(changedProps);
    arguments.push_back(QStringList());

    QDBusMessage msg = QDBusMessage::createSignal(managerPath,
                                                  "org.freedesktop.DBus.Properties",
                                                  "PropertiesChanged");
    msg.setArguments(arguments);
    m_bus.send(msg);
}
