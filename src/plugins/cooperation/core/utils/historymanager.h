// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "global_defines.h"

namespace cooperation_core {

class HistoryManager : public QObject
{
    Q_OBJECT
public:
    static HistoryManager *instance();

    QMap<QString, QString> getTransHistory();
    void writeIntoTransHistory(const QString &ip, const QString &savePath);
    void removeTransHistory(const QString &ip);

Q_SIGNALS:
    void transHistoryUpdated();

private:
    explicit HistoryManager(QObject *parent = nullptr);
    void onAttributeChanged(const QString &group, const QString &key, const QVariant &value);
};

}   // namespace cooperation_core

#endif   // HISTORYMANAGER_H
