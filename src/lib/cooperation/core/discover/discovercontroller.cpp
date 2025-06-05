﻿// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "discovercontroller.h"
#include "discovercontroller_p.h"
#include "utils/cooperationutil.h"
#include "common/log.h"

#include <QProcess>
#include <QMessageBox>
#include <QTimer>
#include <QJsonDocument>
#include <QDesktopServices>

#include <common/constant.h>
#include <configs/dconfig/dconfigmanager.h>
#include <configs/settings/configmanager.h>

using namespace cooperation_core;

DiscoverControllerPrivate::DiscoverControllerPrivate(DiscoverController *qq)
    : q(qq)
{
}

DiscoverController::DiscoverController(QObject *parent)
    : QObject(parent),
      d(new DiscoverControllerPrivate(this))
{
    qRegisterMetaType<StringMap>("StringMap");
}

DiscoverController::~DiscoverController()
{
}

void DiscoverController::init()
{
    DLOG << "Initializing discovery controller";

    // Connect signals immediately, not dependent on avahi service status
    initConnect();

    // Always create ZeroConf object regardless of service status
    initZeroConf();

    if (isZeroConfDaemonActive()) {
        // Service is running, start publishing and discovery immediately
        DLOG << "Avahi service is active, starting discovery";
        updatePublish();
        // Delay a bit to let service fully start
        QTimer::singleShot(1000, this, [this]() {
            startDiscover();
        });
        return;
    }

    // Service not running, try to start it and wait
    DLOG << "Avahi service not active, attempting to start service";

    if (!openZeroConfDaemonDailog()) {
        // User refused to start service or platform not supported
        DLOG << "User refused or platform not supported, starting compatibility mode only";
        // Still start discovery for compatibility mode
        QTimer::singleShot(1000, this, [this]() {
            startDiscover();
        });
        return;
    }

    // Wait for service to become available with unified timeout
    startServiceWaitLoop();
}

void DiscoverController::startServiceWaitLoop()
{
    QTimer *serviceTimer = new QTimer(this);
    int retryCount = 0;
    const int maxRetries = 5; // 5 retries with 2 second intervals = 10 seconds total

    connect(serviceTimer, &QTimer::timeout, this, [serviceTimer, retryCount, maxRetries, this]() mutable {
        retryCount++;

        if (isZeroConfDaemonActive()) {
            serviceTimer->stop();
            DLOG << "Avahi service became active after" << retryCount << "retries";
            
            // Start browser when service becomes available
            if (!d->zeroConf->browserExists()) {
                DLOG << "Starting ZeroConf browser";
                d->zeroConf->startBrowser(UNI_CHANNEL);
            }
            
            // Update publishing when service becomes available
            updatePublish();
            // Start discovery
            QTimer::singleShot(500, this, [this]() {
                startDiscover();
            });
            return;
        }

        if (retryCount >= maxRetries) {
            serviceTimer->stop();
            WLOG << "Avahi service startup timeout after" << maxRetries << "retries";
            // Start discovery anyway for compatibility mode
            startDiscover();
        }
    });

    serviceTimer->start(2000); // Check every 2 seconds
}

void DiscoverController::initZeroConf()
{
    DLOG << "Initializing ZeroConf service";
    d->zeroConf = new QZeroConf();
    d->zeroconfname = QSysInfo::machineUniqueId();

    // Connect ZeroConf signals
    connectZeroConfSignals();

    // Only start browser when avahi service is active
    if (isZeroConfDaemonActive() && !d->zeroConf->browserExists()) {
        DLOG << "Starting ZeroConf browser";
        d->zeroConf->startBrowser(UNI_CHANNEL);
    }
}

void DiscoverController::connectZeroConfSignals()
{
    if (!d->zeroConf) {
        WLOG << "Cannot connect ZeroConf signals - ZeroConf not initialized";
        return;
    }

    connect(d->zeroConf, &QZeroConf::serviceAdded, this, &DiscoverController::addService);
    connect(d->zeroConf, &QZeroConf::serviceRemoved, this, &DiscoverController::removeService);
    connect(d->zeroConf, &QZeroConf::serviceUpdated, this, &DiscoverController::updateService);
}

