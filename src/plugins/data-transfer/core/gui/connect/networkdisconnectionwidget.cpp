#include "networkdisconnectionwidget.h"

#include <QLabel>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>
#include "../type_defines.h"
#include <gui/connect/choosewidget.h>
#include <utils/transferhepler.h>
NetworkDisconnectionWidget::NetworkDisconnectionWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

NetworkDisconnectionWidget::~NetworkDisconnectionWidget()
{
}

void NetworkDisconnectionWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/icon/noInternet.png"));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *promptLabel = new QLabel(this);
    promptLabel->setText(tr("The network has been disconnected. Please check your network"));
    promptLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setStyleSheet("background-color: #E3E3E3;");
    backButton->setFixedSize(120, 35);
    QObject::connect(backButton, &QToolButton::clicked, this, &NetworkDisconnectionWidget::backPage);

    QToolButton *retryButton = new QToolButton(this);

    retryButton->setText(tr("Try again"));
    retryButton->setStyleSheet("background-color: #E3E3E3;");
    retryButton->setFixedSize(120, 35);

    QObject::connect(retryButton, &QToolButton::clicked, this, &NetworkDisconnectionWidget::retryPage);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(retryButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(promptLabel);
    mainLayout->addSpacing(170);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
}

void NetworkDisconnectionWidget::backPage()
{
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void NetworkDisconnectionWidget::retryPage()
{
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void NetworkDisconnectionWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        //dark
    }
}
