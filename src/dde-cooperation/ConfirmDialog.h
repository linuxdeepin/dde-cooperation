// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

class QLabel;

class ConfirmDialog : public DDialog {
    Q_OBJECT

public:
    explicit ConfirmDialog(const QString &ip, const QString &machineName);

signals:
    void onConfirmed(bool accepted);

private:
    void initTimeout();

private:
    QLabel *m_titleLabel;
    QLabel *m_contentLabel;
};

#endif // CONFIRMDIALOG_H
