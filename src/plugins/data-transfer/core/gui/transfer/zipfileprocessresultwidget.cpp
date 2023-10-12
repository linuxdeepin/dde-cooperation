#include "zipfileprocessresultwidget.h"
#include "../type_defines.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QToolButton>
#include <QDebug>
#include <QLabel>
#include <QApplication>

#pragma execution_character_set("utf-8")

ZipFileProcessResultWidget::ZipFileProcessResultWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ZipFileProcessResultWidget::~ZipFileProcessResultWidget() { }

void ZipFileProcessResultWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    successed();
    exitButton = new QToolButton(this);
    exitButton->setText("退出");
    exitButton->setFixedSize(120, 35);
    exitButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                              "border: 1px solid rgba(0,0,0, 0.03);"
                              "opacity: 1;"
                              "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                              "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                              "font-family: \"SourceHanSansSC-Medium\";"
                              "font-size: 14px;"
                              "font-weight: 500;"
                              "color: rgba(65,77,104,1);"
                              "font-style: normal;"
                              "letter-spacing: 3px;"
                              "text-align: center;"
                              "}");
    exitButton->setEnabled(false);
    QObject::connect(exitButton, &QToolButton::clicked, this, &ZipFileProcessResultWidget::exit);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                              "border: 1px solid rgba(0,0,0, 0.03);"
                              "opacity: 1;"
                              "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                              "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                              "font-family: \"SourceHanSansSC-Medium\";"
                              "font-size: 14px;"
                              "font-weight: 500;"
                              "color: rgba(65,77,104,1);"
                              "font-style: normal;"
                              "letter-spacing: 3px;"
                              "text-align: center;"
                              "}");
    QObject::connect(backButton, &QToolButton::clicked, this,
                     &ZipFileProcessResultWidget::backPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(exitButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    mainLayout->addLayout(buttonLayout);
}

void ZipFileProcessResultWidget::successed()
{
    icon = new QLabel(this);
    icon->setPixmap(QIcon(":/icon/success-128.svg").pixmap(128, 128));
    icon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    tipLabel1 = new QLabel("备份成功", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel2 = new QLabel("恭喜您，信息备份成功", this);
    tipLabel2->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QString display = "<a href=\"https://\" style=\"text-decoration:none;\">前往查看</a>";
    displayLabel = new QLabel(display, this);
    displayLabel->setAlignment(Qt::AlignCenter);
    QObject::connect(displayLabel, &QLabel::linkActivated, this,
                     &ZipFileProcessResultWidget::informationPage);

    ((QHBoxLayout *)(this->layout()))->addSpacing(100);
    this->layout()->addWidget(icon);
    ((QHBoxLayout *)(this->layout()))->addSpacing(10);
    this->layout()->addWidget(tipLabel1);
    ((QHBoxLayout *)(this->layout()))->addSpacing(0);
    this->layout()->addWidget(tipLabel2);
    ((QHBoxLayout *)(this->layout()))->addSpacing(100);
    this->layout()->addWidget(displayLabel);
}

void ZipFileProcessResultWidget::upWidgetToFailed()
{
    displayLabel->setVisible(false);
    icon->setPixmap(QIcon(":/icon/fail.svg").pixmap(128, 128));
    tipLabel1->setText("备份失败");
    tipLabel2->setText("xxxxxxx,信息备份失败");
    exitButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                "border: 1px solid rgba(0,0,0, 0.03);"
                                "opacity: 1;"
                                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                                "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                                "font-family: \"SourceHanSansSC-Medium\";"
                                "font-size: 14px;"
                                "font-weight: 500;"
                                "color: rgba(65,77,104,1);"
                                "font-style: normal;"
                                "letter-spacing: 3px;"
                                "text-align: center;"
                                "}");
}

void ZipFileProcessResultWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void ZipFileProcessResultWidget::informationPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        // stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void ZipFileProcessResultWidget::exit()
{
     QApplication::quit();
}
