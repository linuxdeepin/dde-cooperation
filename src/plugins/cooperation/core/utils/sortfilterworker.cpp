// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sortfilterworker.h"

using namespace cooperation_core;

SortFilterWorker::SortFilterWorker(QObject *parent)
    : QObject(parent)
{
}

void SortFilterWorker::stop()
{
    isStoped = true;
}

void SortFilterWorker::addDevice(const QList<DeviceInfoPointer> &infoList)
{
    if (isStoped)
        return;

    for (auto &info : infoList) {
        if (contains(allDeviceList, info)) {
            updateDevice(info);
            continue;
        }

        int index = 0;
        switch (info->connectStatus()) {
        case DeviceInfo::Connected:
            index = findLast(DeviceInfo::Connected) + 1;
            break;
        case DeviceInfo::Connectable: {
            int i = findLast(DeviceInfo::Connectable);
            if (i != -1) {
                index = i + 1;
                break;
            }

            i = findFirst(DeviceInfo::Offline);
            if (i != -1) {
                index = i;
                break;
            }

            index = allDeviceList.size();
        } break;
        case DeviceInfo::Offline:
            index = allDeviceList.size();
            break;
        }

        if (isStoped)
            return;

        allDeviceList.insert(index, info);
        visibleDeviceList.insert(index, info);
        Q_EMIT sortFilterResult(index, info);
    }
}

void SortFilterWorker::removeDevice(const QString &ip)
{
    for (int i = 0; i < visibleDeviceList.size(); ++i) {
        if (visibleDeviceList[i]->ipAddress() != ip)
            continue;

        allDeviceList.removeOne(visibleDeviceList[i]);
        visibleDeviceList.removeAt(i);
        Q_EMIT deviceRemoved(i);
        break;
    }
}

void SortFilterWorker::filterDevice(const QString &filter)
{
    visibleDeviceList.clear();
    int index = -1;
    for (const auto &dev : allDeviceList) {
        if (dev->deviceName().contains(filter, Qt::CaseInsensitive)
            || dev->ipAddress().contains(filter, Qt::CaseInsensitive)) {
            ++index;
            visibleDeviceList.append(dev);
            Q_EMIT sortFilterResult(index, dev);
        }
    }

    Q_EMIT filterFinished();
}

void SortFilterWorker::clear()
{
    allDeviceList.clear();
}

int SortFilterWorker::findFirst(DeviceInfo::ConnectStatus state)
{
    int index = -1;
    auto iter = std::find_if(allDeviceList.cbegin(), allDeviceList.cend(),
                             [&](const DeviceInfoPointer info) {
                                 if (isStoped)
                                     return true;
                                 index++;
                                 return info->connectStatus() == state;
                             });

    if (iter == allDeviceList.cend())
        return -1;

    return index;
}

int SortFilterWorker::findLast(DeviceInfo::ConnectStatus state)
{
    int index = allDeviceList.size();
    auto iter = std::find_if(allDeviceList.crbegin(), allDeviceList.crend(),
                             [&](const DeviceInfoPointer info) {
                                 if (isStoped)
                                     return true;
                                 index--;
                                 return info->connectStatus() == state;
                             });

    if (iter == allDeviceList.crend())
        return -1;

    return index;
}

void SortFilterWorker::updateDevice(const DeviceInfoPointer info)
{
    // 更新
    int index = indexOf(allDeviceList, info);
    if (allDeviceList[index]->discoveryMode() == DeviceInfo::DiscoveryMode::NotAllow)
        return removeDevice(allDeviceList[index]->ipAddress());

    if (allDeviceList[index]->deviceName() != info->deviceName()
        || allDeviceList[index]->connectStatus() != info->connectStatus()) {
        allDeviceList.replace(index, info);
    }

    if (!contains(visibleDeviceList, info))
        return;

    index = indexOf(visibleDeviceList, info);
    visibleDeviceList.replace(index, info);
    Q_EMIT deviceReplaced(index, info);
}

bool SortFilterWorker::contains(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info)
{
    auto iter = std::find_if(list.begin(), list.end(),
                             [&info](const DeviceInfoPointer it) {
                                 return it->ipAddress() == info->ipAddress();
                             });

    return iter != list.end();
}

int SortFilterWorker::indexOf(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info)
{
    int index = -1;
    auto iter = std::find_if(list.begin(), list.end(),
                             [&](const DeviceInfoPointer it) {
                                 index++;
                                 return it->ipAddress() == info->ipAddress();
                             });

    if (iter == list.end())
        return -1;

    return index;
}
