// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_H
#define COOPERATIONUTIL_H

#include "global_defines.h"
#include "discover/deviceinfo.h"

#include <QWidget>
#include <QDialog>

namespace cooperation_core {

class CooperationUtilPrivate;
class CooperationUtil : public QObject
{
    Q_OBJECT
public:
    static CooperationUtil *instance();

    QWidget *mainWindow();

    DeviceInfoPointer findDeviceInfo(const QString &ip);

    void destroyMainWindow();

    void registerDeviceOperation(const QVariantMap &map);

    void setStorageConfig(const QString &value);

    void showFeatureDisplayDialog(QDialog *dlg);

    void initNetworkListener();

    void initHistory();

    void showCloseDialog();

    void minimizedAPP();

    static QVariantMap deviceInfo();
    static QString localIPAddress();
    static QString barrierProfile();

Q_SIGNALS:
    void onlineStateChanged(const QString &validIP);

private:
    explicit CooperationUtil(QObject *parent = nullptr);
    ~CooperationUtil();

    void checkNetworkState();

private:
    QSharedPointer<CooperationUtilPrivate> d { nullptr };
};

}   // namespace cooperation_core

#endif   // COOPERATIONUTIL_H