void DiscoverController::initConnect()
{
    DLOG << "Setting up discovery controller connections";

    // Ensure signals are connected only once, avoid duplicate connections
    static bool connected = false;
    if (connected) {
        DLOG << "Signals already connected, skipping";
        return;
    }
    connected = true;

    connect(CooperationUtil::instance(), &CooperationUtil::onlineStateChanged, this, [this](const QString &validIP) {
        if (validIP.isEmpty())
            return;
        updatePublish();
        startDiscover();
    });
#ifdef linux
    connect(DConfigManager::instance(), &DConfigManager::valueChanged, this, &DiscoverController::onDConfigValueChanged);
#endif
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &DiscoverController::onAppAttributeChanged);

    // ZeroConf related connections need to be established after ZeroConf initialization
    // These connections will be established when needed through connectZeroConfSignals() method
}

bool DiscoverController::isVaildDevice(const DeviceInfoPointer info)
{
    if (!info || info->ipAddress().isEmpty() || !info->ipAddress().startsWith(d->ipfilter))
        return false;
    else
        return true;
}

DeviceInfoPointer DiscoverController::parseDeviceJson(const QString &info)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(info.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        WLOG << "error parse device info:" << info.toStdString();
        return nullptr;
    }

    auto map = doc.toVariant().toMap();
    auto devInfo = DeviceInfo::fromVariantMap(map);
    devInfo->setConnectStatus(DeviceInfo::Connectable);

    return devInfo;
}

DeviceInfoPointer DiscoverController::parseDeviceService(QZeroConfService zcs)
{
    QVariantMap infomap;
    for (const auto &key : zcs->txt().keys())
        infomap.insert(key, QString::fromUtf8(QByteArray::fromBase64(zcs->txt().value(key))));

    auto devInfo = DeviceInfo::fromVariantMap(infomap);
    if (!isVaildDevice(devInfo))
        return nullptr;

    auto old = findDeviceByIP(devInfo->ipAddress());
    if (old) {
        // update its status
        devInfo->setConnectStatus(old->connectStatus());
    } else {
        // set default status
        devInfo->setConnectStatus(DeviceInfo::Connectable);
    }

    return devInfo;
}

void DiscoverController::deviceLosted(const QString &ip)
{
    // update its status or remove it
    auto oldinfo = findDeviceByIP(ip);
    if (oldinfo) {
        if (_historyDevices.contains(ip)) {
            // just need to update status
            oldinfo->setConnectStatus(DeviceInfo::Offline);
            Q_EMIT deviceOnline({oldinfo});
            return;
        } else {
            d->onlineDeviceList.removeOne(oldinfo);
        }
    }

    // notify to remove it
    Q_EMIT deviceOffline(ip);
}

QList<DeviceInfoPointer> DiscoverController::getOnlineDeviceList() const
{
    return d->onlineDeviceList;
}

bool DiscoverController::openZeroConfDaemonDailog()
{
#ifdef __linux__
    CooperationDialog dlg;
    dlg.setIcon(QIcon::fromTheme("dialog-warning"));
    dlg.addButton(tr("Confirm"), true, CooperationDialog::ButtonRecommend);
    dlg.addButton(tr("Close"), false, CooperationDialog::ButtonWarning);

    dlg.setTitle(tr("Please click to confirm to enable the LAN discovery service!"));
    dlg.setMessage(tr("Unable to discover and be discovered by other devices when LAN discovery service is not turned on"));

    int code = dlg.exec();
    if (code == 0) {
        QProcess::startDetached("systemctl", QStringList() << "start" << "avahi-daemon.service");
        return true;
    }
#else
    int choice = QMessageBox::warning(nullptr, tr("Please click to confirm to enable the LAN discovery service!"),
                                      tr("Unable to discover and be discovered by other devices when LAN discovery service is not turned on"
                                         "Right click on Windows Start menu ->Computer Management ->Services and Applications ->Services to enable Bonjour service"),
                                      QMessageBox::Ok);
    QDesktopServices::openUrl(QUrl("services.msc"));
#endif
    return false;
}

bool DiscoverController::isZeroConfDaemonActive()
{
#ifdef __linux__
    QProcess process;
    process.start("systemctl", QStringList() << "is-active"
                                             << "avahi-daemon.service");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        if (output.contains("active")) {
            LOG << "Avahi service is running";
            return true;
        } else {
            WLOG << "Avahi service is not running";
            return false;
        }
    } else {
        ELOG << "Error: " << error.toStdString();
        return false;
    }

