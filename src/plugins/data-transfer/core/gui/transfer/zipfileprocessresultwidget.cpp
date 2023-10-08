#include "zipfileprocessresultwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QToolButton>
#include <QDebug>
#include <QLabel>

#pragma execution_character_set("utf-8")

zipFileProcessResultWidget::zipFileProcessResultWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

zipFileProcessResultWidget::~zipFileProcessResultWidget() { }

void zipFileProcessResultWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    failed();
    QToolButton *exitButton = new QToolButton(this);
    exitButton->setText("退出");
    exitButton->setFixedSize(120, 35);
    exitButton->setStyleSheet("background-color: lightgray;");
    exitButton->setEnabled(false);
    QObject::connect(exitButton, &QToolButton::clicked, this, &zipFileProcessResultWidget::exit);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(backButton, &QToolButton::clicked, this, &zipFileProcessResultWidget::backPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(exitButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    mainLayout->addLayout(buttonLayout);
}

void zipFileProcessResultWidget::successed()
{
    QLabel *icon = new QLabel(this);
    icon->setPixmap(QIcon(":/icon/success-128.svg").pixmap(128, 128));
    icon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel1 = new QLabel("备份成功", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    QLabel *tipLabel2 = new QLabel("恭喜您，信息备份成功", this);
    tipLabel2->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QString display = "<a href=\"https://\" style=\"text-decoration:none;\">前往查看</a>";
    QLabel *displayLabel = new QLabel(display, this);
    displayLabel->setAlignment(Qt::AlignCenter);
    connect(displayLabel, &QLabel::linkActivated, this, &zipFileProcessResultWidget::informationPage);

    ((QHBoxLayout *)(this->layout()))->addSpacing(100);
    this->layout()->addWidget(icon);
    ((QHBoxLayout *)(this->layout()))->addSpacing(10);
    this->layout()->addWidget(tipLabel1);
    ((QHBoxLayout *)(this->layout()))->addSpacing(0);
    this->layout()->addWidget(tipLabel2);
    ((QHBoxLayout *)(this->layout()))->addSpacing(100);
    this->layout()->addWidget(displayLabel);
}

void zipFileProcessResultWidget::failed()
{
    QLabel *icon = new QLabel(this);
    icon->setPixmap(QIcon(":/icon/fail.svg").pixmap(128, 128));
    icon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel1 = new QLabel("备份失败", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    QLabel *tipLabel2 = new QLabel("xxxxxxx,信息备份失败", this);
    tipLabel2->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ((QHBoxLayout *)(this->layout()))->addSpacing(100);
    this->layout()->addWidget(icon);
    ((QHBoxLayout *)(this->layout()))->addSpacing(10);
    this->layout()->addWidget(tipLabel1);
    ((QHBoxLayout *)(this->layout()))->addSpacing(0);
    this->layout()->addWidget(tipLabel2);
    ((QHBoxLayout *)(this->layout()))->addSpacing(120);
}

void zipFileProcessResultWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        // stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void zipFileProcessResultWidget::informationPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        // stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void zipFileProcessResultWidget::exit()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        // stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
