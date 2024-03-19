#ifndef WIN32
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

#    include <utils/transferhepler.h>

ConnectWidget::ConnectWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ConnectWidget::~ConnectWidget()
{
}

void ConnectWidget::initUI()
{
    setStyleSheet(".ConnectWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    AdaptFontLabel *titileLabel = new AdaptFontLabel(tr("Ready to connect"), AdaptFontLabel::fontstyle1, this);
    titileLabel->setFixedHeight(50);
    // titileLabel->setFont(StyleHelper::font(1));
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    AdaptFontLabel *tipLabel = new AdaptFontLabel(tr("Please open data transfer on Windows, and imput the IP and connect code"),
                                                  AdaptFontLabel::fontstyle3, this);
    tipLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    //tipLabel->setFixedHeight(20);
    //    QFont font;
    //    font.setPointSize(10);
    //    font.setWeight(QFont::Thin);
    //    tipLabel->setFont(font);
    tipLabel->setWordWrap(true);
    connectLayout = new QHBoxLayout();
    initConnectLayout();

    WarnningLabel = new AdaptFontLabel(tr("Connect code is expired, please refresh for new code"), AdaptFontLabel::error, this);
    WarnningLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    // WarnningLabel->setFixedHeight(80);
    //    font.setPointSize(8);
    //    font.setWeight(QFont::Thin);
    //    WarnningLabel->setFont(font);

    //    QPalette palette;
    //    QColor color;
    //color.setNamedColor("#FF5736");
    //palette.setColor(QPalette::WindowText, color);
    //  WarnningLabel->setPalette(palette);
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
    mainLayout->addSpacing(70);
    mainLayout->addLayout(connectLayout);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(WarnningLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
}

void ConnectWidget::initConnectLayout()
{
    //ipLayout
    QList<QHostAddress> address = QNetworkInterface::allAddresses();
    QString ipaddress = address.count() > 2 ? address[2].toString() : "";

    QVBoxLayout *ipVLayout = new QVBoxLayout();
    QLabel *iconLabel = new QLabel(this);
    QLabel *nameLabel = new QLabel(QHostInfo::localHostName() + tr("computer"), this);
    QFrame *ipFrame = new QFrame(this);
    ipFrame->setContentsMargins(0, 0, 0, 0);
    ipLabel = new AdaptFontLabel(AdaptFontLabel::fontstyle7, this);
    ipLabel1 = new AdaptFontLabel(tr("Local IP") + ":", AdaptFontLabel::fontstyle3, this);
    iconLabel->setPixmap(QIcon(":/icon/computer.svg").pixmap(96, 96));

    ipFrame->setStyleSheet(".QFrame{"
                           "background-color: rgba(0, 129, 255, 0.1); "
                           "border-radius: 16; "
                           "border: 1px solid rgba(0, 129, 255, 0.2);"
                           "}");
    ipFrame->setFixedSize(204, 32);
    //    QString ip = QString("<span style='font-size: 17px; font-weight: 600;'>%1</span>")
    //                         .arg(ipaddress);
    ipLabel->setText(ipaddress);
    // ipLabel1->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    //ipLabel1->setFont(StyleHelper::font(3));
    QHBoxLayout *ipHLayout = new QHBoxLayout(ipFrame);
    //   ipHLayout->setSpacing(8);
    ipHLayout->setAlignment(Qt::AlignHCenter);
    ipHLayout->setMargin(0);
    ipHLayout->addWidget(ipLabel1);
    // ipHLayout->addSpacing(8);
    ipHLayout->addWidget(ipLabel);

    iconLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setAlignment(Qt::AlignCenter);

    ipVLayout->addWidget(iconLabel);
    ipVLayout->addWidget(nameLabel);
    ipVLayout->addWidget(ipFrame);
    ipVLayout->setAlignment(Qt::AlignCenter);

    //passwordLayout
    QString password = TransferHelper::instance()->getConnectPassword();
    remainingTime = 300;

    QHBoxLayout *passwordHLayout = new QHBoxLayout();
    QVBoxLayout *passwordVLayout = new QVBoxLayout();
    AdaptFontLabel *passwordLabel = new AdaptFontLabel(password, AdaptFontLabel::fontstyle6, this);
    AdaptFontLabel *refreshLabel = new AdaptFontLabel(AdaptFontLabel::fontstyle3, this);
    AdaptFontLabel *tipLabel = new AdaptFontLabel(AdaptFontLabel::fontstyle3, this);
    QLabel *nullLabel = new QLabel("<font color='#D8D8D8' size='14'>---- ---- ---- --</font>", this);

    nullLabel->setFixedWidth(200);
    nullLabel->setVisible(false);

    //    QFont font;
    //    font.setPointSize(40);
    //    font.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    //    font.setWeight(QFont::Normal);
    //    font.setStyleHint(QFont::Helvetica);
    //    passwordLabel->setFont(font);

    QFont tipfont;
    tipfont.setPointSize(8);
    refreshLabel->setFont(tipfont);
    refreshLabel->setAlignment(Qt::AlignBottom);
    refreshLabel->setFixedHeight(55);
    refreshLabel->setText(QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>").arg(tr("Refresh")));

    //tipLabel->setFont(tipfont);
     tipLabel->setWordWrap(true);

    QTimer *timer = new QTimer();
    connect(timer, &QTimer::timeout, [refreshLabel, tipLabel, passwordLabel, nullLabel, timer, this]() {
        if (remainingTime > 0 && !passwordLabel->text().isEmpty()) {
            remainingTime--;
            QString tip = QString("%1<font color='#6199CA'> %2s </font>%3").arg(tr("The code will be expired in")).arg(QString::number(remainingTime)).arg(tr("please input connect code as soon as possible"));
            tipLabel->setText(tip);
        } else {
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
        QString password = TransferHelper::instance()->getConnectPassword();
        passwordLabel->setText(password);
        tipLabel->setVisible(true);
        passwordLabel->setVisible(true);
        nullLabel->setVisible(false);
        WarnningLabel->setVisible(false);
        remainingTime = 300;
        if (!timer->isActive())
            timer->start(1000);
    });

    passwordHLayout->addWidget(nullLabel);
    passwordHLayout->addWidget(passwordLabel);
    passwordHLayout->addWidget(refreshLabel);

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
}

void ConnectWidget::nextPage()
{
    emit TransferHelper::instance()->changeWidget(PageName::waitwidget);
}

void ConnectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void ConnectWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet(".ConnectWidget{background-color: rgba(255,255,255,1); border-radius: 10px;}");
        separatorLabel->setStyleSheet("QLabel { background-color: rgba(0, 0, 0, 0.1); width: 2px; }");
        ipLabel->setStyleSheet(" ");
        ipLabel1->setStyleSheet(" ");
    } else {
        // dark
        setStyleSheet(".ConnectWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        separatorLabel->setStyleSheet("background-color: rgba(220, 220, 220,0.1); width: 2px;");
        ipLabel->setStyleSheet("color: rgb(192, 192, 192);");
        ipLabel1->setStyleSheet("color: rgb(192, 192, 192);");
    }
}
#endif
