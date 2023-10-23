// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "type_defines.h"

#include <QScopedPointer>

namespace cooperation_core {

class MainWindowPrivate;
class MainWindow : public CooperationMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QScopedPointer<MainWindowPrivate> d;
};

}   // namespace cooperation_core

#endif   // MAINWINDOW_H
