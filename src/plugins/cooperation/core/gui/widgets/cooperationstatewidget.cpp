// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationstatewidget.h"
#include "backgroundwidget.h"
#include "global_defines.h"
#include "utils/cooperationguihelper.h"

#ifdef linux
#    include <DPalette>
#endif

#include <QVariant>
#include <QVBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QUrl>
#include <QTimer>
#include <QDesktopServices>
#include <QToolButton>
#include <QScrollArea>

#include <utils/cooperationutil.h>

using namespace cooperation_core;

#ifdef linux
const char *Kfind_device = "find_device";
const char *Kno_network = "no_network";
const char *Knot_find_device = "not_find_device";
#else
const char *Kfind_device = ":/icons/deepin/builtin/light/icons/find_device_250px.svg";
const char *Kno_network = ":/icons/deepin/builtin/icons/dark@2x.png";
const char *Knot_find_device = ":/icons/deepin/builtin/light/icons/not_find_device_150px.svg";
#endif

LookingForDeviceWidget::LookingForDeviceWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();

    animationTimer = new QTimer(this);
    animationTimer->setInterval(16);
    connect(animationTimer, &QTimer::timeout, this, [this] { update(); });
}

void LookingForDeviceWidget::seAnimationtEnabled(bool enabled)
{
    if (isEnabled == enabled)
        return;

    angle = 0;
    (isEnabled = enabled) ? animationTimer->start() : animationTimer->stop();
}

void LookingForDeviceWidget::initUI()
{
    setFocusPolicy(Qt::ClickFocus);

    iconLabel = new CooperationLabel(this);
    iconLabel->setFixedSize(250, 250);
    QIcon icon = QIcon::fromTheme(Kfind_device);
    iconLabel->setPixmap(icon.pixmap(250, 250));
    connect(CooperationGuiHelper::instance(), &CooperationGuiHelper::themeTypeChanged, this, [icon, this] {
        iconLabel->setPixmap(icon.pixmap(250, 250));
    });

    CooperationLabel *tipsLabel = new CooperationLabel(tr("Looking for devices"), this);
    tipsLabel->setAlignment(Qt::AlignHCenter);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    vLayout->addSpacing(38);
    vLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    vLayout->addWidget(tipsLabel, 0, Qt::AlignVCenter);
    vLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setLayout(vLayout);
}

void LookingForDeviceWidget::paintEvent(QPaintEvent *event)
{
    // 绘制动画效果
    if (isEnabled) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 获取中心点坐标
        int centerX = iconLabel->geometry().center().x() + 1;
        int centerY = iconLabel->geometry().center().y();
        QConicalGradient gradient(centerX, centerY, angle + 180);
        if (CooperationGuiHelper::isDarkTheme()) {
            gradient.setColorAt(0.3, QColor(63, 63, 63));
            gradient.setColorAt(0.7, QColor(63, 63, 63, 0));
        } else {
            gradient.setColorAt(0.3, QColor(208, 228, 245));
            gradient.setColorAt(0.7, QColor(208, 228, 245, 0));
        }

        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);

        // 计算绘制矩形区域，半径111
        int newLeft = centerX - 111;
        int newTop = centerY - 111;
        QRect drawRect(newLeft, newTop, 222, 222);

        painter.drawPie(drawRect, angle * 16, 90 * 16);
        angle -= 2;
    }

    QWidget::paintEvent(event);
}

NoNetworkWidget::NoNetworkWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void NoNetworkWidget::initUI()
{
    setFocusPolicy(Qt::ClickFocus);

    CooperationLabel *iconLabel = new CooperationLabel(this);
    iconLabel->setFixedSize(150, 150);
    QIcon icon = QIcon::fromTheme(Kno_network);
    iconLabel->setPixmap(icon.pixmap(150, 150));
    connect(CooperationGuiHelper::instance(), &CooperationGuiHelper::themeTypeChanged, this, [icon, iconLabel] {
        iconLabel->setPixmap(icon.pixmap(150, 150));
    });

    CooperationLabel *tipsLabel = new CooperationLabel(tr("Please connect to the network"), this);

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

NoResultTipWidget::NoResultTipWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void NoResultTipWidget::onLinkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl(link));
}

