#include "choosewidget.h"
#include "connectwidget.h"
#include "../type_defines.h"

#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QLineEdit>
#include <QTimer>
#include <QHostInfo>
#include <QNetworkInterface>

#include <utils/transferhepler.h>

#pragma execution_character_set("utf-8")

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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("准备连接", this);
    titileLabel->setFixedHeight(40);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel1 = new QLabel("请前往Windows，打开迁移工具，输入本机IP和连接密码。", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel1->setFixedHeight(20);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    connectLayout = new QHBoxLayout(this);
    initConnectLayout();

    QLabel *tipLabel = new QLabel("验证码已过期，请刷新获取新的验证码", this);
    tipLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    tipLabel->setFixedHeight(80);
    font.setPointSize(8);
    font.setWeight(QFont::Thin);
    tipLabel->setFont(font);
    QPalette palette;
    QColor color;
    color.setNamedColor("#FF5736");
    palette.setColor(QPalette::WindowText, color);   // 设置文本颜色为红色
    tipLabel->setPalette(palette);
    tipLabel->setMargin(5);
    tipLabel->setVisible(false);

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("返回");
    nextButton->setFixedSize(250, 36);
    nextButton->setStyleSheet("background-color: #E3E3E3;");
    connect(nextButton, &QToolButton::clicked, this, &ConnectWidget::nextPage);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(nextButton, Qt::AlignCenter);

    IndexLabel *indelabel = new IndexLabel(1, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(connectLayout);
    mainLayout->addWidget(tipLabel);
    mainLayout->addLayout(layout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
}

void ConnectWidget::initConnectLayout()
{
    //ipLayout
    QList<QHostAddress> address = QNetworkInterface::allAddresses();

    QVBoxLayout *ipVLayout = new QVBoxLayout(this);
    QLabel *iconLabel = new QLabel(this);
    QLabel *nameLabel = new QLabel(QHostInfo::localHostName() + "的电脑", this);
    QLabel *ipLabel = new QLabel(this);

    iconLabel->setPixmap(QIcon(":/icon/computer.svg").pixmap(96, 96));

    ipLabel->setStyleSheet("background-color: rgba(0, 129, 255, 0.2); border-radius: 16;");
    QString ip = QString("<font size=12px >本机 IP： </font><span style='font-size: 17px; font-weight: 600;'>%1</span>")
                         .arg(address[2].toString());
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
        password = "777777";
    QHBoxLayout *passwordHLayout = new QHBoxLayout(this);
    QVBoxLayout *passwordVLayout = new QVBoxLayout(this);
    QLabel *passwordLabel = new QLabel(password, this);
    QLabel *refreshLabel = new QLabel("", this);
    QLabel *tipLabel = new QLabel(this);

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
    refreshLabel->setText("<a href=\"https://\" style=\"text-decoration:none;\">刷新</a>");

    tipLabel->setFont(tipfont);

    remainingTime = 300;
    QTimer *timer = new QTimer();
    connect(timer, &QTimer::timeout, [timer, tipLabel, this]() {
        if (remainingTime > 0) {
            remainingTime--;
            QString tip = QString("密码有效时间还剩<font color='#6199CA'>%1s</font>，请尽快输入连接密码").arg(QString::number(remainingTime));
            tipLabel->setText(tip);
        } else {
            tipLabel->setText("0");
            timer->stop();
        }
    });
    timer->start(1000);
    connect(refreshLabel, &QLabel::linkActivated, this, [this, timer, passwordLabel] {
        QString password = TransferHelper::instance()->getConnectPassword();
        passwordLabel->setText(password);
        remainingTime = 300;
        if (!timer->isActive())
            timer->start(1000);
    });

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

    connectLayout->addSpacing(60);
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
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void ConnectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
