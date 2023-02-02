// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MANAGERDBUSADAPTOR_H
#define MANAGERDBUSADAPTOR_H

#include <memory>

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QVector>

class Manager;

class ManagerDBusAdaptor : public QObject {
    friend class Manager;

    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Cooperation1")

    Q_PROPERTY(QVector<QDBusObjectPath> Machines READ getMachines)
    Q_PROPERTY(QStringList CooperatedMachines READ getCooperatedMachines)
    Q_PROPERTY(bool DeviceSharingSwitch READ getDeviceSharingSwitch)
    Q_PROPERTY(QString FilesStoragePath READ getFilesStoragePath)
    Q_PROPERTY(bool SharedClipboard READ getSharedClipboard)
    Q_PROPERTY(bool SharedDevices READ getSharedDevices)

public:
    ManagerDBusAdaptor(Manager *manager, QDBusConnection bus);

public: // D-Bus properties
    QVector<QDBusObjectPath> getMachines() const;
    QStringList getCooperatedMachines() const;
    bool getDeviceSharingSwitch() const;
    QString getFilesStoragePath() const;
    bool getSharedClipboard() const;
    bool getSharedDevices() const;

public slots: // D-Bus methods
    void Scan(const QDBusMessage &message) const;
    void Knock(const QString &ip, quint16 port) const;
    void ConnectAndroidDevice() const;
    void SendFile(const QStringList &files, int osType, const QDBusMessage &message) const;
    void SetFilesStoragePath(const QString &path, const QDBusMessage &message) const;
    void OpenSharedClipboard(bool on) const;
    void OpenSharedDevices(bool on) const;
    void SetDeviceSharingSwitch(bool value) const;

protected: // update properties
    void updateMachines(const QVector<QDBusObjectPath> &machines);
    void updateCooperatedMachines(const QStringList &UUIDs);
    void updateDeviceSharingSwitch(bool on);
    void updateFileStoragePath(const QString &path);
    void updateSharedClipboard(bool on);
    void updateSharedDevices(bool on);

private:
    void propertiesChanged(const QString &property, const QVariant &value);

private:
    Manager *m_manager;
    QDBusConnection m_bus;
};

#endif // !MANAGERDBUSADAPTOR_H
