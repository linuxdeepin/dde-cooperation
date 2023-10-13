// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

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
    void moveCenter();

    void handleSettingMenuTriggered(int action);

public:
    MainWindow *q { nullptr };
};

}   // namespace cooperation_core

#endif   // MAINWINDOW_P_H
