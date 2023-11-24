#include "promptwidget.h"
#include "../type_defines.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>
#include <utils/transferhepler.h>
PromptWidget::PromptWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

PromptWidget::~PromptWidget() { }

void PromptWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *textLabel = new QLabel(tr("Before tranfer"), this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel->setFont(font);
    textLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QStringList prompts{ tr("Data transfer requires some time, to avoid interrupting the migration "
                            "due to low battery, please keep connect to the  power."),
                         tr("Other applications may slowdown the transfer speed. For smoother "
                            "experience, please close other applications."),
                         tr("For the security of your transfer, please use a trusted network.") };

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    for (int i = 0; i < prompts.count(); i++) {
        QLabel *iconlabel = new QLabel(this);
        iconlabel->setPixmap(QIcon(":/icon/dialog-warning.svg").pixmap(14, 14));

        QLabel *textlabel = new QLabel(prompts[i], this);
        textlabel->setWordWrap(true);
        textlabel->setFixedSize(500, 50);
        gridLayout->addWidget(iconlabel, i, 0);
        gridLayout->addWidget(textlabel, i, 1);
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(10);

    }

    QHBoxLayout *promptLayout = new QHBoxLayout();
    promptLayout->addSpacing(150);
    promptLayout->addLayout(gridLayout);

    backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setFixedSize(120, 35);

    connect(backButton, &QToolButton::clicked, this, &PromptWidget::backPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText(tr("Confirm"));
    nextButton->setFixedSize(120, 35);
#ifdef WIN32
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
                              "text-align: center;"
                              ";}");
    nextButton->setStyleSheet(".QToolButton{"
                              "border-radius: 8px;"
                              "border: 1px solid rgba(0,0,0, 0.03);"
                              "opacity: 1;"
                              "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                              "rgba(37, 183, 255, 1), stop:1 rgba(0, 152, 255, 1));"
                              "font-family: \"SourceHanSansSC-Medium\";"
                              "font-size: 14px;"
                              "font-weight: 500;"
                              "color: rgba(255,255,255,1);"
                              "font-style: normal;"
                              "text-align: center;"
                              "}");
#else
    backButton->setStyleSheet(".QToolButton{background-color: lightgray;border-radius: 8px;}");
    nextButton->setStyleSheet(".QToolButton{background-color: rgba(0, 125, 255, 1);border-radius: 8px;}");
#endif

    connect(nextButton, &QToolButton::clicked, this, &PromptWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignCenter);

    mainLayout->addSpacing(30);
    mainLayout->addWidget(textLabel);
    mainLayout->addSpacing(30);
    mainLayout->addLayout(promptLayout);
    mainLayout->addSpacing(220);
    mainLayout->addLayout(buttonLayout);
}

void PromptWidget::nextPage()
{
#ifdef _WIN32
    emit TransferHelper::instance()->changeWidget(PageName::readywidget);
#else
    emit TransferHelper::instance()->changeWidget(PageName::connectwidget);

#endif

}

void PromptWidget::backPage()
{
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void PromptWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet(".ChooseWidget{ background-color: rgba(255,255,255,1); border-radius: 10px;}");
        backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "background-color: lightgray;"
                                  "}");

    } else {
        // dark
        setStyleSheet(".ChooseWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "opacity: 1;"
                                  "background-color: rgba(255,255,255, 0.1);"
                                  "}");
    }
}
