// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dconfigmanager.h"
#include "dconfigmanager_p.h"

#include <DConfig>

#include <QDebug>
#include <QSet>

static constexpr char kCfgAppId[] { "org.deepin.dde.cooperation" };

DCORE_USE_NAMESPACE

DConfigManager::DConfigManager(QObject *parent)
    : QObject(parent), d(new DConfigManagerPrivate(this))
{
    addConfig(kDefaultCfgPath);
}

DConfigManager *DConfigManager::instance()
{
    qDebug() << "Getting DConfigManager instance";
    static DConfigManager ins;
    return &ins;
}

DConfigManager::~DConfigManager()
{
    qDebug() << "Destroying DConfigManager instance";
#ifdef DTKCORE_CLASS_DConfig
    QWriteLocker locker(&d->lock);

    auto configs = d->configs.values();
    qDebug() << "Cleaning up" << configs.size() << "configurations";
    std::for_each(configs.begin(), configs.end(), [](DConfig *cfg) { delete cfg; });
    d->configs.clear();
#else
    qDebug() << "DTKCORE_CLASS_DConfig not defined, skipping cleanup";
#endif
}

bool DConfigManager::addConfig(const QString &config, QString *err)
{
    qDebug() << "Adding config:" << config;
#ifdef DTKCORE_CLASS_DConfig
    QWriteLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        qWarning() << "Config already exists:" << config;
        if (err)
            *err = "config is already added";
        return false;
    }

    auto cfg = DConfig::create(kCfgAppId, config, "", this);
    if (!cfg) {
        qCritical() << "Failed to create config:" << config;
        if (err)
            *err = "cannot create config";
        return false;
    }

    if (!cfg->isValid()) {
        qCritical() << "Config is not valid:" << config;
        if (err)
            *err = "config is not valid";
        delete cfg;
        return false;
    }

    d->configs.insert(config, cfg);
    locker.unlock();
    connect(cfg, &DConfig::valueChanged, this, [=](const QString &key) { Q_EMIT valueChanged(config, key); });
#else
    qDebug() << "DTKCORE_CLASS_DConfig not defined, skipping addConfig";
#endif
    return true;
}

bool DConfigManager::removeConfig(const QString &config, QString *err)
{
    qDebug() << "Removing config:" << config;
    Q_UNUSED(err)

#ifdef DTKCORE_CLASS_DConfig
    QWriteLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        delete d->configs[config];
        d->configs.remove(config);
        qInfo() << "Successfully removed config:" << config;
    } else {
        qWarning() << "Config not found:" << config;
    }
#else
    qDebug() << "DTKCORE_CLASS_DConfig not defined, skipping removeConfig";
#endif
    return true;
}

QStringList DConfigManager::keys(const QString &config) const
{
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    if (!d->configs.contains(config)) {
        qWarning() << "Config not found:" << config;
        return QStringList();
    }

    return d->configs[config]->keyList();
#else
    qDebug() << "DTKCORE_CLASS_DConfig not defined, returning empty QStringList";
    return QStringList();
#endif
}

bool DConfigManager::contains(const QString &config, const QString &key) const
{
    if (key.isEmpty()) {
        qWarning() << "Key is empty, returning false";
        return false;
    }
    return keys(config).contains(key);
}

QVariant DConfigManager::value(const QString &config, const QString &key, const QVariant &fallback) const
{
    qDebug() << "Getting value for config:" << config << "key:" << key;
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    if (d->configs.contains(config))
        return d->configs.value(config)->value(key, fallback);
    else
        qWarning() << "Config: " << config << "is not registered!!!";
    return fallback;
#else
    qDebug() << "DConfig not supported, using fallback value:" << fallback;
    return fallback;
#endif
}

void DConfigManager::setValue(const QString &config, const QString &key, const QVariant &value)
{
    qDebug() << "Setting value for config:" << config << "key:" << key << "value:" << value;
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    if (d->configs.contains(config)) {
        d->configs.value(config)->setValue(key, value);
    } else {
        qWarning() << "Config: " << config << "is not registered!!!";
    }
#else
    qDebug() << "DTKCORE_CLASS_DConfig not defined, skipping setValue";
#endif
}

bool DConfigManager::validateConfigs(QStringList &invalidConfigs) const
{
#ifdef DTKCORE_CLASS_DConfig
    QReadLocker locker(&d->lock);

    bool ret = true;
    for (auto iter = d->configs.cbegin(); iter != d->configs.cend(); ++iter) {
        bool valid = iter.value()->isValid();
        if (!valid) {
            qWarning() << "Config is invalid:" << iter.key();
            invalidConfigs << iter.key();
        }
        ret &= valid;
    }
    return ret;
#else
    qDebug() << "DTKCORE_CLASS_DConfig not defined, returning true";
    return true;
#endif
}
