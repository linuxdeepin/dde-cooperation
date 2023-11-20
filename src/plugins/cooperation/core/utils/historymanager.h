// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include "global_defines.h"

namespace cooperation_core {

class HistoryManager
{
public:
    static HistoryManager *instance();

    QMap<QString, QString> getTransHistory();
    void writeIntoTransHistory(const QString &ip, const QString &savePath);

private:
    explicit HistoryManager();
};

}   // namespace cooperation_core

#endif   // HISTORYMANAGER_H
