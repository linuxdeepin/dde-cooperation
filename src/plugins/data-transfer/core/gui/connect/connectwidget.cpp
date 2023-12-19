#ifndef WIN32
#    include "choosewidget.h"
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
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel(tr("Ready to connect"), this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPixelSize(24);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel = new QLabel(tr("Please open data transfer on Windows, and imput the IP and connect code"), this);
    tipLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel->setFixedHeight(20);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel->setFont(font);

    connectLayout = new QHBoxLayout();
    initConnectLayout();

    WarnningLabel = new QLabel(tr("Connect code is expired, please refresh for new code"), this);
    WarnningLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    WarnningLabel->setFixedHeight(80);
    font.setPointSize(8);
    font.setWeight(QFont::Thin);
    WarnningLabel->setFont(font);

    QPalette palette;
    QColor color;
    color.setNamedColor("#FF5736");
    palette.setColor(QPalette::WindowText, color);
    WarnningLabel->setPalette(palette);
    WarnningLabel->setMargin(5);
    WarnningLabel->setVisible(false);

    backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setFixedSize(250, 36);
    backButton->setStyleSheet("background-color: #E3E3E3;");
    connect(backButton, &QToolButton::clicked, this, &ConnectWidget::backPage);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(backButton, Qt::AlignCenter);

    IndexLabel *indelabel = new IndexLabel(1, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel);
    mainLayout->addLayout(connectLayout);
    mainLayout->addWidget(WarnningLabel);
    mainLayout->addLayout(layout);
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
    QLabel *ipLabel = new QLabel(this);

    iconLabel->setPixmap(QIcon(":/icon/computer.svg").pixmap(96, 96));

    ipLabel->setStyleSheet("background-color: rgba(0, 129, 255, 0.2); border-radius: 16;");
    QString ip = QString("<font size=12px >%1： </font><span style='font-size: 17px; font-weight: 600;'>%2</span>")
                         .arg(tr("Local IP"))
                         .arg(ipaddress);
    ipLabel->setText(ip);
    ipLabel->setFixedSize(204, 32);

    iconLabel->setAlignment(Qt::AlignCenter);
    nameLabel->setAlignment(Qt::AlignCenter);
    ipLabel->setAlignment(Qt::AlignCenter);

    ipVLayout->addWidget(iconLabel);
    ipVLayout->addWidget(nameLabel);
    ipVLayout->addWidget(ipLabel);
    ipVLayout->setAlignment(Qt::AlignCenter);

    //passwordLayout
    QString password = TransferHelper::instance()->getConnectPassword();
    if (password.isEmpty())
        remainingTime = 0;
    else
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
    font.setPointSize(40);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 4);
    font.setWeight(QFont::Normal);
    font.setStyleHint(QFont::Helvetica);
    passwordLabel->setFont(font);

    QFont tipfont;
    tipfont.setPointSize(8);
    refreshLabel->setFont(tipfont);
    refreshLabel->setAlignment(Qt::AlignBottom);
    refreshLabel->setText(QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>").arg(tr("Refresh")));

    tipLabel->setFont(tipfont);

    QTimer *timer = new QTimer();
    connect(timer, &QTimer::timeout, [refreshLabel, tipLabel, passwordLabel, nullLabel, timer, this]() {
        if (remainingTime > 0) {
            remainingTime--;
            QString tip = QString("%1<font color='#6199CA'>%2s</font>%3").arg(tr("The code will be expired in")).arg(QString::number(remainingTime)).arg(tr("please input connect code as soon as possible"));
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
    QString styleSheet = "QLabel { background-color: rgba(0, 0, 0, 0.1); width: 2px; }";
    QLabel *separatorLabel = new QLabel(this);
    separatorLabel->setFixedSize(2, 160);
    separatorLabel->setStyleSheet(styleSheet);

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
    emit TransferHelper::instance()->changeWidget(PageName::waitgwidget);
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
        setStyleSheet(".ChooseWidget{ background-color: rgba(255,255,255,1); border-radius: 10px;}");
        backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "background-color: lightgray;"
                                  "}");

    } else {
        // dark
        setStyleSheet(".ChooseWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "opacity: 1;"
                                  "background-color: rgba(255,255,255, 0.1);"
                                  "}");
    }
}
#endif
