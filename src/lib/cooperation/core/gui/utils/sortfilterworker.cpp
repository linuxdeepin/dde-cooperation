// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sortfilterworker.h"
#include "utils/historymanager.h"
#include "common/log.h"

using TransHistoryInfo = QMap<QString, QString>;
Q_GLOBAL_STATIC(TransHistoryInfo, transHistory)

using namespace cooperation_core;

SortFilterWorker::SortFilterWorker(QObject *parent)
    : QObject(parent)
{
    DLOG << "Initializing worker";
    onTransHistoryUpdated();
    connect(HistoryManager::instance(), &HistoryManager::transHistoryUpdated, this, &SortFilterWorker::onTransHistoryUpdated, Qt::QueuedConnection);
    DLOG << "Initialization completed";
}

void SortFilterWorker::stop()
{
    DLOG << "Stopping worker";
    isStoped = true;
    DLOG << "Worker stopped";
}

void SortFilterWorker::onTransHistoryUpdated()
{
    *transHistory = HistoryManager::instance()->getTransHistory();
}

int SortFilterWorker::calculateIndex(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info)
{
    DLOG << "Calculating index for device";
    int index = 0;
    switch (info->connectStatus()) {
    case DeviceInfo::Connected:
        // 连接中的设备放第一个
        index = 0;
        DLOG << "Connected device placed at index 0";
        break;
    case DeviceInfo::Connectable: {
        DLOG << "Connectable device";
        index = findLast(list, DeviceInfo::Connectable, info);
        if (index != -1) {
            DLOG << "Connectable device found at index:" << index;
            break;
        }

        index = findFirst(list, DeviceInfo::Offline);
        if (index != -1) {
            DLOG << "Connectable device placed before offline devices at index:" << index;
            break;
        }

        index = list.size();
        DLOG << "Connectable device placed at the end of the list:" << index;
    } break;
    case DeviceInfo::Offline:
    default:
        index = list.size();
        DLOG << "Offline device placed at end:" << index;
        break;
    }

    DLOG << "Calculation completed, index:" << index;
    return index;
}

void SortFilterWorker::addDevice(const QList<DeviceInfoPointer> &infoList)
{
    DLOG << "Adding" << infoList.size() << "devices";
    if (isStoped) {
        DLOG << "Worker stopped, skipping add";
        return;
    }

    for (auto info : infoList) {
        if (isStoped) {
            DLOG << "Worker stopped during device addition, returning";
            return;
        }
        if (info->ipAddress() == selfip) {
            DLOG << "Skipping self IP:" << selfip.toStdString();
            continue;
        }

        // 分别进行更新
        if (contains(allDeviceList, info)) {
            DLOG << "Device already in allDeviceList, updating";
            updateDevice(allDeviceList, info, false);
            if (contains(visibleDeviceList, info)) {
                DLOG << "Device also in visibleDeviceList, updating";
                updateDevice(visibleDeviceList, info, true);
            }
            continue;
        }

        if (info->connectStatus() == DeviceInfo::Unknown) {
            DLOG << "Device status is Unknown, setting to Connectable";
            info->setConnectStatus(DeviceInfo::Connectable);
        }

        auto index = calculateIndex(allDeviceList, info);
        allDeviceList.insert(index, info);

        // 判断是否需要过滤
        if (!filterText.isEmpty()) {
            DLOG << "Filter text is not empty:" << filterText.toStdString();
            if (info->deviceName().contains(filterText, Qt::CaseInsensitive)
                || info->ipAddress().contains(filterText, Qt::CaseInsensitive)) {
                DLOG << "Device matches filter, adding to visible list";
                index = calculateIndex(visibleDeviceList, info);
            } else {
                DLOG << "Device does not match filter, skipping";
                continue;
            }
        }

        visibleDeviceList.insert(index, info);
        Q_EMIT sortFilterResult(index, info);
    }

    DLOG << "Added" << infoList.size() << "devices successfully";
    Q_EMIT filterFinished();
}

