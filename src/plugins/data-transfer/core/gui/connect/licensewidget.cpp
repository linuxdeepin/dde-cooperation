#include "licensewidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>

#pragma execution_character_set("utf-8")

LicenseWidget::LicenseWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

LicenseWidget::~LicenseWidget()
{
}

void LicenseWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *textLabel1 = new QLabel("UOS迁移工具最终用户许可协议", this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel1->setFont(font);
    textLabel1->setAlignment(Qt::AlignCenter);

    QLabel *textLabel2 = new QLabel("版本更新日期：[2023]年 [10]月[18]日", this);
    textLabel2->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    textLabel2->setFont(font);

    QLabel *licenseLabel = new QLabel(this);
    font.setPointSize(9);
    licenseLabel->setFont(font);
    QString license;
    for (int i = 0; i < 800; i++) {
        license += "协议内容协议内容";
    }
    licenseLabel->setText(license);
    licenseLabel->setWordWrap(true);
    licenseLabel->setFixedWidth(680);

    QHBoxLayout *licenseLayout = new QHBoxLayout();
    licenseLayout->addWidget(licenseLabel);
    licenseLayout->setAlignment(Qt::AlignCenter);

    QLabel *label = new QLabel(this);
    font.setPointSize(9);
    label->setFont(font);
    label->setText("我已阅读并同意<a href=\"https://\" style=\"text-decoration:none;\">《UOS迁移工具》</a>");

    checkBox = new QCheckBox(this);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("取消");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &LicenseWidget::backPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText("确定");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: rgba(0, 152, 255, 0.12);");
    connect(nextButton, &QToolButton::clicked, this, &LicenseWidget::nextPage);

    nextButton->setEnabled(false);

    connect(checkBox, &QCheckBox::stateChanged, this, [nextButton](int state) {
        if (state == Qt::Checked) {
            nextButton->setEnabled(true);
            nextButton->setStyleSheet("background-color: rgb(0, 152, 255);");
        } else {
            nextButton->setEnabled(false);
            nextButton->setText("下一步");
            nextButton->setStyleSheet("background-color: rgba(0, 152, 255, 0.12);");
        }
    });

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(checkBox);
    buttonLayout->addWidget(label);
    buttonLayout->addSpacing(300);
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(textLabel1);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(textLabel2);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(licenseLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
}

void LicenseWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void LicenseWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void LicenseWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        //dark
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
    }
}
