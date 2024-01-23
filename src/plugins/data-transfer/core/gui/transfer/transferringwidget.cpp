#include "transferringwidget.h"
#include "../type_defines.h"
#include "errorwidget.h"

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
#include <utils/optionsmanager.h>

TransferringWidget::TransferringWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
    initConnect();
}

TransferringWidget::~TransferringWidget() {}

void TransferringWidget::initUI()
{
    setStyleSheet(".TransferringWidget{background-color: white; border-radius: 10px;}");

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

    titileLabel = new QLabel(tr("Transferring..."), this);
    titileLabel->setFixedHeight(50);
    titileLabel->setFont(StyleHelper::font(2));
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    progressLabel = new ProgressBarLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setProgress(0);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    timeLabel = new QLabel(this);
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    timefont.setPointSize(12);
    timeLabel->setFont(timefont);

    fileLabel = new QLabel(this);
    fileLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setText(QString(tr("Calculationing...")));

    QString display = QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>")
                              .arg(tr("Show processes"));
    displayLabel = new QLabel(display, this);
    displayLabel->setAlignment(Qt::AlignCenter);
    QObject::connect(displayLabel, &QLabel::linkActivated, this,
                     &TransferringWidget::initInformationPage);

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
    processTextBrowser->setStyleSheet(StyleHelper::textBrowserStyle(1));

    QString scrollBarStyle = "QScrollBar:vertical {"
                             "width: 6px;"
                             "background: #c0c0c0;"
                             "border-radius: 3px;"
                             "}"
                             "QScrollBar::handle:vertical {"
                             "background: #a0a0a0;"
                             "border-radius: 3px;"
                             "}"
                             "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
                             "background: #c0c0c0;"
                             "border-radius: 3px;"
                             "}";
    processTextBrowser->verticalScrollBar()->setStyleSheet(scrollBarStyle);

    processTextBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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
    mainLayout->addSpacing(30);
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
    connect(TransferHelper::instance(), &TransferHelper::disconnected, this,
            &TransferringWidget::clear);
}

void TransferringWidget::initInformationPage()
{
    if (!isVisible) {
        isVisible = true;
        iconLabel->setVisible(false);
        fileLabel->setVisible(false);
        fileNameFrame->setVisible(true);

        QString display = QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>")
                                  .arg(tr("Hide processes"));
        displayLabel->setText(display);
        QPropertyAnimation *showAnimation = new QPropertyAnimation(processTextBrowser, "pos");
        showAnimation->setDuration(200);
        showAnimation->setStartValue(QPoint(0, 250));
        showAnimation->setEndValue(QPoint(0, 0));
        showAnimation->setEasingCurve(QEasingCurve::Linear);
        showAnimation->start();

    } else {
        isVisible = false;

        QString display = QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>")
                                  .arg(tr("Show processes"));
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
    timeLabel->setText(QString(tr("Transfer will be completed in %1 minutes")).arg(time));
}

void TransferringWidget::changeProgressLabel(const int &ratio)
{
    progressLabel->setProgress(ratio);
}

void TransferringWidget::updateProcess(const QString &tpye, const QString &content, int progressbar,
                                       int estimatedtime)
{
#ifdef WIN32
    if (OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0]
        == TransferMethod::kLocalExport) {
        return;
    }
#else
    if (tpye == tr("Transfering") && content.contains("transfer.json"))
        TransferHelper::instance()->checkSize(content);
#endif

    if (estimatedtime == -1)
        return;

    //处理过程内容，只显示一级目录文件
    QString str = resetContent(tpye, content);

    if (!str.isEmpty()) {
        QString info = QString("<img src=':/icon/success-128.svg' width='12' height='12' style='vertical-align: baseline;'>"
                               "<font color='#526A7F'style='vertical-align: baseline;'>&nbsp;&nbsp;&nbsp;%1</font>"
                               "&nbsp;&nbsp;<font color='#6199CA' style='vertical-align: baseline;'>%2</font>")
                               .arg(str, tpye);
        processTextBrowser->append(info);
        fileLabel->setText(QString("%1<font style='color: rgba(0, 0, 0, 0.6);'>&nbsp;&nbsp;&nbsp;%2</font>").arg(tpye, str));
    }

    progressLabel->setProgress(progressbar);

    timeLabel->setText(QString(tr("Calculationing...")));
    if (estimatedtime > 0) {
        titileLabel->setText(tr("Transferring..."));
        if (estimatedtime > 60)
            timeLabel->setText(QString(tr("Transfer will be completed in %1 minutes"))
                                       .arg(estimatedtime / 60));
        else
            timeLabel->setText(
                    QString(tr("Transfer will be completed in %1 secondes")).arg(estimatedtime));
    }
    if (estimatedtime == -2) {
        timeLabel->setText(QString(tr("Transfer will be completed in --")));
    }
}

void TransferringWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet(".TransferringWidget{background-color: white; border-radius: 10px;}");
        iconLabel->setMovie(lighticonmovie);
    } else {
        // dark
        setStyleSheet(".TransferringWidget{background-color: rgb(37, 37, 37); border-radius: 10px;}");
        iconLabel->setMovie(darkiconmovie);
    }
}

void TransferringWidget::clear()
{
    processTextBrowser->clear();
    progressLabel->setProgress(0);
    timeLabel->setText(tr("Calculationing..."));
    titileLabel->setText(tr("Transferring..."));
    fileLabel->setText("");
    finishJobs.clear();
}

QString TransferringWidget::resetContent(const QString &type, const QString &content)
{
    if (!type.startsWith(tr("Decompressing")) && !type.startsWith(tr("Transfering")))
        return content;

    QString res = content;

    if (finishJobs.isEmpty()) {
        if (type.startsWith(tr("Transfering"))) {
            QStringList parts = content.split("/");
            if (parts.size() > 3)
                res = "/" + parts[1] + "/" + parts[2] + "/" + parts[3];
        }
        finishJobs.append(res);
        return QString();
    }

    res = getTransferFileName(content, finishJobs.first());
    if (finishJobs.contains(res))
        return QString();
    else
        finishJobs.append(res);
    return res;
}

QString TransferringWidget::getTransferFileName(const QString &fullPath, const QString &targetPath)
{
    std::string path = fullPath.toStdString();
    std::string toRemove = targetPath.toStdString();

    size_t found = path.find(toRemove);   // 查找子字符串的位置
    if (found != std::string::npos) {   // 如果找到了子字符串
        std::string result = path.substr(found + toRemove.length() + 1);   // 截取子字符串之后的部分
        found = result.find('/');   // 查找第一个路径名
        if (found != std::string::npos) {
            result = result.substr(0, found);   // 截取第一个路径名
        }
        return QString::fromStdString(result);
    } else {
        return QString();
    }
}
