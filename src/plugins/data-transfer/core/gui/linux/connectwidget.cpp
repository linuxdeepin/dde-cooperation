#include "connectwidget.h"

#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QLineEdit>

#include <utils/transferhepler.h>

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
    titileLabel->setFixedHeight(70);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel1 = new QLabel("请在传输设备上输入以下连接密码，确认输入后\n请点击“下一步”。", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel1->setFixedHeight(80);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    passwordLayout = new QGridLayout(this);
    initPassWord();

    QLabel *tipLabel2 = new QLabel("密码有效时间为5分钟，请尽快输入密码。", this);
    tipLabel2->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel2->setFixedHeight(80);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel2->setFont(font);
    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::red);   // 设置文本颜色为红色
    tipLabel2->setPalette(palette);

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("下一步");
    nextButton->setFixedSize(300, 35);
    nextButton->setStyleSheet("background-color: lightgray;");
    connect(nextButton, &QToolButton::clicked, this, &ConnectWidget::nextPage);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(passwordLayout);
    mainLayout->addWidget(tipLabel2);
    mainLayout->addLayout(layout);
}

void ConnectWidget::initPassWord()
{
    QString password = QString::number(TransferHelper::instance()->getConnectPassword());
    for (int i = 0; i < password.count(); i++) {
        QLabel *digitLabel = new QLabel(QString(password[i]), this);
        digitLabel->setAlignment(Qt::AlignCenter);
        digitLabel->setStyleSheet("border: 2px solid gray; border-radius: 7px;");
        digitLabel->setFixedSize(50, 50);
        passwordLayout->addWidget(digitLabel, 0, i);
    }

    QToolButton *refreshButton = new QToolButton(this);
    refreshButton->setIcon(QIcon::fromTheme("folder"));
    refreshButton->setIconSize(QSize(50, 50));
    refreshButton->setFixedSize(50, 50);
    connect(refreshButton, &QToolButton::clicked, this, &ConnectWidget::updatePassWord);
    passwordLayout->addWidget(refreshButton, 0, password.count() + 1);

    passwordLayout->setSpacing(15);
    passwordLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
}

void ConnectWidget::updatePassWord()
{
    QString password = QString::number(TransferHelper::instance()->getConnectPassword());
    for (int i = 0; i < password.count(); i++) {
        QLabel *digitLabel = qobject_cast<QLabel *>(passwordLayout->itemAt(i)->widget());
        digitLabel->setText(QString(password[i]));
    }
}

void ConnectWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
