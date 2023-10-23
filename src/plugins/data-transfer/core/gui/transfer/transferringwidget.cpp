#include "transferringwidget.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QPropertyAnimation>
#include <QEventLoop>
#include <QPainterPath>
#include <QMovie>
#include <QScrollBar>

#include <utils/transferhepler.h>

#include <gui/connect/choosewidget.h>
#include <utils/optionsmanager.h>
#pragma execution_character_set("utf-8")

TransferringWidget::TransferringWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
    initConnect();
}

TransferringWidget::~TransferringWidget() {}

void TransferringWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    iconLabel = new QLabel(this);
    lighticonmovie = new QMovie(this);
    lighticonmovie->setFileName(":/icon/GIF/light/transferring.gif");
    lighticonmovie->setScaledSize(QSize(200, 160));
    lighticonmovie->setSpeed(80);
    lighticonmovie->start();
    darkiconmovie = new QMovie(this);
    darkiconmovie->setFileName(":/icon/GIF/dark/transferring.gif");
    darkiconmovie->setScaledSize(QSize(200, 160));
    darkiconmovie->setSpeed(80);
    darkiconmovie->start();
    iconLabel->setMovie(lighticonmovie);
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    titileLabel = new QLabel("正在迁移…", this);
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

    QString file = "";
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
                                      "font-size: 12px;"
                                      "font-weight: 400;"
                                      "color: rgb(82, 106, 127);"
                                      "line-height: 300%;"
                                      "background-color:rgba(0, 0, 0,0.08);}");

    processTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    processTextBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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

    fileNameFrame->setVisible(false);
}

void TransferringWidget::initConnect()
{
    connect(TransferHelper::instance(), &TransferHelper::transferContent, this,
            &TransferringWidget::updateProcess);
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

void TransferringWidget::updateProcess(const QString &content, int progressbar, int estimatedtime)
{
#ifdef WIN32
    if (OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0]
        == TransferMethod::kLocalExport) {
        return;
    }
#endif
    QString info =
            QString("正在传输<font color='#526A7F'>&nbsp;&nbsp;&nbsp;%1</font>").arg(content);
    processTextBrowser->append(info);
    progressLabel->setProgress(progressbar);
    fileLabel->setText(info);

    if (content.contains("transfer.json"))
        TransferHelper::instance()->checkSize(content);

    if (estimatedtime == 0) {
        timeLabel->setText("迁移完成");
        fileLabel->setText("迁移完成");
        processTextBrowser->append("迁移完成");
        titileLabel->setText("迁移完成!!!");
    } else if (estimatedtime > 0) {
        if (estimatedtime > 60)
            timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(estimatedtime / 60));
        else
            timeLabel->setText(QString("预计迁移时间还剩 %1秒").arg(estimatedtime));
    } else {
        timeLabel->setText(QString("计算中"));
    }
}

void TransferringWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
        iconLabel->setMovie(lighticonmovie);
    } else {
        //dark
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        iconLabel->setMovie(darkiconmovie);
    }
}
