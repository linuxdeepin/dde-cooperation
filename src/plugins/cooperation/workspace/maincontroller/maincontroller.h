// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QNetworkConfigurationManager>

namespace cooperation_workspace {

class WorkspaceWidget;
class MainController : public QObject
{
    Q_OBJECT
public:
    static MainController *instance();

    QWidget *widget();
    void start();
    void stop();

    void registerDeviceOperation(const QVariantMap &map);

private Q_SLOTS:
    void onlineStateChanged(bool isOnline);

private:
    explicit MainController(QObject *parent = nullptr);
    void initConnect();

private:
    WorkspaceWidget *w { nullptr };
    QNetworkConfigurationManager *networkMgr { nullptr };

    bool isRuning = false;
};

}   // namespace cooperation_workspace

#endif   // MAINCONTROLLER_H
