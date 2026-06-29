// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef __linux__
#    include "connectwidget.h"
#    include "../type_defines.h"

#    include <QLabel>
#    include <QDebug>
#    include <QToolButton>
#    include <QStackedWidget>
#    include <QLineEdit>
#    include <QTimer>
#    include <QHostInfo>
#    include <QRegularExpressionValidator>
#    include <QVBoxLayout>
#    include <QGridLayout>
#    include <common/commonutils.h>
#    include <QDesktopServices>
#    include <QMouseEvent>

#    include <net/helper/transferhepler.h>
#    include <utils/portmanager.h>

ConnectWidget::ConnectWidget(QWidget *parent)
    : QFrame(parent)
{
    DLOG << "Widget constructor called";
    initUI();
}

ConnectWidget::~ConnectWidget()
{
    DLOG << "Widget destructor called";
}

void ConnectWidget::initUI()
{
    DLOG << "ConnectWidget initUI";
    setStyleSheet(".ConnectWidget{background-color: white; border-radius: 10px;}");
    setFocusPolicy(Qt::StrongFocus);  // 接受焦点，点击即可让 portInput 失焦触发校验

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel(tr("Ready to connect"), this);
    titileLabel->setFixedHeight(50);
    StyleHelper::setAutoFont(titileLabel, 24, QFont::DemiBold);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel = new QLabel(tr("Please open data transfer on Windows, and imput the IP and connect code"), this);
    tipLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel->setFixedHeight(30);

    QLabel *downloadLabel = new QLabel("", this);
    downloadLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    downloadLabel->setContentsMargins(0, 10, 0, 0);
    downloadLabel->setText(QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>").arg(tr("Download Windows client")));
    connect(downloadLabel, &QLabel::linkActivated, this, [] {
        DLOG << "Download Windows client link activated";
        QDesktopServices::openUrl(QUrl("https://www.chinauos.com/resource/deepin-data-transfer"));
    });

    StyleHelper::setAutoFont(tipLabel, 14, QFont::Normal);
    StyleHelper::setAutoFont(downloadLabel, 12, QFont::Normal);

    connectLayout = new QHBoxLayout();
    initConnectLayout();

    WarnningLabel = new QLabel(tr("Connect code is expired, please refresh for new code"), this);
    WarnningLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    WarnningLabel->setFixedHeight(80);
    StyleHelper::setAutoFont(WarnningLabel, 12, QFont::Normal);

    QPalette palette;
    QColor color;
    color.setNamedColor("#FF5736");
    palette.setColor(QPalette::WindowText, color);
    WarnningLabel->setPalette(palette);
    WarnningLabel->setMargin(5);
    WarnningLabel->setVisible(false);

    ButtonLayout *buttonLayout = new ButtonLayout();
    buttonLayout->setCount(1);
    backButton = buttonLayout->getButton1();
    backButton->setText(tr("Back"));
    connect(backButton, &QPushButton::clicked, this, &ConnectWidget::backPage);

    IndexLabel *indelabel = new IndexLabel(1, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel);
    mainLayout->addWidget(downloadLabel);
    mainLayout->addSpacing(30);
    mainLayout->addLayout(connectLayout);
    mainLayout->setStretchFactor(connectLayout, 1);
    mainLayout->addSpacing(30);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
    DLOG << "ConnectWidget initUI finished";
}

void ConnectWidget::initConnectLayout()
{
    DLOG << "ConnectWidget initConnectLayout";
    QString ipaddress = deepin_cross::CommonUitls::getFirstIp().data();
    m_savedPort = PortManager::instance()->getPort();

    QVBoxLayout *ipVLayout = new QVBoxLayout();
    ipVLayout->setSpacing(8);
    QLabel *iconLabel = new QLabel(this);
    QLabel *nameLabel = new QLabel(QHostInfo::localHostName() + tr("computer"), this);

    // ---- IP 信息区（无外框） ----
    ipLabel = new QLabel(ipaddress, this);
    ipLabel1 = new QLabel(tr("Local IP") + ":", this);
    StyleHelper::setAutoFont(ipLabel, 14, QFont::Bold);
    StyleHelper::setAutoFont(ipLabel1, 12, QFont::Normal);

    // 端口（可见输入框，可直接编辑）
    QLabel *portLabel = new QLabel(tr("Port") + ":", this);
    StyleHelper::setAutoFont(portLabel, 12, QFont::Normal);

    portInput = new QLineEdit(QString::number(m_savedPort), this);
    portInput->setStyleSheet("background-color: white;"
                             "border: 1px solid #d8d7d7;"
                             "border-radius: 8px;"
                             "padding: 2px 8px;");
    portInput->setFixedHeight(36);
    portInput->setFixedWidth(80);
    portInput->setValidator(new QRegularExpressionValidator(QRegularExpression("^[0-9]*$"), this));
    portInput->setAlignment(Qt::AlignLeft);
    StyleHelper::setAutoFont(portInput, 14, QFont::Normal);
    connect(portInput, &QLineEdit::editingFinished, this, &ConnectWidget::onPortEditingFinished);

    // 监听端口变更
    connect(PortManager::instance(), &PortManager::portChanged, this, [this](int newPort) {
        m_savedPort = newPort;
        portInput->setText(QString::number(newPort));
    });

    // 网格布局：IP 行与端口行的标签、值分别列对齐
    // 两侧 stretch（列0/3）使标签+值整体水平居中，标签列右对齐、值列左对齐，两行严格对齐
    QGridLayout *infoGrid = new QGridLayout();
    infoGrid->setHorizontalSpacing(8);
    infoGrid->setVerticalSpacing(8);
    infoGrid->setColumnStretch(0, 1);   // 左侧弹性，把内容推到中间
    infoGrid->setColumnStretch(3, 1);   // 右侧弹性
    infoGrid->addWidget(ipLabel1, 0, 1, Qt::AlignRight | Qt::AlignVCenter);
    infoGrid->addWidget(ipLabel, 0, 2, Qt::AlignLeft | Qt::AlignVCenter);
    infoGrid->addWidget(portLabel, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
    infoGrid->addWidget(portInput, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);

    iconLabel->setPixmap(QIcon(":/icon/computer.svg").pixmap(96, 96));
    // 容器宽度（220）大于图标（96）：撑开左侧信息区宽度，使 IP/端口列与右侧密码区比例协调
    iconLabel->setFixedSize(220, 96);
    iconLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setAlignment(Qt::AlignCenter);

    ipVLayout->addWidget(iconLabel, 0, Qt::AlignCenter);
    ipVLayout->setSpacing(10);
    ipVLayout->addWidget(nameLabel, 0, Qt::AlignCenter);
    ipVLayout->addLayout(infoGrid);

    // ---- 密码区 ----
    QString password = TransferHelper::instance()->updateConnectPassword();
    remainingTime = 300;

    QHBoxLayout *passwordHLayout = new QHBoxLayout();
    QVBoxLayout *passwordVLayout = new QVBoxLayout();
    QLabel *passwordLabel = new QLabel(password, this);
    QLabel *refreshLabel = new QLabel("", this);
    QLabel *tipLabel = new QLabel(this);
    QLabel *nullLabel = new QLabel("<font color='#D8D8D8' size='14'>---- ---- ---- --</font>", this);

    nullLabel->setFixedWidth(200);
    nullLabel->setVisible(false);

    QFont font;
    font.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    font.setWeight(QFont::Normal);
    font.setStyleHint(QFont::Helvetica);
    passwordLabel->setFont(font);

    StyleHelper::setAutoFont(refreshLabel, 12, QFont::Normal);
    StyleHelper::setAutoFont(tipLabel, 12, QFont::Normal);
    StyleHelper::setAutoFont(passwordLabel, 54, QFont::Normal);

    refreshLabel->setAlignment(Qt::AlignBottom);
    refreshLabel->setFixedHeight(55);
    refreshLabel->setText(QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>").arg(tr("Refresh")));

    tipLabel->setWordWrap(true);

    QTimer *timer = new QTimer();
    connect(timer, &QTimer::timeout, [refreshLabel, tipLabel, passwordLabel, nullLabel, timer, this]() {
        if (remainingTime > 0 && !passwordLabel->text().isEmpty()) {
            remainingTime--;
            QString tip = QString("%1<font color='#6199CA'> %2s </font>%3").arg(tr("The code will be expired in")).arg(QString::number(remainingTime)).arg(tr("please input connect code as soon as possible"));
            tipLabel->setText(tip);
        } else {
            DLOG << "Connect code expired or password label is empty";
            tipLabel->setVisible(false);
            passwordLabel->setVisible(false);
            nullLabel->setVisible(true);
            WarnningLabel->adjustSize();
            WarnningLabel->move((this->width() - WarnningLabel->width()) / 2, this->height() - 140);
            WarnningLabel->raise();
            WarnningLabel->show();
            timer->stop();
            emit refreshLabel->linkActivated(" ");
        }
    });
    timer->start(1000);
    connect(refreshLabel, &QLabel::linkActivated, this, [this, timer, passwordLabel, tipLabel, nullLabel] {
        DLOG << "Refreshing connection password";
        QString password = TransferHelper::instance()->updateConnectPassword();
        passwordLabel->setText(password);
        tipLabel->setVisible(true);
        passwordLabel->setVisible(true);
        nullLabel->setVisible(false);
        WarnningLabel->hide();
        remainingTime = 300;
        if (!timer->isActive()) {
            DLOG << "Restarting password expiration timer";
            timer->start(1000);
        }
    });

    passwordHLayout->addWidget(nullLabel);
    passwordHLayout->addWidget(passwordLabel);
    passwordHLayout->addWidget(refreshLabel);

    passwordVLayout->setContentsMargins(0, 0, 0, 0);
    passwordVLayout->setSpacing(0);
    passwordVLayout->addLayout(passwordHLayout);
    passwordVLayout->addWidget(tipLabel);
    passwordVLayout->setAlignment(Qt::AlignCenter);

    // 分隔线
    separatorLabel = new QLabel(this);
    separatorLabel->setFixedSize(2, 160);
    separatorLabel->setStyleSheet(".QLabel { background-color: rgba(0, 0, 0, 0.1); width: 2px; }");

    connectLayout->addStretch();
    connectLayout->addSpacing(37);
    connectLayout->addLayout(ipVLayout);
    connectLayout->addSpacing(30);
    connectLayout->addWidget(separatorLabel);
    connectLayout->addSpacing(30);
    connectLayout->addLayout(passwordVLayout);
    connectLayout->addSpacing(37);
    connectLayout->addStretch();
    connectLayout->setSpacing(15);
    connectLayout->setAlignment(Qt::AlignCenter);
    DLOG << "ConnectWidget initConnectLayout finished";
}

void ConnectWidget::onPortEditingFinished()
{
    DLOG << "onPortEditingFinished called, text:'" << portInput->text().toStdString() << "'"
         << "savedPort:" << m_savedPort;

    if (!portDebounceTimer) {
        portDebounceTimer = new QTimer(this);
        portDebounceTimer->setSingleShot(true);
        portDebounceTimer->setInterval(1000);
        connect(portDebounceTimer, &QTimer::timeout, this, [this]() {
            QString text = portInput->text().trimmed();
            DLOG << "debounced validate, text:'" << text.toStdString() << "'";

            if (text.isEmpty()) {
                portInput->setText(QString::number(m_savedPort));
                WarnningLabel->hide();
                return;
            }

            int port = text.toInt();
            if (port == m_savedPort) {
                WarnningLabel->hide();
                return;
            }

            QString err = PortManager::instance()->validatePort(port);
            DLOG << "validate result:'" << err.toStdString() << "'";
            if (err.isEmpty()) {
                PortManager::instance()->setPort(port);
                m_savedPort = port;
                WarnningLabel->hide();
            } else {
                portInput->setText(QString::number(m_savedPort));
                WarnningLabel->setText(err);
                WarnningLabel->adjustSize();
                int x = (width() - WarnningLabel->width()) / 2;
                WarnningLabel->move(x, height() - 140);
                WarnningLabel->lower();
                WarnningLabel->show();
            }
        });
    }

    portDebounceTimer->start();
}

void ConnectWidget::nextPage()
{
    DLOG << "Navigating to wait widget";
    emit TransferHelper::instance()->changeWidget(PageName::waitwidget);
}

void ConnectWidget::backPage()
{
    DLOG << "Returning to previous page";
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        DLOG << "Stacked widget found, setting current index to previous page";
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
        DLOG << "Stacked widget not found";
    }
}

void ConnectWidget::themeChanged(int theme)
{
    DLOG << "Theme changed to:" << (theme == 1 ? "light" : "dark");

    if (theme == 1) {
        DLOG << "Theme is light, setting stylesheet";
        setStyleSheet(".ConnectWidget{background-color: rgba(255,255,255,1); border-radius: 10px;}");
        separatorLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 0.1); width: 2px; }");
        ipLabel->setStyleSheet(" ");
        ipLabel1->setStyleSheet(" ");
    } else {
        DLOG << "Theme is dark, setting stylesheet";
        setStyleSheet(".ConnectWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        separatorLabel->setStyleSheet("background-color: rgba(220, 220, 220,0.1); width: 2px;");
        ipLabel->setStyleSheet("color: rgb(192, 192, 192);");
        ipLabel1->setStyleSheet("color: rgb(192, 192, 192);");
    }
}

void ConnectWidget::mousePressEvent(QMouseEvent *event)
{
    // 点击端口输入框以外的区域时失焦，触发 editingFinished 校验
    QFrame::mousePressEvent(event);
    if (portInput && portInput->hasFocus()) {
        QPoint localPos = portInput->mapFrom(this, event->pos());
        if (!portInput->rect().contains(localPos)) {
            portInput->clearFocus();
        }
    }
}
#endif
