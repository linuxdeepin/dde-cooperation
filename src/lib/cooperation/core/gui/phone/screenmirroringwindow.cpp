// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "screenmirroringwindow.h"
#include "global_defines.h"
#include "vncviewer.h"

#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>
#include <DTitlebar>

#include <QVBoxLayout>
#include <QStackedLayout>

#include <gui/utils/cooperationguihelper.h>
using namespace cooperation_core;

inline constexpr char KIcon[] { ":/icons/deepin/builtin/icons/uos_assistant.png" };

ScreenMirroringWindow::ScreenMirroringWindow(const QString &device, QWidget *parent)
    : CooperationMainWindow(parent)
{
    initTitleBar(device);
    initWorkWidget();
    initBottom();

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    mainLayout->addLayout(stackedLayout);
    mainLayout->addWidget(bottomWidget, Qt::AlignBottom);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    connect(this, &ScreenMirroringWindow::buttonClicked, m_vncViewer, &VncViewer::onShortcutAction);
    connect(m_vncViewer, &VncViewer::sizeChanged, this, &ScreenMirroringWindow::handleSizeChange);
}

ScreenMirroringWindow::~ScreenMirroringWindow()
{
    if (m_vncViewer) {
        m_vncViewer->stop();
    }
}

void ScreenMirroringWindow::initWorkWidget()
{
    stackedLayout = new QStackedLayout;

    m_vncViewer = new VncViewer(this);
    stackedLayout->addWidget(m_vncViewer);

    LockScreenWidget *lockWidget = new LockScreenWidget(this);
    stackedLayout->addWidget(lockWidget);
    stackedLayout->setCurrentIndex(0);
}

void ScreenMirroringWindow::initBottom()
{
    bottomWidget = new QWidget(this);
    bottomWidget->setFixedHeight(BOTTOM_HEIGHT);
    bottomWidget->setStyleSheet(".QWidget{background-color : white;}");
    QString lightStyle = ".QWidget{background-color : white;}";
    QString darkStyle = ".QWidget{background-color : rgba(0, 0, 0, 0.1);}";
    CooperationGuiHelper::initThemeTypeConnect(bottomWidget, lightStyle, darkStyle);

    QHBoxLayout *buttonLayout = new QHBoxLayout(bottomWidget);
    QStringList buttonIcons = { "phone_back", "home", "multi_task" };   // 按钮icon列表

    for (int i = 0; i < buttonIcons.size(); ++i) {
        CooperationIconButton *btn = new CooperationIconButton();
        btn->setIcon(QIcon::fromTheme(buttonIcons[i]));
        btn->setIconSize(QSize(16, 16));
        btn->setFixedSize(36, 36);

        connect(btn, &CooperationIconButton::clicked, this, [this, i]() {
            emit this->buttonClicked(i);
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

void ScreenMirroringWindow::connectVncServer(const QString &ip, int port, const QString &password)
{
    m_vncViewer->setServes(ip.toStdString(), port, password.toStdString());
    m_vncViewer->start();
}

void ScreenMirroringWindow::handleSizeChange(const QSize &size)
{
    float scale = 1.0;
    QScreen *screen = qApp->primaryScreen();
    if (screen) {
        int height = screen->geometry().height();

        if (height >= 2160) {
            // 4K
            scale = 1.8;
        } else if (height >= 1140) {
            // 2K
            scale = 2.2;
        } else {
            // 1080P
            scale = 2.6;
        }
    }

    auto titleBar = titlebar();
    int w = (size.width() / scale);
    int h = (size.height() / scale) + titleBar->height() + BOTTOM_HEIGHT;

    this->resize(w, h);
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
