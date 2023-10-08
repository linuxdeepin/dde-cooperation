#include "successwidget.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QApplication>

#include <gui/mainwindow_p.h>
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

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(73, 73));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *titileLabel = new QLabel("迁移完成", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, qApp, &QApplication::quit);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText("退出");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: #0098FF;");
    connect(nextButton, &QToolButton::clicked, qApp, &QApplication::quit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(100);
    mainLayout->addLayout(buttonLayout);
}
