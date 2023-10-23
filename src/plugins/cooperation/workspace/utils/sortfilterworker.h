// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SORTFILTERWORKER_H
#define SORTFILTERWORKER_H

#include "global_defines.h"

#include <QObject>

class SortFilterWorker : public QObject
{
    Q_OBJECT
public:
    explicit SortFilterWorker(QObject *parent = nullptr);

    void stop();

public Q_SLOTS:
    void sortDevice(const DeviceInfo &info);
    void filterDevice(const QString &filter);
    void clear();

Q_SIGNALS:
    void sortFilterResult(int index, const DeviceInfo &info);
    void filterFinished();

private:
    int findFirst(ConnectState state);
    int findLast(ConnectState state);

private:
    QList<DeviceInfo> allDeviceList;
    std::atomic_bool isStoped { false };
};

#endif   // SORTFILTERWORKER_H
