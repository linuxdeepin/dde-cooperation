#include "appselectwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
#include <gui/mainwindow_p.h>
#pragma execution_character_set("utf-8")

AppSelectWidget::AppSelectWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

AppSelectWidget::~AppSelectWidget() { }

void AppSelectWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("同步应用", this);
    titileLabel->setFixedHeight(40);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QHBoxLayout *layout1 = new QHBoxLayout(this);
    selectFrame = new QFrame(this);
    layout1->addWidget(selectFrame, Qt::AlignCenter);
    initSelectFrame();

    QLabel *tipLabel1 = new QLabel("如果勾选同步应用，会自动为你下载对应的UOS\n版应用程序", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel1->setFixedHeight(80);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    QToolButton *determineButton = new QToolButton(this);
    QPalette palette = determineButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    determineButton->setPalette(palette);
    determineButton->setText("确定");
    determineButton->setFixedSize(120, 35);
    determineButton->setStyleSheet("background-color: #0098FF;");
    QObject::connect(determineButton, &QToolButton::clicked, this, &AppSelectWidget::nextPage);

    QToolButton *cancelButton = new QToolButton(this);
    cancelButton->setText("取消");
    cancelButton->setFixedSize(120, 35);
    cancelButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(cancelButton, &QToolButton::clicked, this, &AppSelectWidget::backPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(determineButton);
    buttonLayout->setAlignment(Qt::AlignHCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(layout1);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(20);
}

void AppSelectWidget::initSelectFrame()
{
    selectLayout = new QGridLayout(this);
    auto appList = TransferHelper::instance()->getAppList();
    QList<QString> appName = appList.keys();

    for (int i = 0; i < appList.count(); i++) {
        const QString name = appName[i];
        const QString icon = appList[name];
        QCheckBox *checkBox = new QCheckBox(name, selectFrame);
        checkBox->setIcon(QIcon::fromTheme(icon));
        checkBox->setIconSize(QSize(30, 30));
        selectLayout->addWidget(checkBox, i, 0);
    }

    selectLayout->setHorizontalSpacing(70);
    selectLayout->setVerticalSpacing(20);
    selectLayout->setAlignment(Qt::AlignCenter);

    selectFrame->setLayout(selectLayout);
    selectFrame->setStyleSheet("background-color: lightgray; border-radius: 8px;");
    selectFrame->setFixedWidth(450);
}

void AppSelectWidget::sendOptions()
{
    QStringList appName;
    for (int i = 0; i < selectLayout->count(); i++) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(selectLayout->itemAt(i)->widget());
        if (checkBox && checkBox->isChecked()) {
            const QString dir = checkBox->text();
            appName.append(dir);
        }
    }
    qInfo() << "select app :" << appName;
    OptionsManager::instance()->addUserOption(Options::kApp, appName);
}

void AppSelectWidget::nextPage()
{
    // send useroptions
    sendOptions();

    // nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
void AppSelectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
