// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SORTFILTERWORKER_H
#define SORTFILTERWORKER_H

#include "global_defines.h"

#include <QObject>

namespace cooperation_core {

class SortFilterWorker : public QObject
{
    Q_OBJECT
public:
    explicit SortFilterWorker(QObject *parent = nullptr);

    void stop();

public Q_SLOTS:
    void addDevice(const QList<DeviceInfo> &infoList);
    void removeDevice(const QString &ip);
    void filterDevice(const QString &filter);
    void clear();

Q_SIGNALS:
    void sortFilterResult(int index, const DeviceInfo &info);
    void deviceRemoved(int index);
    void deviceReplaced(int index, const DeviceInfo &info);
    void filterFinished();

private:
    int findFirst(ConnectState state);
    int findLast(ConnectState state);
    void updateDevice(const DeviceInfo &info);

private:
    QList<DeviceInfo> visibleDeviceList;
    QList<DeviceInfo> allDeviceList;
    std::atomic_bool isStoped { false };
};

}   // namespace cooperation_core

#endif   // SORTFILTERWORKER_H
