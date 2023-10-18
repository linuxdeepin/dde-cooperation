#include "resultdisplay.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QApplication>

#include <gui/mainwindow_p.h>
#pragma execution_character_set("utf-8")

ResultDisplayWidget::ResultDisplayWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ResultDisplayWidget::~ResultDisplayWidget()
{
}

void ResultDisplayWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success half-96.svg").pixmap(73, 73));
    iconLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    QLabel *titileLabel = new QLabel("部分迁移完成", this);
    titileLabel->setStyleSheet("color: black;"
                               "font-size: 24px;"
                               "font-weight: 700;");
    titileLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    QHBoxLayout *titilelayout = new QHBoxLayout(this);
    titilelayout->addWidget(iconLabel);
    titilelayout->addSpacing(5);
    titilelayout->addWidget(titileLabel);
    titilelayout->addSpacing(50);

    QLabel *tipiconlabel = new QLabel(this);
    tipiconlabel->setPixmap(QIcon(":/icon/dialog-warning.svg").pixmap(14, 14));

    QLabel *tiptextlabel = new QLabel(this);
    tiptextlabel->setText("<font size=12px color='gray' >部分信息迁移失败，请手动迁移。</font>");

    QHBoxLayout *tiplayout = new QHBoxLayout(this);
    tiplayout->addSpacing(80);
    tiplayout->addWidget(tipiconlabel);
    tiplayout->addWidget(tiptextlabel);
    tiplayout->setAlignment(Qt::AlignLeft);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &ResultDisplayWidget::nextPage);

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

    mainLayout->addSpacing(30);
    mainLayout->addLayout(titilelayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(tiplayout);
    mainLayout->addSpacing(300);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(5);
}

void ResultDisplayWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void ResultDisplayWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        //dark
    }
}
