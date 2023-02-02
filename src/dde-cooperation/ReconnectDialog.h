// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDE_COOPERATION_RECONNECTDIALOG_H
#define DDE_COOPERATION_RECONNECTDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class ReconnectDialog : public DDialog {
    Q_OBJECT

public:
    explicit ReconnectDialog(const QString &machineName);

signals:
    void onOperated(bool tryAgain);
};

#endif // DDE_COOPERATION_RECONNECTDIALOG_H
