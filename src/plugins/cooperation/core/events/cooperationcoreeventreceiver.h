// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONCOREEVENTRECEIVER_H
#define COOPERATIONCOREEVENTRECEIVER_H

#include <QObject>

#include <functional>

namespace cooperation_core {

class CooperationCoreEventReceiver : public QObject
{
    Q_OBJECT

public:
    static CooperationCoreEventReceiver *instance();

public Q_SLOTS:
    void handleRegisterWorkspace(QVariant param);

private:
    explicit CooperationCoreEventReceiver(QObject *parent = nullptr);
};

}   // namespace cooperation_core

#endif   // COOPERATIONCOREEVENTRECEIVER_H
