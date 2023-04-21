// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MANAGER_H
#define MANAGER_H

#include <unordered_map>
#include <filesystem>
#include <memory>
#include <thread>

#include <arpa/inet.h>

#include <QtDBus>

#include <DConfig>

#include "Wrappers/InputGrabberWrapper.h"
#include "ClipboardBase.h"
#include "ManagerDBusAdaptor.h"

#include "protocol/pair.pb.h"

DCORE_USE_NAMESPACE

class QUdpSocket;
class QTcpServer;
class QHostAddress;

class ManagerDBusAdaptor;
class FsSendFileRequest;
class FsRequest;
class FsResponse;
class Machine;
class DisplayBase;
class ClipboardBase;
class AndroidMainWindow;
class InputGrabbersManager;

class Manager : public QObject, public ClipboardObserver {
    friend class ManagerDBusAdaptor;

    Q_OBJECT
    Q_DISABLE_COPY(Manager)

public:
    Manager(const std::filesystem::path &dataDir);
    ~Manager();

    std::string uuid() const noexcept { return m_uuid; }
    QString fileStoragePath() const noexcept { return m_fileStoragePath; }
    bool isSharedClipboard() const noexcept { return m_sharedClipboard; }
    bool isSharedDevices() const noexcept { return m_sharedDevices; }

    bool tryFlowOut(uint16_t direction, uint16_t x, uint16_t y, bool evFromPeer);
    bool hasPcMachinePaired() const;
    bool hasAndroidPaired() const;
    void machineCooperated(const std::string &machineId);

    void ping(const std::string &ip, uint16_t port = m_scanPort);
    void onMachineOffline(const std::string &uuid);
    void onStartDeviceSharing(const std::weak_ptr<Machine> &machine, bool proactively);
    void onStopDeviceSharing();
    void onFlowBack(uint16_t direction, uint16_t x, uint16_t y);
    void onFlowOut(const std::weak_ptr<Machine> &machine);
    virtual void onClipboardTargetsChanged(const std::vector<std::string> &targets) override;
    virtual bool onReadClipboardContent(const std::string &target) override;
    void onMachineOwnClipboard(const std::weak_ptr<Machine> &machine,
                               const std::vector<std::string> &targets);
    void onInputEvent();

    const QString &getFileStoragePath() const { return m_fileStoragePath; }
    void completeDeviceInfo(DeviceInfo *info);
    QPointer<AndroidMainWindow> getAndroidMainWindow();

protected:
    void scan() noexcept;
    void connectNewAndroidDevice() noexcept;
    bool sendFile(const QStringList &files, int osType) noexcept;
    void setFileStoragePath(const QString &path) noexcept;
    void openSharedClipboard(bool on) noexcept;
    void openSharedDevices(bool on) noexcept;
    bool setDeviceSharingSwitch(bool value) noexcept;

private:
    QDBusConnection m_bus;
    ManagerDBusAdaptor *m_dbusAdaptor;

    const std::filesystem::path m_dataDir;
    const std::filesystem::path m_mountRoot;

    uint32_t m_lastMachineIndex;
    std::unordered_map<std::string, std::shared_ptr<Machine>> m_machines;
    bool m_deviceSharingSwitch;
    std::weak_ptr<Machine> m_clipboardOwner;
    std::unique_ptr<DisplayBase> m_displayServer;
    std::unique_ptr<ClipboardBase> m_clipboard;

    std::thread m_uvThread;
    QUdpSocket *m_socketScan;
    uint16_t m_port;
    QTcpServer *m_listenPair;
    static const uint16_t m_scanPort = 51595;

    std::string m_uuid;
    QString m_fileStoragePath;

    QDBusInterface m_powersaverProxy;

    int m_deviceSharingCnt;
    uint32_t m_inhibitCookie;

    bool m_sharedClipboard;
    bool m_sharedDevices;
    QStringList m_cooperatedMachines;
    std::shared_ptr<DConfig> m_dConfig;

    QPointer<AndroidMainWindow> m_androidMainWindow;
    InputGrabbersManager *m_inputGrabbersManager;

    void ensureDataDirExists();
    void initUUID();
    std::string newUUID() const;
    bool isValidUUID(const std::string &str) const noexcept;
    QString addrToString(const QHostAddress &addr) const;

    void initFileStoragePath();
    void initSharedClipboardStatus();
    void initSharedDevicesStatus();
    void initCooperatedMachines();
    void initServiceSwitch();

    void cooperationStatusChanged(bool enable);
    void updateMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);
    void addMachine(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);
    QVector<QDBusObjectPath> getMachinePaths() const noexcept;
    void inhibitScreensaver();
    void unInhibitScreensaver();

    void handleSocketError(const std::string &title, const std::string &msg);
    void handleReceivedSocketScan() noexcept;
    void handleNewConnection() noexcept;
    void handleNewConnectionFirstPacket() noexcept;
    void sendServiceStoppedNotification() const;
    void serviceStatusChanged();
};

#endif // !MANAGER_H
