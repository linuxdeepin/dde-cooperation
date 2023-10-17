#include "promptwidget.h"
#include "../type_defines.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>

#pragma execution_character_set("utf-8")

PromptWidget::PromptWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

PromptWidget::~PromptWidget()
{
}

void PromptWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *textLabel = new QLabel("开始之前", this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel->setFont(font);
    textLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QStringList prompts {
        "迁移需要一定的时间，避免断电中断迁移，请插上电源。",
        "其他应用可能会干扰迁移速度，为了迁移更流畅，请关闭其他应用。",
        "建议暂时关闭UOS自动更新程序，以免打断迁移过程。",
    };

    QGridLayout *gridLayout = new QGridLayout();
    for (int i = 0; i < prompts.count(); i++) {
        QLabel *iconlabel = new QLabel(this);
        iconlabel->setPixmap(QIcon(":/icon/dialog-warning.svg").pixmap(14, 14));

        QLabel *textlabel = new QLabel(prompts[i], this);

        gridLayout->addWidget(iconlabel, i, 0);
        gridLayout->addWidget(textlabel, i, 1);
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(10);
        gridLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    }

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &PromptWidget::backPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText("确定");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: #0098FF;");
    connect(nextButton, &QToolButton::clicked, this, &PromptWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignCenter);

    mainLayout->addSpacing(30);
    mainLayout->addWidget(textLabel);
    mainLayout->addLayout(gridLayout);
    mainLayout->addSpacing(250);
    mainLayout->addLayout(buttonLayout);
}

void PromptWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
#ifdef _WIN32
        stackedWidget->setCurrentIndex(PageName::readywidget);
#else
        stackedWidget->setCurrentIndex(PageName::waitgwidget);
#endif
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void PromptWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void PromptWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        //dark
    }
}
