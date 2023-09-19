#include "choosewidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>

#pragma execution_character_set("utf-8")

ChooseWidget::ChooseWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ChooseWidget::~ChooseWidget()
{
}

void ChooseWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *textLabel1 = new QLabel("选择信息迁移方式", this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel1->setFont(font);
    textLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ModeItem *winItem = new ModeItem("从windows PC", QIcon ::fromTheme("folder"), this);
    ModeItem *packageItem = new ModeItem("从备份文件导入", QIcon ::fromTheme("folder"), this);

    QHBoxLayout *modeLayout = new QHBoxLayout(this);
    modeLayout->addWidget(winItem, Qt::AlignTop);
    modeLayout->addSpacing(20);
    modeLayout->addWidget(packageItem, Qt::AlignTop);
    modeLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("下一步");
    nextButton->setFixedSize(300, 35);
    nextButton->setStyleSheet("background-color: lightgray;");
    nextButton->setEnabled(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addSpacing(40);
    mainLayout->addWidget(textLabel1);
    mainLayout->addLayout(modeLayout);
    mainLayout->addSpacing(40);
    mainLayout->addLayout(buttonLayout);

    connect(nextButton, &QToolButton::clicked, this, &ChooseWidget::nextPage);
    connect(winItem->checkBox, &QCheckBox::stateChanged, [packageItem, nextButton](int state) {
        if (state == Qt::Checked)
            packageItem->checkBox->setCheckState(Qt::Unchecked);
        if (state == Qt::Checked || packageItem->checkBox->checkState() == Qt::Checked)
            nextButton->setEnabled(true);
        else
            nextButton->setEnabled(false);
    });
    connect(packageItem->checkBox, &QCheckBox::stateChanged, this, [winItem, nextButton](int state) {
        if (state == Qt::Checked)
            winItem->checkBox->setCheckState(Qt::Unchecked);
        if (state == Qt::Checked || winItem->checkBox->checkState() == Qt::Checked)
            nextButton->setEnabled(true);
        else
            nextButton->setEnabled(false);
    });
}

void ChooseWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

ModeItem::ModeItem(QString text, QIcon icon, QWidget *parent)
    : QFrame(parent)
{
    setStyleSheet("background-color: gray; border-radius: 10px;");
    setFixedSize(250, 250);

    checkBox = new QCheckBox(text, this);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(icon.pixmap(120, 120));
    iconLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(checkBox);
    mainLayout->addWidget(iconLabel);
}

ModeItem::~ModeItem()
{
}

void ModeItem::mousePressEvent(QMouseEvent *event)
{
    if (checkBox->checkState() == Qt::Checked)
        checkBox->setCheckState(Qt::Unchecked);
    else
        checkBox->setCheckState(Qt::Checked);
    return QFrame::mousePressEvent(event);
}
