// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "global_defines.h"
#include "historymanager.h"
#include "configs/settings/configmanager.h"
#include "common/log.h"

using namespace cooperation_core;

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent)
{
    DLOG << "HistoryManager constructor";
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &HistoryManager::onAttributeChanged);
    DLOG << "HistoryManager initialized";
}

HistoryManager *HistoryManager::instance()
{
    static HistoryManager ins;
    return &ins;
}

void HistoryManager::onAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    Q_UNUSED(value)

    if (group != AppSettings::CacheGroup) {
        DLOG << "Group is not CacheGroup, returning";
        return;
    }

    if (key == AppSettings::TransHistoryKey) {
        DLOG << "Transfer history updated";
        Q_EMIT transHistoryUpdated();
        return;
    }

    if (key == AppSettings::ConnectHistoryKey) {
        DLOG << "Connection history updated";
        Q_EMIT connectHistoryUpdated();
    } else {
        DLOG << "Unknown key:" << key.toStdString();
    }
}

QMap<QString, QString> HistoryManager::getTransHistory()
{
    DLOG << "Getting transfer history";
    QMap<QString, QString> dataMap;

    if (qApp->property("onlyTransfer").toBool()) {
        DLOG << "In transfer-only mode, skipping transfer history";
        return dataMap;
    }
    const auto &list = ConfigManager::instance()->appAttribute(AppSettings::CacheGroup, AppSettings::TransHistoryKey).toList();
    DLOG << "Found" << list.size() << "transfer history entries";

    for (const auto &item : list) {
        const auto &map = item.toMap();
        const auto &ip = map.value("ip").toString();
        const auto &path = map.value("savePath").toString();
        
        if (ip.isEmpty() || path.isEmpty()) {
            DLOG << "Skipping invalid transfer history entry";
            continue;
        }

        DLOG << "Adding transfer history entry - ip:" << ip.toStdString() << "path:" << path.toStdString();
        dataMap.insert(ip, path);
    }

    DLOG << "Returning" << dataMap.size() << "valid transfer history entries";
    return dataMap;
}

void HistoryManager::refreshHistory(bool found)
{
    DLOG << "Refreshing history, found:" << found;
    if (!found) {
        DLOG << "No devices found, returning";
        return;
    }
    auto connectHistory = getConnectHistory();

    if (!connectHistory.isEmpty()) {
        DLOG << "Connection history is not empty, emitting historyConnected";
        emit historyConnected(connectHistory);
    }
}

void HistoryManager::writeIntoTransHistory(const QString &ip, const QString &savePath)
{
    DLOG << "Writing into transfer history, ip:" << ip.toStdString() << "path:" << savePath.toStdString();
    auto history = getTransHistory();
    if (history.contains(ip) && history.value(ip) == savePath)
        return;

    history.insert(ip, savePath);
    QVariantList list;
    auto iter = history.begin();
    while (iter != history.end()) {
        QVariantMap map;
        map.insert("ip", iter.key());
        map.insert("savePath", iter.value());

        list << map;
        ++iter;
    }

    ConfigManager::instance()->setAppAttribute(AppSettings::CacheGroup, AppSettings::TransHistoryKey, list);
}

void HistoryManager::removeTransHistory(const QString &ip)
{
    DLOG << "Removing from transfer history, ip:" << ip.toStdString();
    auto history = getTransHistory();

    if (history.remove(ip) == 0) {
        DLOG << "IP not found in transfer history, nothing to remove";
        return;
    }

    DLOG << "IP removed from transfer history, updating config";
    QVariantList list;
    auto iter = history.begin();
    while (iter != history.end()) {
        QVariantMap map;
        map.insert("ip", iter.key());
        map.insert("savePath", iter.value());

        list << map;
        ++iter;
    }

    ConfigManager::instance()->setAppAttribute(AppSettings::CacheGroup, AppSettings::TransHistoryKey, list);
    DLOG << "Transfer history updated in config";
}

QMap<QString, QString> HistoryManager::getConnectHistory()
{
    DLOG << "Getting connection history";
    QMap<QString, QString> dataMap;

    if (qApp->property("onlyTransfer").toBool()) {
        DLOG << "In transfer-only mode, skipping connection history";
        return dataMap;
    }
    const auto &list = ConfigManager::instance()->appAttribute(AppSettings::CacheGroup, AppSettings::ConnectHistoryKey).toList();
    DLOG << "Found" << list.size() << "connection history entries";

    for (const auto &item : list) {
        const auto &map = item.toMap();
        const auto &ip = map.value("ip").toString();
        const auto &devName = map.value("devName").toString();
        
        if (ip.isEmpty() || devName.isEmpty()) {
            DLOG << "Skipping invalid connection history entry";
            continue;
        }

        DLOG << "Adding connection history entry - ip:" << ip.toStdString() << "device:" << devName.toStdString();
        dataMap.insert(ip, devName);
    }

    DLOG << "Returning" << dataMap.size() << "valid connection history entries";
    return dataMap;
}

void HistoryManager::writeIntoConnectHistory(const QString &ip, const QString &devName)
{
    DLOG << "Writing into connection history, ip:" << ip.toStdString() << "device:" << devName.toStdString();
    auto history = getConnectHistory();

    if (history.contains(ip) && history.value(ip) == devName) {
        DLOG << "Connection history already contains same entry, skipping update";
        return;
    }

    DLOG << "Adding new connection history entry";
    history.insert(ip, devName);
    QVariantList list;
    auto iter = history.begin();
    while (iter != history.end()) {
        QVariantMap map;
        map.insert("ip", iter.key());
        map.insert("devName", iter.value());

        list << map;
        ++iter;
    }

    ConfigManager::instance()->setAppAttribute(AppSettings::CacheGroup, AppSettings::ConnectHistoryKey, list);
    DLOG << "Connection history updated in config";
}
