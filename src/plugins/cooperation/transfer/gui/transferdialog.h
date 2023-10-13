// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERDIALOG_H
#define TRANSFERDIALOG_H

#include "global_defines.h"

#include <QStackedLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

namespace cooperation_transfer {

class TransferDialog : public CooperationDialog
{
public:
    explicit TransferDialog(QWidget *parent = nullptr);

    void switchWaitConfirmPage();
    void switchResultPage(bool success, const QString &msg);
    void switchProgressPage(const QString &title);

public Q_SLOTS:
    void updateProgress(int value, const QString &msg);

private:
    void initUI();
    void createWaitConfirmPage();
    void createResultPage();
    void createProgressPage();

private:
    QStackedLayout *stackedLayout { nullptr };

    QPushButton *okBtn { nullptr };
    CooperationSpinner *spinner { nullptr };
    QLabel *iconLabel { nullptr };
    QLabel *msgLabel { nullptr };
    QLabel *titleLabel { nullptr };
    QLabel *progressMsgLael { nullptr };
    QProgressBar *progressBar { nullptr };
};

}   // namespace cooperation_transfer

#endif   // TRANSFERDIALOG_H
