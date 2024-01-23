// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERDIALOG_H
#define TRANSFERDIALOG_H

#include "global_defines.h"

#include <QStackedLayout>
#include <QPushButton>
#include <QProgressBar>

namespace cooperation_core {

class TransferDialog : public CooperationDialog
{
    Q_OBJECT

public:
    explicit TransferDialog(QWidget *parent = nullptr);

    void switchWaitConfirmPage();
    void switchResultPage(bool success, const QString &msg);
    void switchProgressPage(const QString &title);

public Q_SLOTS:
    void updateProgress(int value, const QString &remainTime);

Q_SIGNALS:
    void cancel();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void initUI();
    void createWaitConfirmPage();
    void createResultPage();
    void createProgressPage();

private:
    QStackedLayout *stackedLayout { nullptr };

    QPushButton *okBtn { nullptr };
    CooperationSpinner *spinner { nullptr };
    CooperationLabel *iconLabel { nullptr };
    CooperationLabel *msgLabel { nullptr };
    CooperationLabel *titleLabel { nullptr };
    CooperationLabel *progressMsgLael { nullptr };
    QProgressBar *progressBar { nullptr };
};

}   // namespace cooperation_core

#endif   // TRANSFERDIALOG_H