#else
    QProcess process;
    process.start("sc query \"Bonjour Service\"");
    process.waitForFinished();
    QByteArray output = process.readAllStandardOutput();
    QString res = QString::fromLocal8Bit(output);

    if (res.contains("RUNNING"))
        return true;
    else
        return false;
#endif
}

DeviceInfoPointer DiscoverController::findDeviceByIP(const QString &ip)
{
    for (int i = 0; i < d->onlineDeviceList.size(); ++i) {
        auto info = d->onlineDeviceList[i];
        if (info->ipAddress() == ip)
            return info;
    }
    return nullptr;
}

DeviceInfoPointer DiscoverController::selfInfo()
{
    return DeviceInfo::fromVariantMap(CooperationUtil::deviceInfo());
}

void DiscoverController::updateDeviceState(const DeviceInfoPointer info)
{
    auto oldinfo = findDeviceByIP(info->ipAddress());
    if (oldinfo)
        d->onlineDeviceList.removeOne(oldinfo);

    if (DeviceInfo::Connected == info->connectStatus()) {
        //record the connected status IP
        _connectedDevice = info->ipAddress();
    } else {
        _connectedDevice = "";
    }

    d->onlineDeviceList.append(info);
    Q_EMIT deviceOnline({ info });
}

void DiscoverController::onDConfigValueChanged(const QString &config, const QString &key)
{
    Q_UNUSED(key);
    if (config != kDefaultCfgPath)
        return;

    updatePublish();
}

void DiscoverController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != AppSettings::GenericGroup)
        return;

    if (key == AppSettings::StoragePathKey)
        CooperationUtil::instance()->setStorageConfig(value.toString());

    updatePublish();
}

void DiscoverController::addService(QZeroConfService zcs)
{
    if (zcs.get()->name() == d->zeroconfname) {
        LOG << "add service, ignore self zcs service";
        return;
    }
    auto devInfo = parseDeviceService(zcs);

    if (!devInfo)
        return;

    d->onlineDeviceList.append(devInfo);
    Q_EMIT deviceOnline({ devInfo });
}

void DiscoverController::updateService(QZeroConfService zcs)
{
    if (zcs.get()->name() == d->zeroconfname) {
        LOG << "update service, ignore self zcs service";
        return;
    }
    auto devInfo = parseDeviceService(zcs);

    if (!devInfo)
        return;
    auto oldinfo = findDeviceByIP(devInfo->ipAddress());
    if (oldinfo)
        d->onlineDeviceList.removeOne(oldinfo);

    d->onlineDeviceList.append(devInfo);
    Q_EMIT deviceOnline({ devInfo });
}

void DiscoverController::removeService(QZeroConfService zcs)
{
    auto devInfo = parseDeviceService(zcs);

    if (!devInfo)
        return;

    deviceLosted(devInfo->ipAddress());
}

void DiscoverController::updateHistoryDevices(const QMap<QString, QString> &connectMap)
{
    _historyDevices.clear();
    d->historyDeviceMap.clear(); // Store complete history device info

    QList<DeviceInfoPointer> offlineDevList;
    auto iter = connectMap.begin();
    for (; iter != connectMap.end(); ++iter) {
        QString ip = iter.key();
        QString deviceName = iter.value();

        _historyDevices.append(ip);
        d->historyDeviceMap[ip] = deviceName; // Store complete info

        if (findDeviceByIP(ip))
            continue;

        DeviceInfoPointer info(new DeviceInfo(ip, deviceName));
        info->setConnectStatus(DeviceInfo::Offline);
        offlineDevList << info;
    }

    if (!offlineDevList.isEmpty())
        Q_EMIT deviceOnline(offlineDevList);
}

DiscoverController *DiscoverController::instance()
{
    static DiscoverController ins;
    return &ins;
}

