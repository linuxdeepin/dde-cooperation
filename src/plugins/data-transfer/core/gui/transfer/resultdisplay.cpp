#include "resultdisplay.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QApplication>
#include <QStandardItemModel>
#include <QScrollBar>
#include <QTextBrowser>
#include <utils/transferhepler.h>

ResultDisplayWidget::ResultDisplayWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ResultDisplayWidget::~ResultDisplayWidget() {}

void ResultDisplayWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    iconLabel->setAlignment(Qt::AlignCenter);

    titileLabel = new QLabel(tr("Transfer completed"), this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignCenter);

    tiptextlabel = new QLabel(this);
    tiptextlabel->setText(QString("<font size=12px color='#526A7F' >%1</font>").arg(tr("Partial information migration failed, please go to UOS for manual transfer")));
    tiptextlabel->setAlignment(Qt::AlignCenter);
    tiptextlabel->setVisible(false);

    processTextBrowser = new QTextBrowser(this);
    processTextBrowser->setFixedSize(460, 122);
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

    QToolButton *backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &ResultDisplayWidget::nextPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText(tr("Exit"));
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: #0098FF;");
    connect(nextButton, &QToolButton::clicked, qApp, &QApplication::quit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

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
}

void ResultDisplayWidget::nextPage()
{
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void ResultDisplayWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        // dark
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
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
    info = QString("<font color='#526A7F'>&nbsp;&nbsp;&nbsp;%1</font>&nbsp;&nbsp;&nbsp;&nbsp;<font color='%2'>%3</font>")
                   .arg(name, color, reason);
    processTextBrowser->append(info);
}

void ResultDisplayWidget::clear()
{
    processTextBrowser->clear();
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
