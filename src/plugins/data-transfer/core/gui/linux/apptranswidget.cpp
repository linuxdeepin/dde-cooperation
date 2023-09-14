#include "apptranswidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>

AppTransWidget::AppTransWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

AppTransWidget::~AppTransWidget()
{
}

void AppTransWidget::initUI()
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

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("确定");
    nextButton->setFixedSize(300, 35);
    nextButton->setStyleSheet("background-color: blue;");
    connect(nextButton, &QToolButton::clicked, this, &AppTransWidget::nextPage);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(layout1);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(layout);
}

void AppTransWidget::initSelectFrame()
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

void AppTransWidget::sendOptions()
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

void AppTransWidget::nextPage()
{
    //send useroptions
    sendOptions();

    //nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
