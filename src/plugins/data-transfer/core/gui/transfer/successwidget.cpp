#include "successwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>

#pragma execution_character_set("utf-8")

SuccessWidget::SuccessWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

SuccessWidget::~SuccessWidget()
{
}

void SuccessWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("传输成功", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon::fromTheme("folder").pixmap(200, 100));
    iconLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(100);
}
