// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "gui/mainwindow.h"

using namespace cooperation_core;

CooperationUtil::CooperationUtil()
{
}

CooperationUtil::~CooperationUtil()
{
}

CooperationUtil *CooperationUtil::instance()
{
    static CooperationUtil ins;
    return &ins;
}

QWidget *CooperationUtil::mainWindow()
{
    if (!window)
        window = new MainWindow;

    return window;
}

void CooperationUtil::destroyMainWindow()
{
    if (window)
        delete window;
}

void CooperationUtil::registWorkspace(QWidget *workspace)
{
    if (!workspace)
        return;

    window->setCentralWidget(workspace);
}
