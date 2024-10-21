// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenmirroringwindow.h"
#include "global_defines.h"
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>
#include <DTitlebar>

#include <QVBoxLayout>
#include <QStackedLayout>
using namespace cooperation_core;

inline constexpr char KIcon[] { ":/icons/deepin/builtin/icons/uos_assistant.png" };

ScreenMirroringWindow::ScreenMirroringWindow(const QString &device, QWidget *parent)
    : CooperationMainWindow(parent)
{
    initTitleBar(device);
    initWorkWidget();
    initBottom();
    resize(360, 800);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    mainLayout->addLayout(stackedLayout);
    mainLayout->addWidget(bottomWidget, Qt::AlignBottom);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
}

void ScreenMirroringWindow::initWorkWidget()
{
    stackedLayout = new QStackedLayout;

    LockScreenWidget *lockWidget = new LockScreenWidget(this);
    stackedLayout->addWidget(lockWidget);
    stackedLayout->setCurrentIndex(0);
    //todo add screencast widget
}

void ScreenMirroringWindow::initBottom()
{
    bottomWidget = new QWidget(this);
    bottomWidget->setMaximumHeight(56);
    bottomWidget->setStyleSheet(".QWidget{background-color : white;}");

    QHBoxLayout *buttonLayout = new QHBoxLayout(bottomWidget);
    QStringList buttonIcons = { "phone_back", "home", "multi_task" };   // 按钮icon列表

    for (int i = 0; i < buttonIcons.size(); ++i) {
        CooperationIconButton *btn = new CooperationIconButton();
        btn->setIcon(QIcon::fromTheme(buttonIcons[i]));
        btn->setIconSize(QSize(16, 16));
        btn->setFixedSize(36, 36);

        connect(btn, &CooperationIconButton::clicked, this, [this, i]() {
            emit this->ButtonClicked(i);
        });

        buttonLayout->setAlignment(Qt::AlignCenter);
        buttonLayout->setSpacing(20);
        buttonLayout->addWidget(btn);
    }
}

void ScreenMirroringWindow::initTitleBar(const QString &device)
{
    auto titleBar = titlebar();

    titleBar->setIcon(QIcon::fromTheme(KIcon));
    titleBar->setMenuVisible(false);

    QLabel *title = new QLabel(device);
    titleBar->addWidget(title, Qt::AlignLeft);
}

LockScreenWidget::LockScreenWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon::fromTheme(KIcon).pixmap(64, 64));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel(tr("The current device has been locked"), this);
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subTitleLabel = new QLabel(tr("You can unlock it on mobile devices"), this);
    subTitleLabel->setStyleSheet("font-weight: 400; font-size: 12px; color:rgba(0, 0, 0, 0.6);");
    subTitleLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(subTitleLabel);
    layout->setAlignment(Qt::AlignCenter);
}

void LockScreenWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    //Q_UNUSED(event);
    //    QPainter painter(this);
    //    QPixmap background(":/icons/deepin/builtin/icons/background_app.png");
    //    painter.drawPixmap(this->rect(), background.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}
