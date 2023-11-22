// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#include <QScopedPointer>

namespace cooperation_core {

class MainWindowPrivate;
class MainWindow : public CooperationMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

#ifdef WIN32
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
#endif

public Q_SLOTS:
    void onlineStateChanged(bool isOnline);
    void onLookingForDevices();
    void onDiscoveryFinished(bool hasFound);
    void addDevice(const QList<DeviceInfoPointer> &infoList);
    void removeDevice(const QString &ip);
    void onRegistOperations(const QVariantMap &map);

private:
    QScopedPointer<MainWindowPrivate> d;
};

}   // namespace cooperation_core

#endif   // MAINWINDOW_H
