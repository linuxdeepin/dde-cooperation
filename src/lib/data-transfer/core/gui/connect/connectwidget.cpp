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
#    include <QNetworkInterface>
#    include <QDesktopServices>

#    include <net/helper/transferhepler.h>

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
    mainLayout->addSpacing(50);
    mainLayout->addLayout(connectLayout);
    mainLayout->addWidget(WarnningLabel);
    mainLayout->addSpacing(50);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
    DLOG << "ConnectWidget initUI finished";
}

void ConnectWidget::initConnectLayout()
{
    DLOG << "ConnectWidget initConnectLayout";
    //ipLayout
    QList<QHostAddress> address = QNetworkInterface::allAddresses();
    QString ipaddress = address.count() > 2 ? address[2].toString() : "";

    QVBoxLayout *ipVLayout = new QVBoxLayout();
    QLabel *iconLabel = new QLabel(this);
    QLabel *nameLabel = new QLabel(QHostInfo::localHostName() + tr("computer"), this);
    QFrame *ipFrame = new QFrame(this);
    ipLabel = new QLabel(this);
    ipLabel1 = new QLabel(tr("Local IP") + ":", this);
    iconLabel->setPixmap(QIcon(":/icon/computer.svg").pixmap(96, 96));

    ipFrame->setStyleSheet(".QFrame{"
                           "background-color: rgba(0, 129, 255, 0.1); "
                           "border-radius: 16; "
                           "border: 1px solid rgba(0, 129, 255, 0.2);"
                           "}");
	ipFrame->setFixedHeight(32);
	ipFrame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    ipLabel->setText(ipaddress);
    ipLabel1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    StyleHelper::setAutoFont(ipLabel, 17, QFont::Bold);
    StyleHelper::setAutoFont(ipLabel1, 12, QFont::Normal);

	// keep both the localized prefix and IP value inside the rounded frame
	QHBoxLayout *ipInnerLayout = new QHBoxLayout(ipFrame);
	ipInnerLayout->addWidget(ipLabel1);
	ipInnerLayout->addSpacing(8);
	ipInnerLayout->addWidget(ipLabel);
	ipInnerLayout->setSpacing(8);
	ipInnerLayout->setContentsMargins(12, 0, 12, 0);

    iconLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setAlignment(Qt::AlignCenter);

    ipVLayout->addWidget(iconLabel);
    ipVLayout->addWidget(nameLabel);
	ipVLayout->addWidget(ipFrame);
    ipVLayout->setAlignment(Qt::AlignCenter);

    //passwordLayout
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
            DLOG << "Remaining time:" << remainingTime << "s";
            remainingTime--;
            QString tip = QString("%1<font color='#6199CA'> %2s </font>%3").arg(tr("The code will be expired in")).arg(QString::number(remainingTime)).arg(tr("please input connect code as soon as possible"));
            tipLabel->setText(tip);
        } else {
            DLOG << "Connect code expired or password label is empty";
            tipLabel->setVisible(false);
            passwordLabel->setVisible(false);
            nullLabel->setVisible(true);
            WarnningLabel->setVisible(true);
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
        WarnningLabel->setVisible(false);
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

    //separatorLabel
    separatorLabel = new QLabel(this);
    separatorLabel->setFixedSize(2, 160);
    separatorLabel->setStyleSheet(".QLabel { background-color: rgba(0, 0, 0, 0.1); width: 2px; }");

    connectLayout->addSpacing(37);
    connectLayout->addLayout(ipVLayout);
    connectLayout->addSpacing(30);
    connectLayout->addWidget(separatorLabel);
    connectLayout->addSpacing(30);
    connectLayout->addLayout(passwordVLayout);
    connectLayout->setSpacing(15);
    connectLayout->setAlignment(Qt::AlignCenter);
    DLOG << "ConnectWidget initConnectLayout finished";
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

    // light
    if (theme == 1) {
        DLOG << "Theme is light, setting stylesheet";
        setStyleSheet(".ConnectWidget{background-color: rgba(255,255,255,1); border-radius: 10px;}");
        separatorLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 0.1); width: 2px; }");
        ipLabel->setStyleSheet(" ");
        ipLabel1->setStyleSheet(" ");
    } else {
        // dark
        DLOG << "Theme is dark, setting stylesheet";
        setStyleSheet(".ConnectWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        separatorLabel->setStyleSheet("background-color: rgba(220, 220, 220,0.1); width: 2px;");
        ipLabel->setStyleSheet("color: rgb(192, 192, 192);");
        ipLabel1->setStyleSheet("color: rgb(192, 192, 192);");
    }
}
#endif
