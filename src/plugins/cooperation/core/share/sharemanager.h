// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHAREMANAGER_H
#define SHAREMANAGER_H

#include "info/deviceinfo.h"

namespace cooperation_core {

class ShareManager : public QObject
{
    Q_OBJECT

public:
    static ShareManager *instance();

    void regist();

    static void buttonClicked(const QString &id, const DeviceInfoPointer info);
    static bool buttonVisible(const QString &id, const DeviceInfoPointer info);

private:
    explicit ShareManager(QObject *parent = nullptr);
};

}   // namespace cooperation_core

#endif   // SHAREMANAGER_H
