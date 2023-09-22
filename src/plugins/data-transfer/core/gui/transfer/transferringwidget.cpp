#include "transferringwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>

#include <utils/transferhepler.h>

#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

TransferringWidget::TransferringWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

TransferringWidget::~TransferringWidget()
{
}

void TransferringWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
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

    QHBoxLayout *progressLayout = new QHBoxLayout(this);
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    QLabel *timeLabel = new QLabel(this);
    int time = 43;
    timeLabel->setText(QString("预计迁移时间还剩 43分钟").arg(QString::number(time)));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    font.setPointSize(7);
    timeLabel->setFont(timefont);

    QString file = "/Documents/…….doc";
    QLabel *fileLabel = new QLabel(QString("正在接收<font color='#526A7F'>&nbsp;&nbsp;&nbsp;%1s</font>").arg(file), this);
    fileLabel->setAlignment(Qt::AlignCenter);

    QString display = "<a href=\"https://\" style=\"text-decoration:none;\">显示进程</a>";
    QLabel *displayLabel = new QLabel(display, this);
    displayLabel->setAlignment(Qt::AlignCenter);
    connect(displayLabel, &QLabel::linkActivated, this, &TransferringWidget::nextPage);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(7);
    mainLayout->addWidget(timeLabel);
    mainLayout->addWidget(fileLabel);
    mainLayout->addWidget(displayLabel);
    mainLayout->addLayout(indexLayout);
}

void TransferringWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
