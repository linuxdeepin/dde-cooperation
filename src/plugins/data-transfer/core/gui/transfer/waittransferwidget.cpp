#include "waittransferwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QMovie>

#include <utils/transferhepler.h>

#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

WaitTransferWidget::WaitTransferWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

WaitTransferWidget::~WaitTransferWidget()
{
}

void WaitTransferWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("等待迁移…", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel = new QLabel("请前往windows PC 端选择要传输的内容", this);
    tipLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    iconLabel = new QLabel(this);
    lighticonmovie = new QMovie(this);
    lighticonmovie->setFileName(":/icon/GIF/light/transferring.gif");
    lighticonmovie->setScaledSize(QSize(200, 160));
    lighticonmovie->setSpeed(80);
    lighticonmovie->start();
    darkiconmovie = new QMovie(this);
    darkiconmovie->setFileName(":/icon/GIF/dark/transferring.gif");
    darkiconmovie->setScaledSize(QSize(200, 160));
    darkiconmovie->setSpeed(80);
    darkiconmovie->start();
    iconLabel->setMovie(lighticonmovie);
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    backButton = new QToolButton(this);
    backButton->setText("取消");
    backButton->setFixedSize(250, 36);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &WaitTransferWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(2, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(50);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
}

void WaitTransferWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 2);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void WaitTransferWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void WaitTransferWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
        backButton->setStyleSheet("background-color: lightgray;");
        iconLabel->setMovie(lighticonmovie);
    } else {
        //dark
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        backButton->setStyleSheet("background-color: rgba(0, 0, 0, 0.08);");
        iconLabel->setMovie(darkiconmovie);
    }
}
