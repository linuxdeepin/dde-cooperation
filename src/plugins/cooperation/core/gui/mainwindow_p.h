// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include "widgets/workspacewidget.h"

#include <QObject>

namespace cooperation_core {

class MainWindow;
class MainWindowPrivate : public QObject
{
    Q_OBJECT
public:
    explicit MainWindowPrivate(MainWindow *qq);
    virtual ~MainWindowPrivate();

    void initWindow();
    void initTitleBar();
    void initConnect();
    void moveCenter();

    void handleSettingMenuTriggered(int action);

public:
    MainWindow *q { nullptr };
    WorkspaceWidget *workspaceWidget { nullptr };
};

}   // namespace cooperation_core

#endif   // MAINWINDOW_P_H