void DiscoverController::publish()
{
    DLOG << "Publishing device info via ZeroConf";

    QVariantMap deviceInfo = CooperationUtil::deviceInfo();
    // Set to not discoverable on LAN
    if (deviceInfo.value(AppSettings::DiscoveryModeKey) == 1) {
        unpublish();
        return;
    }

    QString selfIP = deviceInfo.value(AppSettings::IPAddress).toString();
    d->ipfilter = selfIP.lastIndexOf(".") != -1 ? selfIP.left(selfIP.lastIndexOf(".")) : "";

    LOG << "publish " << d->zeroconfname.toStdString() << " on: " << selfIP.toStdString();

    // Always emit compatibility mode info (UDP multicast discovery)
    auto doc = QJsonDocument::fromVariant(deviceInfo);
    Q_EMIT registCompatAppInfo(true, doc.toJson());
    
    // Only publish via avahi if service is available
    if (isZeroConfDaemonActive() && d->zeroConf) {
        d->zeroConf->clearServiceTxtRecords();
        for (const auto &key : deviceInfo.keys())
            d->zeroConf->addServiceTxtRecord(key, deviceInfo.value(key).toString().toUtf8().toBase64());
        d->zeroConf->startServicePublish(d->zeroconfname.toUtf8(), UNI_CHANNEL, "local", UNI_RPC_PORT_UDP);
    } else {
        DLOG << "Avahi service not available, skipping ZeroConf publishing";
    }
}

void DiscoverController::unpublish()
{
    DLOG << "Stopping ZeroConf service publishing";

    // Always stop compatibility mode (UDP multicast discovery)
    Q_EMIT registCompatAppInfo(false, "");

    // Only stop avahi publishing if service is available and object exists
    if (d->zeroConf && isZeroConfDaemonActive()) {
        d->zeroConf->stopServicePublish();
    }
}

void DiscoverController::updatePublish()
{
    if (!d->zeroConf) {
        WLOG << "Cannot update publish - ZeroConf not initialized";
        return;
    }

    DLOG << "Updating ZeroConf service publishing";
    unpublish();
    publish();
}

void DiscoverController::refresh()
{
    if (!d->zeroConf) {
        WLOG << "Cannot refresh - ZeroConf not initialized";
        finishDiscoveryWithError("ZeroConf not initialized for refresh");
        return;
    }

    DLOG << "Refreshing discovered devices list";

    // Clear current list and collect new devices
    refreshDeviceList();

    // Notify results
    finishDiscovery();
}

void DiscoverController::refreshDeviceList()
{
    // Preserve current online/connected history devices before clearing
    QList<DeviceInfoPointer> preservedHistoryDevices;
    for (const auto &device : d->onlineDeviceList) {
        if (_historyDevices.contains(device->ipAddress()) && 
            device->connectStatus() != DeviceInfo::Offline) {
            preservedHistoryDevices.append(device);
            DLOG << "Preserving online history device:" << device->ipAddress().toStdString();
        }
    }

    d->onlineDeviceList.clear();

    // Collect devices from different sources
    collectAvahiDevices();
    addPreservedHistoryDevices(preservedHistoryDevices);
    collectOfflineHistoryDevices();
    addSearchDeviceIfExists();
}

void DiscoverController::addPreservedHistoryDevices(const QList<DeviceInfoPointer> &preservedDevices)
{
    for (const auto &device : preservedDevices) {
        // Check if device was found by avahi (might have updated status)
        auto existingDevice = findDeviceByIP(device->ipAddress());
        if (!existingDevice) {
            // Device not found by avahi, keep its previous status
            d->onlineDeviceList.append(device);
            DLOG << "Re-added preserved history device:" << device->ipAddress().toStdString();
        } else {
            DLOG << "History device found by avahi, using avahi info:" << device->ipAddress().toStdString();
        }
    }
}

void DiscoverController::collectOfflineHistoryDevices()
{
    DLOG << "Collecting offline history devices, count:" << _historyDevices.size();

    // Add offline history devices that are not already in the list
    for (const QString &historyIP : _historyDevices) {
        if (findDeviceByIP(historyIP)) {
            DLOG << "History device already in list:" << historyIP.toStdString();
            continue; // Already in list (from avahi, search, or preserved)
        }

        // Get complete device info from stored map
        QString deviceName = d->historyDeviceMap.value(historyIP, "Unknown Device");
        DeviceInfoPointer historyDevice(new DeviceInfo(historyIP, deviceName));
        historyDevice->setConnectStatus(DeviceInfo::Offline);
        d->onlineDeviceList.append(historyDevice);
        DLOG << "Added offline history device:" << historyIP.toStdString() << "name:" << deviceName.toStdString();
    }
}

