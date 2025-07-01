// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../mainwindow.h"
#include "../mainwindow_p.h"
#include "common/log.h"

#include <DTitlebar>
#include <DIconButton>
#include <DButtonBox>

#include <QVBoxLayout>
#include <QApplication>
#include <QStackedLayout>

#include <gui/widgets/cooperationstatewidget.h>
#include <gui/widgets/devicelistwidget.h>

#include <gui/mainwindow_p.h>

using namespace cooperation_core;
DWIDGET_USE_NAMESPACE

void MainWindowPrivate::initWindow()
{
    DLOG << "Enter initWindow() - Initializing main window components";
    q->setObjectName("MainWindow");
    q->setFixedSize(500, 630);
    q->setWindowIcon(QIcon::fromTheme("dde-cooperation"));

    workspaceWidget = new WorkspaceWidget(q);

    stackedLayout = new QStackedLayout;
    stackedLayout->addWidget(workspaceWidget);
#ifdef ENABLE_PHONE
    DLOG << "Phone support enabled, creating PhoneWidget";
    phoneWidget = new PhoneWidget(q);
    stackedLayout->addWidget(phoneWidget);
#endif
    stackedLayout->setCurrentIndex(0);

    QWidget *centralWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    bottomLabel = new BottomLabel(q);
    mainLayout->addLayout(stackedLayout);
    mainLayout->addWidget(bottomLabel);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget->setLayout(mainLayout);

    q->setCentralWidget(centralWidget);
    DLOG << "Exit initWindow() - Main window initialized successfully";
}

void MainWindowPrivate::initTitleBar()
{
    DLOG << "Enter initTitleBar() - Initializing title bar";
    auto titleBar = q->titlebar();
#ifdef ENABLE_PHONE
    DLOG << "Phone support enabled, creating switch button";
    DButtonBox *switchBtn = new DButtonBox(q);
    QList<DButtonBoxButton *> list;
    DButtonBoxButton *PCBtn = new DButtonBoxButton(tr("Computer"));
    DButtonBoxButton *mobileBtn = new DButtonBoxButton(tr("Phone"));
    list.append(PCBtn);
    list.append(mobileBtn);
    switchBtn->setButtonList(list, true);
    titleBar->addWidget(switchBtn, Qt::AlignCenter);
    PCBtn->setChecked(true);
    connect(PCBtn, &DButtonBoxButton::clicked, q, [this] { q->onSwitchMode(CooperationMode::kPC); });
    connect(mobileBtn, &DButtonBoxButton::clicked, q, [this] { q->onSwitchMode(CooperationMode::kMobile); });
#endif
    if (qApp->property("onlyTransfer").toBool()) {
        DLOG << "onlyTransfer property is true, hiding menu and setting title";
        titleBar->setMenuVisible(false);
        titleBar->addWidget(new QLabel(tr("Selection of delivery device")), Qt::AlignHCenter);
        auto margins = titleBar->contentsMargins();
        margins.setLeft(10);
        titleBar->setContentsMargins(margins);
        q->setWindowFlags(q->windowFlags() & ~Qt::WindowMinimizeButtonHint);
        return;
    }

    titleBar->setIcon(QIcon::fromTheme("dde-cooperation"));
    auto menu = titleBar->menu();
    QAction *action = new QAction(tr("Settings"), menu);
    action->setData(MenuAction::kSettings);
    menu->addAction(action);

    action = new QAction(tr("Download client"), menu);
    action->setData(MenuAction::kDownloadWindowClient);
    menu->addAction(action);

    QObject::connect(menu, &QMenu::triggered, [this](QAction *act) {
        bool ok { false };
        int val { act->data().toInt(&ok) };
        if (ok) {
            DLOG << "Handling setting menu triggered with value: " << val;
            handleSettingMenuTriggered(val);
        } else {
            DLOG << "Failed to convert action data to integer";
        }
    });
    DLOG << "Exit initTitleBar() - Title bar initialized successfully";
}
