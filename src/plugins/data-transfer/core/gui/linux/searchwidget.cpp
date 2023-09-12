#include "searchwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>

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

    QLabel *tipLabel = new QLabel("已查找到的设备：", this);
    tipLabel->setFixedHeight(60);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel->setFont(font);

    QHBoxLayout *layout1 = new QHBoxLayout(this);
    layout1->addSpacing(175);
    layout1->addWidget(tipLabel);

    userlayout = new QGridLayout(this);
    QHBoxLayout *layout2 = new QHBoxLayout(this);
    layout2->addLayout(userlayout);
    initUserlayout();

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
    mainLayout->addLayout(layout);
}

void SearchWidget::initUserlayout()
{
    QLabel *iconLabel1 = new QLabel();
    QPixmap iconPixmap1(QIcon::fromTheme("folder").pixmap(200, 100));
    iconLabel1->setPixmap(iconPixmap1);
    QLabel *descriptionLabel1 = new QLabel("Icon 1 Description");

    QLabel *iconLabel2 = new QLabel();
    QPixmap iconPixmap2(QIcon::fromTheme("folder").pixmap(200, 100));
    iconLabel2->setPixmap(iconPixmap2);
    QLabel *descriptionLabel2 = new QLabel("Icon 2 Description");

    QLabel *iconLabel3 = new QLabel();
    QPixmap iconPixmap3(QIcon::fromTheme("folder").pixmap(200, 100));
    iconLabel3->setPixmap(iconPixmap3);
    QLabel *descriptionLabel3 = new QLabel("Icon 3 Description");

    userlayout->addWidget(iconLabel1, 0, 0);
    userlayout->addWidget(descriptionLabel1, 1, 0);
    userlayout->addWidget(iconLabel2, 0, 1);
    userlayout->addWidget(descriptionLabel2, 1, 1);
    userlayout->addWidget(iconLabel3, 0, 2);
    userlayout->addWidget(descriptionLabel3, 1, 2);
    userlayout->setHorizontalSpacing(50);
    userlayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
}

void SearchWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
