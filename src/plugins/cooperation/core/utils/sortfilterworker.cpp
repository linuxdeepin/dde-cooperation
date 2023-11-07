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

void SortFilterWorker::addDevice(const QList<DeviceInfo> &infoList)
{
    if (isStoped)
        return;

    for (auto &info : infoList) {
        if (allDeviceList.contains(info)) {
            updateDevice(info);
            continue;
        }

        int index = 0;
        switch (info.state) {
        case ConnectState::kConnected:
            index = findLast(ConnectState::kConnected) + 1;
            break;
        case ConnectState::kConnectable: {
            int i = findLast(ConnectState::kConnectable);
            if (i != -1) {
                index = i + 1;
                break;
            }

            i = findFirst(ConnectState::kOffline);
            if (i != -1) {
                index = i;
                break;
            }

            index = allDeviceList.size();
        } break;
        case ConnectState::kOffline:
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
        if (visibleDeviceList[i].ipStr != ip)
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
        if (dev.deviceName.contains(filter, Qt::CaseInsensitive)
            || dev.ipStr.contains(filter, Qt::CaseInsensitive)) {
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

int SortFilterWorker::findFirst(ConnectState state)
{
    int index = -1;
    auto iter = std::find_if(allDeviceList.cbegin(), allDeviceList.cend(),
                             [&](const DeviceInfo &info) {
                                 if (isStoped)
                                     return true;
                                 index++;
                                 return info.state == state;
                             });

    if (iter == allDeviceList.cend())
        return -1;

    return index;
}

int SortFilterWorker::findLast(ConnectState state)
{
    int index = allDeviceList.size();
    auto iter = std::find_if(allDeviceList.crbegin(), allDeviceList.crend(),
                             [&](const DeviceInfo &info) {
                                 if (isStoped)
                                     return true;
                                 index--;
                                 return info.state == state;
                             });

    if (iter == allDeviceList.crend())
        return -1;

    return index;
}

void SortFilterWorker::updateDevice(const DeviceInfo &info)
{
    // 更新
    int index = allDeviceList.indexOf(info);
    if (allDeviceList[index].deviceName != info.deviceName
        || allDeviceList[index].state != info.state) {
        allDeviceList.replace(allDeviceList.indexOf(info), info);
    }

    if (!visibleDeviceList.contains(info))
        return;

    index = visibleDeviceList.indexOf(info);
    visibleDeviceList.replace(visibleDeviceList.indexOf(info), info);
    Q_EMIT deviceReplaced(index, info);
}
