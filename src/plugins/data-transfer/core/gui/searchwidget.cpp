#include "searchwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>

#pragma execution_character_set("utf-8")

SearchWidget::SearchWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

SearchWidget::~SearchWidget()
{
}

void SearchWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("正在搜索......", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *textLabel = new QLabel("已查找到的设备：", this);
    textLabel->setFixedHeight(60);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    textLabel->setFont(font);

    QHBoxLayout *layout1 = new QHBoxLayout(this);
    layout1->addSpacing(175);
    layout1->addWidget(textLabel);

    userlayout = new QGridLayout(this);
    QHBoxLayout *layout2 = new QHBoxLayout(this);
    layout2->addLayout(userlayout);
    updateUserlayout();

    tipLabel = new QLabel("请选择连接的对象", this);
    tipLabel->setVisible(false);
    tipLabel->setAlignment(Qt::AlignCenter);

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("下一步");
    nextButton->setFixedSize(300, 35);
    nextButton->setStyleSheet("background-color: lightgray;");
    connect(nextButton, &QToolButton::clicked, this, &SearchWidget::nextPage);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(layout1);
    mainLayout->addLayout(layout2);
    mainLayout->addWidget(tipLabel);
    mainLayout->addLayout(layout);
}

void SearchWidget::updateUserlayout()
{
    QStringList userList = TransferHelper::instance()->getUesr();
    for (int i = 0; i < userList.count(); i++) {
        Useritem *item = new Useritem(userList[i], this);
        userlayout->addWidget(item, 0, i);
    }
    userlayout->setHorizontalSpacing(50);
    userlayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
}

void SearchWidget::nextPage()
{
    if (OptionsManager::instance()->getUserOption(Options::kUser).isEmpty()) {
        qInfo() << "No connection user selected";
        tipLabel->setVisible(true);
        return;
    }

    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void SearchWidget::setTip(bool status) const
{
    tipLabel->setVisible(status);
}

Useritem::Useritem(QString name, QWidget *parent)
    : QWidget(parent), name(name)
{
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon::fromTheme("folder").pixmap(200, 100));

    QLabel *descriptionLabel = new QLabel(name, this);
    descriptionLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(descriptionLabel);
}

Useritem::~Useritem()
{
}

void Useritem::mousePressEvent(QMouseEvent *event)
{
    OptionsManager::instance()->addUserOption(Options::kUser, QStringList() << name);
    qInfo() << "select user :" << name;

    SearchWidget *parent = qobject_cast<SearchWidget *>(this->parent());
    if (parent) {
        parent->setTip(false);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }

    return QWidget::mousePressEvent(event);
}
