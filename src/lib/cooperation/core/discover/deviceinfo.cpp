// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deviceinfo.h"
#include "deviceinfo_p.h"
#include "global_defines.h"
#include "common/log.h"

#include <QJsonDocument>

using namespace cooperation_core;
using namespace deepin_cross;

DeviceInfoPrivate::DeviceInfoPrivate(DeviceInfo *qq)
    : q(qq)
{
    DLOG << "DeviceInfoPrivate created";
}

DeviceInfo::DeviceInfo()
    : d(new DeviceInfoPrivate(this))
{
    DLOG << "DeviceInfo created";
}

DeviceInfo::DeviceInfo(const QString &ip, const QString &name)
    : d(new DeviceInfoPrivate(this))
{
    DLOG << "DeviceInfo created with ip:" << ip.toStdString() << "and name:" << name.toStdString();
    d->deviceName = name;
    d->ipAddress = ip;
}

DeviceInfo::DeviceInfo(const DeviceInfo &other)
    : DeviceInfo()
{
    DLOG << "DeviceInfo copy constructor";
    operator=(other);
}

DeviceInfo::~DeviceInfo()
{
    DLOG << "DeviceInfo destroyed";
}

bool DeviceInfo::isValid()
{
    bool valid = !(deviceName().isEmpty() || ipAddress().isEmpty());
    DLOG << "Device is valid:" << valid;
    return valid;
}

void DeviceInfo::setOsType(BaseUtils::OS_TYPE type)
{
    DLOG << "Setting OS type to:" << type;
    d->osType = type;
}

BaseUtils::OS_TYPE DeviceInfo::osType() const
{
    return d->osType;
}

void DeviceInfo::setIpAddress(const QString &ip)
{
    DLOG << "Setting IP address to:" << ip.toStdString();
    d->ipAddress = ip;
}

QString DeviceInfo::ipAddress() const
{
    return d->ipAddress;
}

void DeviceInfo::setConnectStatus(ConnectStatus status)
{
    DLOG << "Setting connect status to:" << (int)status;
    d->conStatus = status;
}

DeviceInfo::ConnectStatus DeviceInfo::connectStatus() const
{
    return d->conStatus;
}

void DeviceInfo::setDeviceName(const QString &name)
{
    DLOG << "Setting device name to:" << name.toStdString();
    d->deviceName = name;
}

QString DeviceInfo::deviceName() const
{
    return d->deviceName;
}

void DeviceInfo::setTransMode(DeviceInfo::TransMode mode)
{
    DLOG << "Setting transfer mode to:" << (int)mode;
    d->transMode = mode;
}

DeviceInfo::TransMode DeviceInfo::transMode() const
{
    return d->transMode;
}

void DeviceInfo::setDiscoveryMode(DeviceInfo::DiscoveryMode mode)
{
    DLOG << "Setting discovery mode to:" << (int)mode;
    d->discoveryMode = mode;
}

DeviceInfo::DiscoveryMode DeviceInfo::discoveryMode() const
{
    return d->discoveryMode;
}

void DeviceInfo::setLinkMode(DeviceInfo::LinkMode mode)
{
    DLOG << "Setting link mode to:" << (int)mode;
    d->linkMode = mode;
}

DeviceInfo::LinkMode DeviceInfo::linkMode() const
{
    return d->linkMode;
}

void DeviceInfo::setDeviceType(DeviceInfo::DeviceType type)
{
    DLOG << "Setting device type to:" << (int)type;
    d->deviceType = type;
}

DeviceInfo::DeviceType DeviceInfo::deviceType() const
{
    return d->deviceType;
}

void DeviceInfo::setPeripheralShared(bool b)
{
    DLOG << "Setting peripheral shared to:" << b;
    d->isPeripheralShared = b;
}

bool DeviceInfo::peripheralShared() const
{
    return d->isPeripheralShared;
}

void DeviceInfo::setClipboardShared(bool b)
{
    DLOG << "Setting clipboard shared to:" << b;
    d->isClipboardShared = b;
}

bool DeviceInfo::clipboardShared() const
{
    return d->isClipboardShared;
}

void DeviceInfo::setCooperationEnable(bool enable)
{
    DLOG << "Setting cooperation enable to:" << enable;
    d->cooperationEnabled = enable;
}

bool DeviceInfo::cooperationEnable() const
{
    return d->cooperationEnabled;
}

QVariantMap DeviceInfo::toVariantMap()
{
    // DLOG << "Converting DeviceInfo to QVariantMap";
    QVariantMap map;
    map.insert(AppSettings::IPAddress, d->ipAddress);
    map.insert(AppSettings::OSType, d->osType);
    map.insert(AppSettings::DeviceNameKey, d->deviceName);
    map.insert(AppSettings::TransferModeKey, static_cast<int>(d->transMode));
    map.insert(AppSettings::DiscoveryModeKey, static_cast<int>(d->discoveryMode));
    map.insert(AppSettings::LinkDirectionKey, static_cast<int>(d->linkMode));
    map.insert(AppSettings::ClipboardShareKey, d->isClipboardShared);
    map.insert(AppSettings::PeripheralShareKey, d->isPeripheralShared);
    map.insert(AppSettings::CooperationEnabled, d->cooperationEnabled);

    return map;
}

DeviceInfoPointer DeviceInfo::fromVariantMap(const QVariantMap &map)
{
    // DLOG << "Creating DeviceInfo from QVariantMap";
    if (map.isEmpty()) {
        DLOG << "Map is empty, returning null pointer";
        return {};
    }

    DeviceInfoPointer info = DeviceInfoPointer(new DeviceInfo);
    info->setIpAddress(map.value(AppSettings::IPAddress).toString());
    info->setDeviceName(map.value(AppSettings::DeviceNameKey).toString());
    info->setTransMode(static_cast<TransMode>(map.value(AppSettings::TransferModeKey).toInt()));
    info->setDiscoveryMode(static_cast<DiscoveryMode>(map.value(AppSettings::DiscoveryModeKey).toInt()));
    info->setLinkMode(static_cast<LinkMode>(map.value(AppSettings::LinkDirectionKey).toInt()));
    info->setClipboardShared(map.value(AppSettings::ClipboardShareKey).toBool());
    info->setPeripheralShared(map.value(AppSettings::PeripheralShareKey).toBool());
    info->setCooperationEnable(map.value(AppSettings::CooperationEnabled).toBool());
    info->setOsType(static_cast<BaseUtils::OS_TYPE>(map.value(AppSettings::OSType).toInt()));

    return info;
}

DeviceInfo &DeviceInfo::operator=(const DeviceInfo &info)
{
    // DLOG << "Assigning DeviceInfo from another object";
    d->deviceName = info.d->deviceName;
    d->ipAddress = info.d->ipAddress;
    d->conStatus = info.d->conStatus;
    d->transMode = info.d->transMode;
    d->discoveryMode = info.d->discoveryMode;
    d->linkMode = info.d->linkMode;
    d->isClipboardShared = info.d->isClipboardShared;
    d->isPeripheralShared = info.d->isPeripheralShared;
    d->cooperationEnabled = info.d->cooperationEnabled;
    d->osType = info.d->osType;

    return *this;
}

bool DeviceInfo::operator==(const DeviceInfo &info) const
{
    return d->ipAddress == info.d->ipAddress;
}

bool DeviceInfo::operator!=(const DeviceInfo &info) const
{
    return !(operator==(info));
}
