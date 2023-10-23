// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_H
#define COOPERATIONUTIL_H

#include <QWidget>

namespace cooperation_core {

class MainWindow;
class CooperationUtil
{
public:
    static CooperationUtil *instance();

    QWidget *mainWindow();
    void destroyMainWindow();
    void registWorkspace(QWidget *workspace);

private:
    explicit CooperationUtil();
    ~CooperationUtil();

private:
    MainWindow *window { nullptr };
};

}   // namespace cooperation_core

#endif   // COOPERATIONUTIL_H
