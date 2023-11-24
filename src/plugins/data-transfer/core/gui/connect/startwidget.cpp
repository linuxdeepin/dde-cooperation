#include "startwidget.h"
#include "../type_defines.h"
#include <utils/transferhepler.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>

StartWidget::StartWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

StartWidget::~StartWidget()
{
}

void StartWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/picture-home.png").pixmap(200, 160));
    iconLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    QLabel *textLabel1 = new QLabel(tr("UOS data transfer"), this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel1->setFont(font);
    textLabel1->setAlignment(Qt::AlignCenter);

    QLabel *textLabel2 = new QLabel(tr("UOS transfer tool enables one click migration of your files, personal data, and applications to\nUOS, helping you seamlessly replace your system."), this);
    textLabel2->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    textLabel2->setFont(font);

    nextButton = new QToolButton(this);
    nextButton->setText(tr("Next"));
    nextButton->setFixedSize(250, 36);
    nextButton->setStyleSheet("background-color: lightgray;");
    connect(nextButton, &QToolButton::clicked, this, &StartWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel1);
    mainLayout->addWidget(textLabel2);
    mainLayout->addSpacing(60);
    mainLayout->addLayout(buttonLayout);
}

void StartWidget::nextPage()
{
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void StartWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
        nextButton->setStyleSheet("background-color: lightgray;");
    } else {
        //dark
        nextButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "opacity: 1;"
                                  "background-color: rgba(255,255,255, 0.1);"
                                  "}");
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
    }
}
