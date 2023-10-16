#include "transferringwidget.h"
#include "zipfileprocesswidget.h"
#include "zipfileprocessresultwidget.h"
#include "../type_defines.h"
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>
#include <QTextBrowser>
#include <QMovie>
#include <QStackedWidget>

#include <gui/connect/choosewidget.h>
#include <utils/transferhepler.h>

#pragma execution_character_set("utf-8")

zipFileProcessWidget::zipFileProcessWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

zipFileProcessWidget::~zipFileProcessWidget() { }

void zipFileProcessWidget::updateProcess(const QString &content, int processbar, int estimatedtime)
{
    if(processbar == -1)
    {
        QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
        ZipFileProcessResultWidget *widgeZipfileresutlt = qobject_cast<ZipFileProcessResultWidget *>(stackedWidget->widget(PageName::zipfileprocessresultwidget));
        widgeZipfileresutlt->upWidgetToFailed();
        nextPage();
        return;
    }
    if(processbar ==100)
    {
        nextPage();
        return;
    }
    changeFileLabel(content);
    changeTimeLabel(estimatedtime);
    changeProgressBarLabel(processbar);
}

void zipFileProcessWidget::changeFileLabel(const QString &path)
{
    fileLabel->setText(QString("正在打包 %1").arg(path));
}

void zipFileProcessWidget::changeTimeLabel(const int &time)
{
    timeLabel->setText(QString("预计迁移时间还剩 %1分钟").arg(QString::number(time)));
}

void zipFileProcessWidget::changeProgressBarLabel(const int &processbar)
{
    progressLabel->setProgress(processbar);
}

void zipFileProcessWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QMovie *iconmovie = new QMovie(this);
    iconmovie->setFileName(":/icon/GIF/transferring.gif");
    iconmovie->setScaledSize(QSize(200, 160));
    iconmovie->setSpeed(80);
    iconmovie->start();
    iconLabel->setMovie(iconmovie);

    QLabel *titileLabel = new QLabel("正在创建备份文件…", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignHCenter);

    progressLabel = new ProgressBarLabel(this);
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

    QObject::connect(TransferHelper::instance(), &TransferHelper::transferContent, this,
                     &zipFileProcessWidget::updateProcess);
}

void zipFileProcessWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::zipfileprocessresultwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