void NoResultTipWidget::initUI()
{
    QString leadintText =
            tr("1. Enable cross-end collaborative applications. Applications on the UOS "
               "can be downloaded from the App Store, and applications on the Windows "
               "side can be downloaded from: ");
    QString hyperlink = "https://www.deepin.org/index/assistant";

    QString websiteLinkTemplate =
            "<a href='%1' style='text-decoration: none; color: #0081FF;'>%2</a>";
    QString content1 = leadintText + websiteLinkTemplate.arg(hyperlink, hyperlink);
    CooperationLabel *contentLable1 = new CooperationLabel(this);
    QFont font;
    font.setWeight(QFont::Normal);
    font.setPixelSize(12);
    contentLable1->setFont(font);
    contentLable1->setWordWrap(true);
    contentLable1->setText(content1);
    connect(contentLable1, &QLabel::linkActivated, this, &NoResultTipWidget::onLinkActivated);

    CooperationLabel *contentLable2 = new CooperationLabel(tr("2. On the same LAN as the device"), this);
    contentLable2->setWordWrap(true);
    contentLable2->setFont(font);
    CooperationLabel *contentLable3 = new CooperationLabel(
            tr("3. Settings-Basic Settings-Discovery Mode-\"Allow everyone in the same LAN\""),
            this);
    contentLable3->setWordWrap(true);
    contentLable3->setFont(font);
    CooperationLabel *contentLable4 = new CooperationLabel(
            tr("4. Try entering the target device IP in the top search box"),
            this);
    contentLable4->setWordWrap(true);
    contentLable4->setFont(font);

#ifdef linux
    contentLable1->setForegroundRole(DTK_GUI_NAMESPACE::DPalette::TextTips);
    contentLable2->setForegroundRole(DTK_GUI_NAMESPACE::DPalette::TextTips);
    contentLable3->setForegroundRole(DTK_GUI_NAMESPACE::DPalette::TextTips);
    contentLable4->setForegroundRole(DTK_GUI_NAMESPACE::DPalette::TextTips);
#else
    QList<QColor> colorList { QColor(0, 0, 0, qRound(255 * 0.6)),
                              QColor(192, 192, 192) };
    CooperationGuiHelper::instance()->autoUpdateTextColor(contentLable1, colorList);
    CooperationGuiHelper::instance()->autoUpdateTextColor(contentLable2, colorList);
    CooperationGuiHelper::instance()->autoUpdateTextColor(contentLable3, colorList);
    CooperationGuiHelper::instance()->autoUpdateTextColor(contentLable4, colorList);
#endif

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setSpacing(10);
    contentLayout->addWidget(contentLable1);
    contentLayout->addWidget(contentLable2);
    contentLayout->addWidget(contentLable3);
    contentLayout->addWidget(contentLable4);
    setLayout(contentLayout);
}

NoResultWidget::NoResultWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void NoResultWidget::initUI()
{
    setFocusPolicy(Qt::ClickFocus);

    CooperationLabel *iconLabel = new CooperationLabel(this);
    iconLabel->setFixedSize(150, 150);
    QIcon icon = QIcon::fromTheme(Knot_find_device);
    iconLabel->setPixmap(icon.pixmap(150, 150));
    connect(CooperationGuiHelper::instance(), &CooperationGuiHelper::themeTypeChanged, this, [icon, iconLabel] {
        iconLabel->setPixmap(icon.pixmap(150, 150));
    });

    CooperationLabel *tipsLabel = new CooperationLabel(tr("No device found"), this);
    auto font = tipsLabel->font();
    font.setWeight(QFont::Medium);
    tipsLabel->setFont(font);

    BackgroundWidget *contentBackgroundWidget = new BackgroundWidget(this);
    contentBackgroundWidget->setBackground(17, BackgroundWidget::ItemBackground,
                                           BackgroundWidget::TopAndBottom);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->addWidget(new NoResultTipWidget());
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

BottomLabel::BottomLabel(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    setFixedSize(500, 33);
}

void BottomLabel::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QColor color(0, 0, 0, 12);
    painter.setPen(QPen(color, 3));
    painter.drawLine(0, 0, width(), 0);
}

void BottomLabel::initUI()
{
    QString ip = QString(tr("Local IP: %1").arg(CooperationUtil::localIPAddress()));
    QLabel *ipLabel = new QLabel(ip);
    ipLabel->setAlignment(Qt::AlignHCenter);
    ipLabel->setFixedHeight(30);
    ipLabel->setStyleSheet("color: rgba(0, 0, 0, 0.6);");
    QFont font;
    font.setWeight(QFont::Normal);
    font.setPixelSize(12);
    ipLabel->setFont(font);

    dailog = new CooperationAbstractDialog(this);
    QScrollArea *scrollArea = new QScrollArea(dailog);
    QToolButton *tipButton = new QToolButton(qobject_cast<QWidget *>(this->parent()));
#ifdef linux
    tipButton->setGeometry(460, 552, 24, 24);
#else
    tipButton->setGeometry(455, 600, 24, 24);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dailog->setStyleSheet("background-color: white;"
                          "border-radius: 10px;}"
                          "QScrollBar:vertical {"
                          "width: 0px;"
                          "}");
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");
#endif
    tipButton->setIcon(QIcon(":/icons/deepin/builtin/texts/icon_tips.svg"));
    tipButton->setIconSize(QSize(24, 24));

    dailog->setFixedSize(260, 207);
    scrollArea->setWidgetResizable(true);
    QWidget *contentWidget = new QWidget;

    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->addWidget(new NoResultTipWidget());
    scrollArea->setWidget(contentWidget);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    dailog->setLayout(contentLayout);
    contentLayout->addWidget(scrollArea);

    connect(tipButton, &QToolButton::clicked, [this]() {
        QMainWindow *activeMainWindow = qobject_cast<QMainWindow *>(QApplication::activeWindow());
#ifdef linux
        dailog->move(activeMainWindow->pos() + QPoint(220, 390));
#else
       dailog->move(activeMainWindow->pos() + QPoint(220, 360));
#endif
        if (dailog->isVisible())
            dailog->hide();
        else
            dailog->show();
    });
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(ipLabel);
    setLayout(mainLayout);
}
