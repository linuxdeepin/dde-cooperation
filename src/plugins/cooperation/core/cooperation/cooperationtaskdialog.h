// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONTASKDIALOG_H
#define COOPERATIONTASKDIALOG_H

#include "global_defines.h"

#include <QStackedLayout>

class CooperationTaskDialog : public CooperationDialog
{
    Q_OBJECT
public:
    explicit CooperationTaskDialog(QWidget *parent = nullptr);

    void switchWaitPage(const QString &dev);
    void switchFailPage(const QString &dev, const QString &msg, bool retry);

Q_SIGNALS:
    void waitCanceled();
    void retryConnected();

protected:
    void init();
    void setTaskTitle(const QString &title);
    QWidget *createWaitPage();
    QWidget *createFailPage();

private:
    QStackedLayout *mainLayout { nullptr };

    // fail widget
    QLabel *msgLabel { nullptr };
    QPushButton *cancelBtn { nullptr };
    QPushButton *retryBtn { nullptr };
};

#endif   // COOPERATIONTASKDIALOG_H
