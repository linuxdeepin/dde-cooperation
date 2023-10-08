#include "errorwidget.h"
#include "transferringwidget.h"

#include <QLabel>
#include <QVBoxLayout>

#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

ErrorWidget::ErrorWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ErrorWidget::~ErrorWidget() { }
void ErrorWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/transfer.png").pixmap(200, 160));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *titileLabel = new QLabel("正在迁移…", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ProgressBarLabel *progressLabel = new ProgressBarLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setProgress(50);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    QLabel *timeLabel = new QLabel(this);
    int time = 43;
    timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(QString::number(time)));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    font.setPointSize(7);
    timeLabel->setFont(timefont);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);


    mainLayout->setAlignment(Qt::AlignHCenter);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(7);
    mainLayout->addWidget(timeLabel);
    mainLayout->addSpacing(5);
    mainLayout->addLayout(indexLayout);
}
