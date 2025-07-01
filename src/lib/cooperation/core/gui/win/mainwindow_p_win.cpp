// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../mainwindow.h"
#include "../mainwindow_p.h"
#include "common/log.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QToolButton>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QPainterPath>
#include <QStackedLayout>

#include <gui/widgets/cooperationstatewidget.h>

using namespace cooperation_core;
void MainWindowPrivate::initWindow()
{
    DLOG << "Initializing window";
    q->setObjectName("MainWindow");
    q->setFixedSize(500, 630);

    q->setWindowIcon(QIcon::fromTheme(":/icons/deepin/builtin/icons/dde-cooperation_128px.png"));

    workspaceWidget = new WorkspaceWidget(q);

    QWidget *centralWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    bottomLabel = new BottomLabel(q);
    mainLayout->addWidget(workspaceWidget);
    mainLayout->addWidget(bottomLabel);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    centralWidget->setLayout(mainLayout);
    q->setCentralWidget(centralWidget);
    DLOG << "Central widget setup completed";

    workspaceWidget->setStyleSheet("background-color: rgba(230,230,230, 0.1);"
                                   "border-bottom-right-radius: 10px;"
                                   "border-bottom-left-radius: 10px;");
}

void MainWindowPrivate::paintEvent(QPaintEvent *event)
{
    QPainter painter(q);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(230, 230, 230));
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRoundedRect(q->rect(), 10, 10);
    painter.drawPath(path);
}

void MainWindowPrivate::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() == Qt::LeftButton) && leftButtonPressed) {
        DLOG << "Mouse moving with left button pressed";
        q->move(event->globalPos() - lastPosition);
    }
}

void MainWindowPrivate::mouseReleaseEvent(QMouseEvent *event)
{
    leftButtonPressed = false;
}

void MainWindowPrivate::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        DLOG << "Left mouse button pressed";
        leftButtonPressed = true;
        lastPosition = event->globalPos() - q->pos();
    }
}

void MainWindowPrivate::initTitleBar()
{
    DLOG << "Initializing title bar";
    q->setWindowFlags(Qt::FramelessWindowHint);
    q->setAttribute(Qt::WA_TranslucentBackground);
    QWidget *titleBar = new QWidget(q->centralWidget());
    titleBar->setFixedSize(500, 50);
    titleBar->setStyleSheet("QWidget {"
                            "background-color: white;"
                            "border-top-right-radius: 10px;"
                            "border-top-left-radius: 10px;"
                            "}");
    titleBar->setFocusPolicy(Qt::ClickFocus);

    QToolButton *closeButton = new QToolButton(titleBar);
    closeButton->setStyleSheet("QWidget {"
                               "border-top-right-radius: 20px;"
                               "}");
    closeButton->setFixedSize(50, 50);
    closeButton->setIcon(QIcon(":/icons/deepin/builtin/icons/close_normal.svg"));
    closeButton->setIconSize(QSize(50, 50));

    QToolButton *minButton = new QToolButton(titleBar);
    minButton->setIcon(QIcon(":/icons/deepin/builtin/icons/minimise_normal.svg"));
    minButton->setFixedSize(50, 50);
    minButton->setIconSize(QSize(50, 50));

    QToolButton *helpButton = new QToolButton(titleBar);
    helpButton->setFixedSize(50, 50);
    helpButton->setIcon(QIcon(":/icons/deepin/builtin/icons/menu_normal.svg"));
    helpButton->setIconSize(QSize(50, 50));

    QLabel *iconLabel = new QLabel(titleBar);
    iconLabel->setStyleSheet("QWidget {"
                             "border-top-left-radius: 20px;"
                             "}");
    iconLabel->setPixmap(
            QIcon(":/icons/deepin/builtin/icons/dde-cooperation_128px.png").pixmap(35, 35));

    QMenu *menu = new QMenu();
    helpButton->setMenu(menu);
    helpButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    helpButton->setPopupMode(QToolButton::InstantPopup);
    helpButton->setStyleSheet("QToolButton::menu-indicator {"
                              "image: none;"
                              "}");

    QAction *action = new QAction(tr("Settings"), menu);
    action->setData(MenuAction::kSettings);
    menu->addAction(action);

    action = new QAction(tr("Download Windows client"), menu);
    action->setData(MenuAction::kDownloadWindowClient);
    menu->addAction(action);

    QObject::connect(menu, &QMenu::triggered, [this](QAction *act) {
        DLOG << "Menu triggered";
        bool ok{ false };
        int val{ act->data().toInt(&ok) };
        if (ok) {
            DLOG << "Handling setting menu triggered with value:" << val;
            handleSettingMenuTriggered(val);
        } else {
            DLOG << "Failed to convert action data to integer";
        }
    });

    QObject::connect(closeButton, &QToolButton::clicked, q, &MainWindow::close);
    QObject::connect(minButton, &QToolButton::clicked, q, &MainWindow::showMinimized);

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);

    titleLayout->addWidget(iconLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(helpButton, Qt::AlignHCenter);
    titleLayout->addWidget(minButton, Qt::AlignHCenter);
    titleLayout->addWidget(closeButton, Qt::AlignHCenter);
    titleLayout->setContentsMargins(8, 0, 0, 0);
    titleBar->setLayout(titleLayout);
    DLOG << "Title bar layout completed";

    q->layout()->addWidget(titleBar);
    DLOG << "Title bar initialization completed";
}
