// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "configmanager.h"
#include "settings.h"

#include <QCoreApplication>
#include <QDebug>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
    qInfo() << "ConfigManager init";
    init();
}

ConfigManager::~ConfigManager()
{
    qInfo() << "ConfigManager destroy";
}

void ConfigManager::init()
{
    qInfo() << "ConfigManager init";
    const auto &orgName = qApp->organizationName();
    const auto &appName = qApp->applicationName();

    QString asCfonigPath = QString("%1/%2/%3").arg(orgName, appName, appName);
    appSettings = new Settings(asCfonigPath, Settings::GenericConfig, this);
    appSettings->setAutoSync(true);
    appSettings->setWatchChanges(true);

    appSettings->moveToThread(thread());
    connect(appSettings, &Settings::valueChanged,
            this, &ConfigManager::appAttributeChanged);
    connect(appSettings, &Settings::valueEdited,
            this, &ConfigManager::appAttributeEdited);
    qInfo() << "ConfigManager init end";
}

QVariant ConfigManager::appAttribute(const QString &group, const QString &key)
{
    return appSetting()->value(group, key);
}

void ConfigManager::setAppAttribute(const QString &group, const QString &key, const QVariant &value)
{
    appSetting()->setValue(group, key, value);
}

bool ConfigManager::syncAppAttribute()
{
    return appSetting()->sync();
}

ConfigManager *ConfigManager::instance()
{
    static ConfigManager ins;
    return &ins;
}

Settings *ConfigManager::appSetting()
{
    return appSettings;
}