void DiscoverController::collectAvahiDevices()
{
    // Check if avahi service is available for actual discovery
    if (!isZeroConfDaemonActive()) {
        DLOG << "Avahi service not active, skipping avahi device discovery";
        return;
    }

    auto allServices = d->zeroConf->getServices();
    DLOG << "Found" << allServices.size() << "avahi services";

    for (const auto &key : allServices.keys()) {
        QZeroConfService zcs = allServices.value(key);
        auto devInfo = parseDeviceService(zcs);

        if (!devInfo || devInfo->ipAddress() == CooperationUtil::localIPAddress()) {
            continue;
        }

        // Check if device already exists and merge status
        auto existingDevice = findDeviceByIP(devInfo->ipAddress());
        if (existingDevice) {
            // Update existing device with latest avahi info
            existingDevice->setConnectStatus(devInfo->connectStatus());
            DLOG << "Updated existing device:" << devInfo->ipAddress().toStdString();
            continue;
        }

        // Update connection status for known connected device
        if (_connectedDevice == devInfo->ipAddress()) {
            devInfo->setConnectStatus(DeviceInfo::Connected);
        }

        d->onlineDeviceList.append(devInfo);
        DLOG << "Added avahi device:" << devInfo->ipAddress().toStdString();
    }
}

void DiscoverController::addSearchDeviceIfExists()
{
    if (!d->searchDevice) {
        return;
    }

    // Check if search device is already in the list
    auto existingDevice = findDeviceByIP(d->searchDevice->ipAddress());
    if (existingDevice) {
        DLOG << "Search device already exists in list, skipping";
        return;
    }

    DLOG << "Adding search device to list:" << d->searchDevice->ipAddress().toStdString();
    d->onlineDeviceList.append(d->searchDevice);
}

void DiscoverController::finishDiscovery()
{
    Q_EMIT deviceOnline({ d->onlineDeviceList });
    bool hasFound = !d->onlineDeviceList.isEmpty();
    DLOG << "Discovery finished, found" << d->onlineDeviceList.size() << "devices";
    Q_EMIT discoveryFinished(hasFound);
}

void DiscoverController::addSearchDeivce(const QString &info)
{
    DLOG << "Adding device from search result";
    auto devInfo = parseDeviceJson(info);
    if (!devInfo) {
        finishDiscoveryWithError("Failed to parse search device info");
        return;
    }
    d->searchDevice = devInfo;
    if (devInfo->isValid())
        Q_EMIT deviceOnline({ d->searchDevice });
}

void DiscoverController::compatAddDeivces(StringMap infoMap)
{
    DLOG << "Adding compatible devices, count:" << infoMap.size();
    QList<DeviceInfoPointer> addedList;
    for (auto it = infoMap.constBegin(); it != infoMap.constEnd(); ++it) {
        QString info = it.key();

        auto devInfo = parseDeviceJson(info);
        if (!devInfo) {
            WLOG << "Can not parse peer: " << info.toStdString();
            continue;
        }

        // Parse combinedIP
        QString combinedIP = it.value();
        QStringList ipList = combinedIP.split(", ");
        if (ipList.size() != 2) {
            WLOG << "Invalid combined IP format: " << combinedIP.toStdString();
            continue;
        }

        QString ip = ipList[0];
        QString sharedip = ipList[1];

        if (ip == CooperationUtil::localIPAddress()) {
            WLOG << "Ignore local host ip: " << ip.toStdString();
            continue;
        }

        devInfo->setIpAddress(ip);
        if (devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
            if (sharedip == CooperationUtil::localIPAddress() || _connectedDevice == devInfo->ipAddress())
                devInfo->setConnectStatus(DeviceInfo::Connected);

            d->onlineDeviceList.append(devInfo);
            addedList.append(devInfo);
        }
    }

    if (!addedList.isEmpty()) {
        Q_EMIT deviceOnline(addedList);
    }
}

void DiscoverController::compatRemoveDeivce(const QString &ip)
{
    DLOG << "Removing compatible device with IP:" << ip.toStdString();
    deviceLosted(ip);
}

void DiscoverController::startDiscover()
{
    if (!d->zeroConf) {
        WLOG << "Cannot start discovery - ZeroConf not initialized";
        finishDiscoveryWithError("ZeroConf not initialized");
        return;
    }

    DLOG << "Starting device discovery";

    // Delay to show discovery interface
    QTimer::singleShot(500, [this]() {
        // clear current list first.
        refresh();
        // compation: start get nodes
        Q_EMIT startDiscoveryDevice();
    });
}

void DiscoverController::finishDiscoveryWithError(const QString &errorMsg)
{
    WLOG << "Discovery failed:" << errorMsg.toStdString();
    Q_EMIT discoveryFinished(false);
}
