// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_H
#define COOPERATIONUTIL_H

#include "global_defines.h"
#include "info/deviceinfo.h"

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

    void pingTarget(const QString &ip);
    void sendTransApply(const QString &ip);
    void sendShareEvents(int type, const QString &jsonMsg);

    void registAppInfo(const QString &infoJson);
    void unregistAppInfo();
    void asyncDiscoveryDevice();
    void setAppConfig(const QString &key, const QString &value);

    QString deviceInfoStr();
    DeviceInfoPointer parseDeviceInfo(const QString &info);

    static QVariantMap deviceInfo();
    static QString localIPAddress();

    void showFeatureDisplayDialog(QDialog *dlg);

Q_SIGNALS:
    void discoveryFinished(const QList<DeviceInfoPointer> &infoList);

private:
    explicit CooperationUtil(QObject *parent = nullptr);
    ~CooperationUtil();

private:
    QSharedPointer<CooperationUtilPrivate> d { nullptr };
};

}   // namespace cooperation_core

#endif   // COOPERATIONUTIL_H
