#include "resultdisplay.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QApplication>
#include <QStandardItemModel>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QStackedWidget>
#include <utils/transferhepler.h>

ResultDisplayWidget::ResultDisplayWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ResultDisplayWidget::~ResultDisplayWidget() {}

void ResultDisplayWidget::initUI()
{
    setStyleSheet(".ResultDisplayWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    iconLabel->setAlignment(Qt::AlignCenter);

    titileLabel = new QLabel(tr("Transfer completed"), this);
    titileLabel->setFont(StyleHelper::font(1));
    titileLabel->setAlignment(Qt::AlignCenter);

    tiptextlabel = new QLabel(this);
    tiptextlabel->setFont(StyleHelper::font(3));
    tiptextlabel->setText(QString("<font color='#526A7F' >%1</font>").arg(tr("Partial information migration failed, please go to UOS for manual transfer")));
    tiptextlabel->setAlignment(Qt::AlignCenter);
    tiptextlabel->setVisible(false);

    processTextBrowser = new QTextBrowser(this);
    processTextBrowser->setFixedSize(460, 122);
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

    QHBoxLayout *textBrowerlayout = new QHBoxLayout();
    textBrowerlayout->setAlignment(Qt::AlignCenter);
    textBrowerlayout->addWidget(processTextBrowser);

    ButtonLayout *buttonLayout = new ButtonLayout();
    QPushButton *backButton = buttonLayout->getButton1();
    backButton->setText(tr("Back"));
    QPushButton *nextButton = buttonLayout->getButton2();
    nextButton->setText(tr("Exit"));

    connect(backButton, &QPushButton::clicked, this, &ResultDisplayWidget::nextPage);
    connect(nextButton, &QPushButton::clicked, qApp, &QApplication::quit);

    mainLayout->addSpacing(40);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(tiptextlabel);
    mainLayout->addLayout(textBrowerlayout);
    mainLayout->addLayout(buttonLayout);

    connect(TransferHelper::instance(), &TransferHelper::addResult, this,
            &ResultDisplayWidget::addResult);
#ifdef linux
    connect(TransferHelper::instance(), &TransferHelper::transferFinished, this, [this] {
        TransferHelper::instance()->sendMessage("add_result", processText);
    });
#endif
}

void ResultDisplayWidget::nextPage()
{
    TransferHelper::instance()->sendMessage("change_page", "startTransfer");
    QTimer::singleShot(1000, this, [this] {
        QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
        if (stackedWidget) {
            if (stackedWidget->currentIndex() == PageName::resultwidget)
                stackedWidget->setCurrentIndex(PageName::choosewidget);
        } else {
            WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                    "nullptr";
        }
    });
}

void ResultDisplayWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        processTextBrowser->setStyleSheet(StyleHelper::textBrowserStyle(1));
    } else {
        // dark
        setStyleSheet(".ResultDisplayWidget{-color: rgb(37, 37, 37); border-radius: 10px;}");
        processTextBrowser->setStyleSheet(StyleHelper::textBrowserStyle(0));
    }
}

void ResultDisplayWidget::addResult(QString name, bool success, QString reason)
{
    QString info, color;
    if (!success) {
        color = "#FF5736";
        setStatus(false);
    } else
        color = "#6199CA";
    name = ellipsizedText(name, 430, QFont());
#ifdef linux
    info = QString("<font color='#526A7F'>&nbsp;&nbsp;&nbsp;%1</font>&nbsp;&nbsp;&nbsp;&nbsp;<font color='%2'>%3</font>")
                   .arg(name, color, reason);
#else
    info = QString("<p style='line-height: 0.5;'><font color='#526A7F'>&nbsp;%1</font>&nbsp;<font color='%2'>%3</font></p>")
                   .arg(name, color, reason);
#endif

    processTextBrowser->append(info);
    QString res = success ? "true" : "false";
    processText.append(name + " " + res + " " + reason + ";");
}

void ResultDisplayWidget::clear()
{
    processTextBrowser->clear();
    processText.clear();
    setStatus(true);
}

void ResultDisplayWidget::setStatus(bool success)
{
    tiptextlabel->setVisible(!success);
    if (success) {
        titileLabel->setText(tr("Transfer completed"));
        iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    } else {
        titileLabel->setText(tr("Transfer completed partially"));
        iconLabel->setPixmap(QIcon(":/icon/success half-96.svg").pixmap(96, 96));
    }
}

QString ResultDisplayWidget::ellipsizedText(const QString &input, int maxLength, const QFont &font)
{
    QFontMetrics fontMetrics(font);
    int textWidth = fontMetrics.horizontalAdvance(input);   // 获取文本的宽度

    if (textWidth <= maxLength) {
        return input;   // 如果文本宽度未超出限制，则直接返回原文本
    } else {
        QString ellipsizedString = fontMetrics.elidedText(input, Qt::ElideMiddle, maxLength);   // 使用省略号替代超出部分
        return ellipsizedString;
    }
}
