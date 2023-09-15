#include "transferringwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>

#include <utils/transferhepler.h>

#pragma execution_character_set("utf-8")

TransferringWidget::TransferringWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

TransferringWidget::~TransferringWidget()
{
}

void TransferringWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("正在发送......", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon::fromTheme("folder").pixmap(200, 100));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *userLabel = new QLabel("windows-user", this);
    userLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QLabel *fileLabel = new QLabel("正在传输：\n /Documents/xxx.doc", this);
    fileLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(userLabel);
    mainLayout->addWidget(fileLabel);
}

void TransferringWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
