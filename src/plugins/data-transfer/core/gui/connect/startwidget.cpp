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
    setStyleSheet(".StartWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/picture-home.png").pixmap(200, 160));
    iconLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    AdaptFontLabel *titileLabel = new AdaptFontLabel(tr("UOS data transfer"),AdaptFontLabel::fontstyle1,this);
    //titileLabel->setFont(StyleHelper::font(1));
    titileLabel->setAlignment(Qt::AlignCenter);


    AdaptFontLabel *textLabel2 = new AdaptFontLabel(tr("UOS transfer tool enables one click migration of your files, personal data, and applications to\nUOS, helping you seamlessly replace your system."),

                                                    AdaptFontLabel::fontstyle5,this);
    textLabel2->setAlignment(Qt::AlignTop | Qt::AlignCenter);
//    textLabel2->setFont(StyleHelper::font(4));

    ButtonLayout *buttonLayout = new ButtonLayout();
    buttonLayout->setCount(1);
    QPushButton *nextButton = buttonLayout->getButton1();
    nextButton->setText(tr("Next"));
    connect(nextButton, &QPushButton::clicked, this, &StartWidget::nextPage);

    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(titileLabel);
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
        setStyleSheet(".StartWidget{background-color: white; border-radius: 10px;}");
    } else {
        //dark
        setStyleSheet(".StartWidget{background-color: rgb(37, 37, 37); border-radius: 10px;}");
    }
}

