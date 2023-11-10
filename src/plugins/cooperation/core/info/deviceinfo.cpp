// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deviceinfo.h"
#include "deviceinfo_p.h"
#include "global_defines.h"

#include <QJsonDocument>

inline constexpr char IPAddress[] { "IPAddress" };

using namespace cooperation_core;

DeviceInfoPrivate::DeviceInfoPrivate(DeviceInfo *qq)
    : q(qq)
{
}

DeviceInfo::DeviceInfo()
    : d(new DeviceInfoPrivate(this))
{
}

DeviceInfo::DeviceInfo(const QString &ip, const QString &name)
    : d(new DeviceInfoPrivate(this))
{
    d->deviceName = name;
    d->ipAddress = ip;
}

DeviceInfo::~DeviceInfo()
{
}

bool DeviceInfo::isValid()
{
    return !(deviceName().isEmpty() || ipAddress().isEmpty());
}

void DeviceInfo::setIpAddress(const QString &ip)
{
    d->ipAddress = ip;
}

QString DeviceInfo::ipAddress() const
{
    return d->ipAddress;
}

void DeviceInfo::setConnectStatus(ConnectStatus status)
{
    d->conStatus = status;
}

DeviceInfo::ConnectStatus DeviceInfo::connectStatus() const
{
    return d->conStatus;
}

void DeviceInfo::setDeviceName(const QString &name)
{
    d->deviceName = name;
}

QString DeviceInfo::deviceName() const
{
    return d->deviceName;
}

void DeviceInfo::setTransMode(DeviceInfo::TransMode mode)
{
    d->transMode = mode;
}

DeviceInfo::TransMode DeviceInfo::transMode() const
{
    return d->transMode;
}

void DeviceInfo::setDiscoveryMode(DeviceInfo::DiscoveryMode mode)
{
    d->discoveryMode = mode;
}

DeviceInfo::DiscoveryMode DeviceInfo::discoveryMode() const
{
    return d->discoveryMode;
}

void DeviceInfo::setLinkMode(DeviceInfo::LinkMode mode)
{
    d->linkMode = mode;
}

DeviceInfo::LinkMode DeviceInfo::linkMode() const
{
    return d->linkMode;
}

void DeviceInfo::setPeripheralShared(bool b)
{
    d->isPeripheralShared = b;
}

bool DeviceInfo::peripheralShared() const
{
    return d->isPeripheralShared;
}

void DeviceInfo::setClipboardShared(bool b)
{
    d->isClipboardShared = b;
}

bool DeviceInfo::clipboardShared() const
{
    return d->isClipboardShared;
}

QVariantMap DeviceInfo::toVariantMap()
{
    QVariantMap map;
    map.insert(IPAddress, d->ipAddress);
    map.insert(AppSettings::DeviceNameKey, d->deviceName);
    map.insert(AppSettings::TransferModeKey, static_cast<int>(d->transMode));
    map.insert(AppSettings::DiscoveryModeKey, static_cast<int>(d->discoveryMode));
    map.insert(AppSettings::LinkDirectionKey, static_cast<int>(d->linkMode));
    map.insert(AppSettings::ClipboardShareKey, d->isClipboardShared);
    map.insert(AppSettings::PeripheralShareKey, d->isPeripheralShared);

    return map;
}

DeviceInfoPointer DeviceInfo::fromVariantMap(const QVariantMap &map)
{
    if (map.isEmpty())
        return {};

    DeviceInfoPointer info = DeviceInfoPointer(new DeviceInfo);
    info->setIpAddress(map.value(IPAddress).toString());
    info->setDeviceName(map.value(AppSettings::DeviceNameKey).toString());
    info->setTransMode(static_cast<TransMode>(map.value(AppSettings::TransferModeKey).toInt()));
    info->setDiscoveryMode(static_cast<DiscoveryMode>(map.value(AppSettings::DiscoveryModeKey).toInt()));
    info->setLinkMode(static_cast<LinkMode>(map.value(AppSettings::LinkDirectionKey).toInt()));
    info->setClipboardShared(map.value(AppSettings::ClipboardShareKey).toBool());
    info->setPeripheralShared(map.value(AppSettings::PeripheralShareKey).toBool());

    return info;
}

DeviceInfo &DeviceInfo::operator=(const DeviceInfo &info)
{
    d->deviceName = info.d->deviceName;
    d->ipAddress = info.d->ipAddress;
    d->conStatus = info.d->conStatus;
    d->transMode = info.d->transMode;
    d->discoveryMode = info.d->discoveryMode;
    d->linkMode = info.d->linkMode;
    d->isClipboardShared = info.d->isClipboardShared;
    d->isPeripheralShared = info.d->isPeripheralShared;

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
