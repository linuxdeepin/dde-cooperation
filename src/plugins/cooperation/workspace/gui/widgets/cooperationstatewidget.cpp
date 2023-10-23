// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationstatewidget.h"
#ifdef WIN32
#else
#include "gui/linux/backgroundwidget.h"

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE
#endif

#include <QVBoxLayout>
#include <QIcon>
#include <QLabel>

using namespace cooperation_workspace;

LookingForDeviceWidget::LookingForDeviceWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void LookingForDeviceWidget::initUI()
{
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(277, 277);
    QIcon icon = QIcon::fromTheme("find_device");
    iconLabel->setPixmap(icon.pixmap(277, 277));

    QLabel *tipsLabel = new QLabel(tr("Looking for devices"), this);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    vLayout->addSpacing(38);
    vLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    vLayout->addWidget(tipsLabel, 0, Qt::AlignCenter);
    vLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(vLayout);
}

NoNetworkWidget::NoNetworkWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void NoNetworkWidget::initUI()
{
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(150, 150);
    QIcon icon = QIcon::fromTheme("no_network");
    iconLabel->setPixmap(icon.pixmap(150, 150));

    QLabel *tipsLabel = new QLabel(tr("Please connect to the network"), this);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    vLayout->addSpacing(116);
    vLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    vLayout->addSpacing(14);
    vLayout->addWidget(tipsLabel, 0, Qt::AlignCenter);
    vLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(vLayout);
}

NoResultWidget::NoResultWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void NoResultWidget::onLinkActivated(const QString &link)
{
    DGuiApplicationHelper::openUrl(link);
}

void NoResultWidget::initUI()
{
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setFixedSize(150, 150);
    QIcon icon = QIcon::fromTheme("not_find_device");
    iconLabel->setPixmap(icon.pixmap(150, 150));

    QLabel *tipsLabel = new QLabel(tr("No device found"), this);

    BackgroundWidget *contentBackgroundWidget = new BackgroundWidget(this);
    contentBackgroundWidget->setBackground(17, BackgroundWidget::kItemBackground);
    QString leadintText = tr("1.Enable cross-end collaborative applications. Applications on the UOS "
                             "can be downloaded from the App Store, and applications on the Windows "
                             "side can be downloaded from: ");
    QString hyperlink = "https://www.deepin.org/index/assistant";

    QString websiteLinkTemplate = "<a href='%1' style='text-decoration: none; color: #0081FF;'>%2</a>";
    QString content1 = leadintText + websiteLinkTemplate.arg(hyperlink, hyperlink);
    QLabel *contentLable1 = new QLabel(this);
    contentLable1->setWordWrap(true);
    contentLable1->setText(content1);
    connect(contentLable1, &QLabel::linkActivated, this, &NoResultWidget::onLinkActivated);

    QLabel *contentLable2 = new QLabel(tr("2.On the same LAN as the device"), this);
    contentLable2->setWordWrap(true);
    QLabel *contentLable3 = new QLabel(tr("3.xx xxv xx ff"), this);
    contentLable3->setWordWrap(true);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(15, 10, 15, 10);
    contentLayout->setSpacing(4);
    contentLayout->addWidget(contentLable1);
    contentLayout->addWidget(contentLable2);
    contentLayout->addWidget(contentLable3);
    contentBackgroundWidget->setLayout(contentLayout);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    vLayout->addSpacing(88);
    vLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    vLayout->addSpacing(14);
    vLayout->addWidget(tipsLabel, 0, Qt::AlignCenter);
    vLayout->addSpacing(22);
    vLayout->addWidget(contentBackgroundWidget);
    vLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(vLayout);
}
