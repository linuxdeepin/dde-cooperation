#include "configselectwidget.h"

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

inline constexpr char internetMethodSelectConfigName[] {"请选择要同步的配置"};
inline constexpr char localFileMethodSelectConfigName[] {"请选择要备份的配置"};

ConfigSelectWidget::ConfigSelectWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ConfigSelectWidget::~ConfigSelectWidget() { }

void ConfigSelectWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel(internetMethodSelectConfigName, this);
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

    QLabel *tipLabel1 = new QLabel("请选择需要同步的用户配置，将为您配置到\nUOS中。", this);
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
    QObject::connect(determineButton, &QToolButton::clicked, this, &ConfigSelectWidget::nextPage);

    QToolButton *cancelButton = new QToolButton(this);
    cancelButton->setText("取消");
    cancelButton->setFixedSize(120, 35);
    cancelButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(cancelButton, &QToolButton::clicked, this, &ConfigSelectWidget::backPage);

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

void ConfigSelectWidget::initSelectFrame()
{
    selectLayout = new QGridLayout(this);

    QLabel *label = new QLabel("浏览器", selectFrame);
    QCheckBox *box = new QCheckBox("书签", selectFrame);
    selectLayout->addWidget(label, 0, 0);
    selectLayout->addWidget(box, 0, 1);

    QLabel *label1 = new QLabel("个人配置", selectFrame);
    QCheckBox *box1 = new QCheckBox("自定义桌面", selectFrame);
    QCheckBox *box2 = new QCheckBox("自定义屏保", selectFrame);
    selectLayout->addWidget(label1, 1, 0);
    selectLayout->addWidget(box1, 1, 1);
    selectLayout->addWidget(box2, 2, 1);

    selectLayout->setHorizontalSpacing(30);
    selectLayout->setVerticalSpacing(10);
    selectLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QHBoxLayout *layout2 = new QHBoxLayout(this);
    layout2->addSpacing(80);
    layout2->addLayout(selectLayout);

    selectFrame->setLayout(layout2);
    selectFrame->setStyleSheet("background-color: lightgray; border-radius: 8px;");
    selectFrame->setFixedWidth(450);
}

void ConfigSelectWidget::sendOptions()
{
    QStringList config;
    for (int i = 0; i < selectLayout->rowCount(); i++) {
        QCheckBox *checkBox =
                qobject_cast<QCheckBox *>(selectLayout->itemAtPosition(i, 1)->widget());
        if (checkBox && checkBox->isChecked()) {
            const QString dir = checkBox->text();
            config.append(dir);
        }
    }
    qInfo() << "select config :" << config;
    OptionsManager::instance()->addUserOption(Options::kConfig, config);
}

void ConfigSelectWidget::nextPage()
{
    // send useroptions
    sendOptions();

    // start
    TransferHelper::instance()->startTransfer();

    // nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
void ConfigSelectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
