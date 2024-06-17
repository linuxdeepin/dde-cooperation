// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../maincontroller.h"
#include "utils/cooperationutil.h"
#include "configs/settings/configmanager.h"
#include "common/constant.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QDir>

#include <mutex>

using namespace cooperation_core;

void MainController::initConnect()
{
    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &MainController::onAppAttributeChanged);
    connect(CooperationUtil::instance(), &CooperationUtil::discoveryFinished, this, &MainController::onDiscoveryFinished, Qt::QueuedConnection);
}

void MainController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != AppSettings::GenericGroup)
        return;

    if (key == AppSettings::StoragePathKey)
        CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, value.toString());

    regist();
}

void MainController::regist()
{
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, storagePath); });

    auto info = CooperationUtil::deviceInfo();
    info.insert(AppSettings::CooperationEnabled, true);

    auto doc = QJsonDocument::fromVariant(info);
    CooperationUtil::instance()->registAppInfo(doc.toJson());
}

void MainController::unregist()
{
    CooperationUtil::instance()->unregistAppInfo();
}
