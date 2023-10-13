// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONCOREEVENTSENDER_H
#define COOPERATIONCOREEVENTSENDER_H

#include <QObject>

namespace cooperation_core {

class CooperationCoreEventSender : public QObject
{
    Q_OBJECT

public:
    static CooperationCoreEventSender *instance();

public Q_SLOTS:
    void sendRequestRefresh();

private:
    explicit CooperationCoreEventSender(QObject *parent = nullptr);
};

}   // namespace cooperation_core

#endif   // COOPERATIONCOREEVENTSENDER_H
