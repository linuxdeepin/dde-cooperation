#include "startwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>

#pragma execution_character_set("utf-8")

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
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon::fromTheme("folder").pixmap(200, 100));
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *titileLabel = new QLabel("UOS迁移工具", this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipLabel1 = new QLabel("uos迁移工具,一键将您的文件，个人数据和应用数据迁移到\nUOS，祝您无缝更换系统。", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    QLabel *tipLabel2 = new QLabel("若要开始，请点击“下一步”", this);
    tipLabel2->setAlignment(Qt::AlignCenter);
    tipLabel2->setFont(font);

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("下一步");
    nextButton->setFixedSize(300, 35);
    nextButton->setStyleSheet("background-color: lightgray;");
    connect(nextButton, &QToolButton::clicked, this, &StartWidget::nextPage);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addWidget(tipLabel2);
    mainLayout->addLayout(layout);
}

void StartWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
