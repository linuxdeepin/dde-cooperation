// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../mainwindow.h"
#include "../mainwindow_p.h"
#include "events/cooperationcoreeventsender.h"

#include <DTitlebar>
#include <DIconButton>

#include <QVBoxLayout>
#include <QApplication>

using namespace cooperation_core;
DWIDGET_USE_NAMESPACE

void MainWindowPrivate::initWindow()
{
    q->setObjectName("MainWindow");
    q->setFixedSize(500, 630);
    q->setWindowIcon(QIcon::fromTheme("collaboration"));
}

void MainWindowPrivate::initTitleBar()
{
    auto titleBar = q->titlebar();
    DIconButton *refreshBtn = new DIconButton(q);
    refreshBtn->setIcon(QIcon::fromTheme("refresh"));
    titleBar->addWidget(refreshBtn, Qt::AlignLeft);
    connect(refreshBtn, &DIconButton::clicked, CooperationCoreEventSender::instance(), &CooperationCoreEventSender::sendRequestRefresh);

    if (qAppName() != "dde-cooperation") {
        titleBar->setMenuVisible(false);
        auto margins = titleBar->contentsMargins();
        margins.setLeft(10);
        titleBar->setContentsMargins(margins);
        q->setWindowFlags(q->windowFlags() &~ Qt::WindowMinimizeButtonHint);
        return;
    }

    titleBar->setIcon(QIcon::fromTheme("collaboration"));
    auto menu = titleBar->menu();
    QAction *action = new QAction(tr("Settings"), menu);
    action->setData(MenuAction::kSettings);
    menu->addAction(action);

    action = new QAction(tr("Download Windows client"), menu);
    action->setData(MenuAction::kDownloadWindowClient);
    menu->addAction(action);

    QObject::connect(menu, &QMenu::triggered, [this](QAction *act) {
        bool ok { false };
        int val { act->data().toInt(&ok) };
        if (ok)
            handleSettingMenuTriggered(val);
    });
}
