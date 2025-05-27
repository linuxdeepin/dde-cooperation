// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "configmanager.h"
#include "settings.h"

#include <QCoreApplication>

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "ConfigManager created";
    init();
}

ConfigManager::~ConfigManager()
{
}

void ConfigManager::init()
{
    qDebug() << "Initializing ConfigManager";
    
    const auto &orgName = qApp->organizationName();
    const auto &appName = qApp->applicationName();
    qDebug() << "Organization:" << orgName << "Application:" << appName;

    QString asCfonigPath = QString("%1/%2/%3").arg(orgName, appName, appName);
    qDebug() << "Config path:" << asCfonigPath;

    appSettings = new Settings(asCfonigPath, Settings::GenericConfig, this);
    appSettings->setAutoSync(true);
    appSettings->setWatchChanges(true);
    qInfo() << "Settings initialized with auto-sync and watch changes enabled";

    appSettings->moveToThread(thread());
    connect(appSettings, &Settings::valueChanged,
            this, &ConfigManager::appAttributeChanged);
    connect(appSettings, &Settings::valueEdited,
            this, &ConfigManager::appAttributeEdited);
    qDebug() << "Signal connections established";

    qInfo() << "ConfigManager initialized successfully";
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
    qDebug() << "Getting ConfigManager instance";
    static ConfigManager ins;
    return &ins;
}

Settings *ConfigManager::appSetting()
{
    qDebug() << "Getting app settings instance";
    return appSettings;
}
