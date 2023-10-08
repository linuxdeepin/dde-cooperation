#include "transferringwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QPropertyAnimation>
#include <QEventLoop>
#include <QPainterPath>

#include <utils/transferhepler.h>

#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

TransferringWidget::TransferringWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

TransferringWidget::~TransferringWidget() { }

void TransferringWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/transfer.png").pixmap(200, 160));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *titileLabel = new QLabel("正在迁移…", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    progressLabel = new ProgressBarLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setProgress(50);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    timeLabel = new QLabel(this);
    int time = 43;
    timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(QString::number(time)));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    font.setPointSize(7);
    timeLabel->setFont(timefont);

    QString file = "/Documents/…….doc";
    fileLabel = new QLabel(
            QString("正在传输<font color='#526A7F'>&nbsp;&nbsp;&nbsp;%1s</font>").arg(file), this);
    fileLabel->setAlignment(Qt::AlignCenter);

    QString display = "<a href=\"https://\" style=\"text-decoration:none;\">显示进程</a>";
    displayLabel = new QLabel(display, this);
    displayLabel->setAlignment(Qt::AlignCenter);
    QObject::connect(displayLabel, &QLabel::linkActivated, this,
                     &TransferringWidget::informationPage);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    fileNameFrame = new QFrame(this);
    fileNameFrame->setFixedSize(500, 250);
    // fileNameFrame->setStyleSheet("background-color:red;");
    processTextBrowser = new QTextBrowser(fileNameFrame);
    processTextBrowser->setFixedSize(500, 250);
    processTextBrowser->setReadOnly(true);
    processTextBrowser->setLineWrapMode(QTextBrowser::NoWrap);
    processTextBrowser->setContextMenuPolicy(Qt::NoContextMenu);
    processTextBrowser->setStyleSheet("QTextBrowser {"
                                      "padding-top: 10px;"
                                      "padding-bottom: 10px;"
                                      "padding-left: 5px;"
                                      "padding-right: 5px;"
                                      "font-size: 16px;"
                                      "font-weight: bold;"
                                      "line-height: 150%;"
                                      "background-color:rgba(0, 0, 0,0.08);}");

    QHBoxLayout *textBrowerlayout = new QHBoxLayout(fileNameFrame);
    fileNameFrame->setLayout(textBrowerlayout);
    textBrowerlayout->addWidget(processTextBrowser);

    mainLayout->setAlignment(Qt::AlignHCenter);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(7);
    mainLayout->addWidget(timeLabel);
    mainLayout->addWidget(fileLabel);
    mainLayout->addWidget(displayLabel);
    mainLayout->addWidget(fileNameFrame);
    mainLayout->addSpacing(5);
    mainLayout->addLayout(indexLayout);

    updateProcessTextBrowser(QString("正在传输.../XXX/XXXX/Documents/......doc"));
    updateProcessTextBrowser(QString("正在传输.../XXX/XXXX/Documents/......doc"));
    fileNameFrame->setVisible(false);
}


void TransferringWidget::informationPage()
{
    if (!isVisible) {
        isVisible = true;
        iconLabel->setVisible(false);
        fileLabel->setVisible(false);
        fileNameFrame->setVisible(true);

        QString display = "<a href=\"https://\" style=\"text-decoration:none;\">隐藏进程</a>";
        displayLabel->setText(display);
        QPropertyAnimation *showAnimation = new QPropertyAnimation(processTextBrowser, "pos");
        showAnimation->setDuration(200);
        showAnimation->setStartValue(QPoint(0, 250));
        showAnimation->setEndValue(QPoint(0, 0));
        showAnimation->setEasingCurve(QEasingCurve::Linear);
        showAnimation->start();

    } else {
        isVisible = false;

        QString display = "<a href=\"https://\" style=\"text-decoration:none;\">显示进程</a>";
        displayLabel->setText(display);

        QPropertyAnimation *hideAnimation = new QPropertyAnimation(processTextBrowser, "pos");
        hideAnimation->setDuration(100);
        hideAnimation->setStartValue(QPoint(0, 0));
        hideAnimation->setEndValue(QPoint(0, 250));
        hideAnimation->setEasingCurve(QEasingCurve::Linear);

        QEventLoop loop;
        QObject::connect(hideAnimation, &QPropertyAnimation::finished, &loop, &QEventLoop::quit);
        hideAnimation->start();
        loop.exec();

        iconLabel->setVisible(true);
        fileLabel->setVisible(true);
        fileNameFrame->setVisible(false);
    }
}

void TransferringWidget::changeTimeLabel(const QString &time)
{
    timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(time));
}

void TransferringWidget::changeProgressLabel(const int &ratio)
{
    progressLabel->setProgress(ratio);
}

void TransferringWidget::updateProcessTextBrowser(const QString &process)
{
    processTextBrowser->append(process);
}
