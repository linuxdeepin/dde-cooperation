// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SORTFILTERWORKER_H
#define SORTFILTERWORKER_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#include <QObject>

namespace cooperation_core {

class SortFilterWorker : public QObject
{
    Q_OBJECT
public:
    explicit SortFilterWorker(QObject *parent = nullptr);

    void stop();

public Q_SLOTS:
    void addDevice(const QList<DeviceInfoPointer> &infoList);
    void removeDevice(const QString &ip);
    void filterDevice(const QString &filter);
    void clear();

Q_SIGNALS:
    void sortFilterResult(int index, const DeviceInfoPointer info);
    void deviceRemoved(int index);
    void deviceUpdated(int index, const DeviceInfoPointer info);
    void filterFinished();

private:
    int findFirst(DeviceInfo::ConnectStatus state);
    int findLast(DeviceInfo::ConnectStatus state);
    void updateDevice(const DeviceInfoPointer info);
    bool contains(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info);
    int indexOf(const QList<DeviceInfoPointer> &list, const DeviceInfoPointer info);

private:
    QList<DeviceInfoPointer> visibleDeviceList;
    QList<DeviceInfoPointer> allDeviceList;
    std::atomic_bool isStoped { false };
};

}   // namespace cooperation_core

#endif   // SORTFILTERWORKER_H
