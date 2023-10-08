#include "transferringwidget.h"
#include "zipfileprocesswidget.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>
#include <QTextBrowser>

#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

zipFileProcessWidget::zipFileProcessWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

zipFileProcessWidget::~zipFileProcessWidget() { }

void zipFileProcessWidget::changeFileLabel(const QString &path)
{
    fileLabel->setText(QString("正在打包 %1").arg(path));
}

void zipFileProcessWidget::changeTimeLabel(const QString &time)
{
    timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(time));
}

void zipFileProcessWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/zipfileprocess.png").pixmap(200, 160));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *titileLabel = new QLabel("正在创建备份文件…", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignHCenter);

    ProgressBarLabel *progressLabel = new ProgressBarLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setProgress(50);

    QHBoxLayout *progressLayout = new QHBoxLayout(this);
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    timeLabel = new QLabel(this);
    int time = 43;
    timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(QString::number(time)));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    font.setPointSize(7);
    timeLabel->setFont(timefont);

    fileLabel = new QLabel(this);
    fileLabel->setText(QString("正在打包 %1").arg(QString("/Documents/....doc")));
    fileLabel->setAlignment(Qt::AlignCenter);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->setAlignment(Qt::AlignHCenter);

    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(progressLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(timeLabel);
    mainLayout->addWidget(fileLabel);
    mainLayout->addLayout(indexLayout);
}
