// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "global_defines.h"

#include <QScopedPointer>

namespace cooperation_core {

class MainWindowPrivate;
class MainWindow : public CooperationMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public Q_SLOTS:
    void onlineStateChanged(bool isOnline);
    void onLookingForDevices();
    void onDevicesFound(const QList<DeviceInfo> &infoList);
    void onRegistOperations(const QVariantMap &map);

private:
    QScopedPointer<MainWindowPrivate> d;
};

}   // namespace cooperation_core

#endif   // MAINWINDOW_H