void SortFilterWorker::removeDevice(const QString &ip)
{
    DLOG << "Removing device with IP:" << ip.toStdString();
    for (int i = 0; i < visibleDeviceList.size(); ++i) {
        if (visibleDeviceList[i]->ipAddress() != ip) {
            continue;
        }

        DLOG << "Found device at index:" << i;
        allDeviceList.removeOne(visibleDeviceList[i]);
        visibleDeviceList.removeAt(i);
        Q_EMIT deviceRemoved(i);
        DLOG << "Device removed successfully";
        break;
    }
}

void SortFilterWorker::filterDevice(const QString &filter)
{
    DLOG << "Filtering devices with text:" << filter.toStdString();
    filterText = filter;
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

    DLOG << "Filtering completed, found" << visibleDeviceList.size() << "devices";
    Q_EMIT filterFinished();
}

void SortFilterWorker::clear()
{
    DLOG << "Clearing all devices";
    allDeviceList.clear();
    visibleDeviceList.clear();
}

int SortFilterWorker::findFirst(const QList<DeviceInfoPointer> &list, DeviceInfo::ConnectStatus state)
{
    // DLOG << "Searching for first device with status:" << (int)state;
    int index = -1;
    auto iter = std::find_if(list.cbegin(), list.cend(),
                             [&](const DeviceInfoPointer info) {
                                 if (isStoped)
                                     return true;
                                 index++;
                                 return info->connectStatus() == state;
                             });

    if (iter == list.cend()) {
        DLOG << "No device found with status:" << (int)state;
        return -1;
    }

    // DLOG << "Found first device with status:" << (int)state << "at index:" << index;
    return index;
}

int SortFilterWorker::findLast(const QList<DeviceInfoPointer> &list, DeviceInfo::ConnectStatus state, const DeviceInfoPointer info)
{
    // DLOG << "Searching for last device with status:" << (int)state;
    bool isRecord = transHistory->contains(info->ipAddress());
    int startPos = -1;
    int endPos = -1;

    for (int i = list.size() - 1; i >= 0; --i) {
        if (list[i]->connectStatus() == state) {
            startPos = (startPos == -1 ? i : startPos);
            endPos = i;

            if (!isRecord)
                return startPos + 1;

            if (transHistory->contains(list[i]->ipAddress()))
                return endPos + 1;
        }
    }

    // DLOG << "Last device with status:" << (int)state << "not found";
    return qMin(startPos, endPos);
}

void SortFilterWorker::updateDevice(QList<DeviceInfoPointer> &list, const DeviceInfoPointer info, bool needNotify)
{
    DLOG << "Updating device with IP:" << info->ipAddress().toStdString();
    int index = indexOf(list, info);

    // 当连接状态不一致时，需要更新位置
    bool needMove = list[index]->connectStatus() != info->connectStatus();
    if (needMove) {
        DLOG << "Moving device from index:" << index;
        list.removeAt(index);
        auto to = calculateIndex(list, info);
        list.insert(to, info);
        DLOG << "Moved device to index:" << to;
        if (needNotify) {
            Q_EMIT deviceMoved(index, to, info);
            DLOG << "Emitted deviceMoved signal";
        }
    } else {
        DLOG << "Updating device at index:" << index;
        list.replace(index, info);
        if (needNotify) {
            Q_EMIT deviceUpdated(index, info);
            DLOG << "Emitted deviceUpdated signal";
        }
    }
}

bool SortFilterWorker::contains(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info)
{
    // DLOG << "Checking if list contains device with IP:" << info->ipAddress().toStdString();
    auto iter = std::find_if(list.begin(), list.end(),
                             [&info](const DeviceInfoPointer it) {
                                 return it->ipAddress() == info->ipAddress();
                             });

    return iter != list.end();
}

int SortFilterWorker::indexOf(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info)
{
    // DLOG << "Searching for index of device with IP:" << info->ipAddress().toStdString();
    int index = -1;
    auto iter = std::find_if(list.begin(), list.end(),
                             [&](const DeviceInfoPointer it) {
                                 index++;
                                 return it->ipAddress() == info->ipAddress();
                             });

    if (iter == list.end()) {
        DLOG << "Device not found in list";
        return -1;
    }

    // DLOG << "Found device at index:" << index << "with IP:" << info->ipAddress().toStdString();
    return index;
}

void SortFilterWorker::setSelfip(const QString &value)
{
    DLOG << "Setting self IP to:" << value.toStdString();
    selfip = value;
}
