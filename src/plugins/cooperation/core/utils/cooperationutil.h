// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_H
#define COOPERATIONUTIL_H

#include "global_defines.h"
#include "info/deviceinfo.h"

#include <QWidget>

namespace cooperation_core {

class CooperationUtilPrivate;
class CooperationUtil : public QObject
{
    Q_OBJECT
public:
    static CooperationUtil *instance();

    QWidget *mainWindow();
    QString sessionId() const;
    void destroyMainWindow();
    void registerDeviceOperation(const QVariantMap &map);

    void registAppInfo(const QString &infoJson);
    void unregistAppInfo();
    void asyncDiscoveryDevice();
    void setAppConfig(const QString &key, const QString &value);

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
