// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
