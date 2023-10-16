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
    titileLabel->setStyleSheet("color: black;"
                               "font-size: 24px;"
                               "font-weight: 700;");
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    processTextBrowser = new QTextBrowser(this);
    processTextBrowser->setFixedSize(460, 122);
    processTextBrowser->setReadOnly(true);
    processTextBrowser->setLineWrapMode(QTextBrowser::NoWrap);
    processTextBrowser->setContextMenuPolicy(Qt::NoContextMenu);
    processTextBrowser->setStyleSheet("QTextBrowser {"
                                      "padding-top: 10px;"
                                      "padding-bottom: 10px;"
                                      "padding-left: 5px;"
                                      "padding-right: 5px;"
                                      "font-size: 12px;"
                                      "font-weight: 400;"
                                      "color: rgb(82, 106, 127);"
                                      "line-height: 300%;"
                                      "background-color:rgba(0, 0, 0,0.08);}");
    processTextBrowser->append("迁移完成！！！");

    QHBoxLayout *textBrowerlayout = new QHBoxLayout(this);
    textBrowerlayout->setAlignment(Qt::AlignCenter);
    textBrowerlayout->addWidget(processTextBrowser);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &SuccessWidget::nextPage);

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

    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(40);
    mainLayout->addLayout(textBrowerlayout);
    mainLayout->addLayout(buttonLayout);
}

void SuccessWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void SuccessWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        //dark
    }
}
