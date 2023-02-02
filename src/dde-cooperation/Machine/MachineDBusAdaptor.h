// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MACHINE_MACHINEDBUSADAPTOR_H
#define MACHINE_MACHINEDBUSADAPTOR_H

#include <memory>

#include <QDBusConnection>
#include <QVector>
#include <QDBusAbstractAdaptor>

class Manager;
class Machine;

class MachineDBusAdaptor : public QDBusAbstractAdaptor {
    friend class Machine;

    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Cooperation1.Machine")

    Q_PROPERTY(QString UUID READ getUUID)
    Q_PROPERTY(QString Name READ getName)
    Q_PROPERTY(QString IP READ getIP)
    Q_PROPERTY(quint16 Port READ getPort)
    Q_PROPERTY(quint32 OS READ getOS)
    Q_PROPERTY(quint32 Compositor READ getCompositor)
    Q_PROPERTY(bool Connected READ getConnected)
    Q_PROPERTY(bool DeviceSharing READ getDeviceSharing)
    Q_PROPERTY(quint16 Direction READ getDirection)
    Q_PROPERTY(bool SharedClipboard READ getSharedClipboard)

public:
    MachineDBusAdaptor(Manager *manager,
                       Machine *machine,
                       QDBusConnection bus,
                       const QString &path);
    ~MachineDBusAdaptor();

public: // D-Bus properties
    QString getUUID() const;
    QString getName() const;
    QString getIP() const;
    quint16 getPort() const;
    quint32 getOS() const;
    quint32 getCompositor() const;
    bool getConnected() const;
    bool getDeviceSharing() const;
    quint16 getDirection() const;
    bool getSharedClipboard() const;

public slots: // D-Bus methods
    void Connect(const QDBusMessage &message) const;
    void Disconnect() const;
    void RequestDeviceSharing(const QDBusMessage &message) const;
    void StopDeviceSharing(const QDBusMessage &message) const;
    void SetFlowDirection(quint16 direction, const QDBusMessage &message) const;
    void SendFiles(const QStringList &paths, const QDBusMessage &message) const;

protected: // update properties
    void updateName(const QString &name);
    void updateConnected(bool connected);
    void updateDeviceSharing(bool on);
    void updateDirection(quint16 direction);

private:
    void propertiesChanged(const QString &property, const QVariant &value);

private:
    QString m_path;
    Manager *m_manager;
    Machine *m_machine;
    QDBusConnection m_bus;
};

#endif // !MACHINE_MACHINEDBUSADAPTOR_H
