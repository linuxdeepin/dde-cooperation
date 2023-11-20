// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "global_defines.h"
#include "historymanager.h"

#include "config/configmanager.h"

using namespace cooperation_core;

HistoryManager::HistoryManager()
{
}

HistoryManager *HistoryManager::instance()
{
    static HistoryManager ins;
    return &ins;
}

QMap<QString, QString> HistoryManager::getTransHistory()
{
    QMap<QString, QString> dataMap;

    const auto &list = ConfigManager::instance()->appAttribute(AppSettings::CacheGroup, AppSettings::TransHistoryKey).toList();
    for (const auto &item : list) {
        const auto &map = item.toMap();
        const auto &ip = map.value("ip").toString();
        const auto &path = map.value("savePath").toString();
        if (ip.isEmpty() || path.isEmpty())
            continue;

        dataMap.insert(ip, path);
    }

    return dataMap;
}

void HistoryManager::writeIntoTransHistory(const QString &ip, const QString &savePath)
{
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
