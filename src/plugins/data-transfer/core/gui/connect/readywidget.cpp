#include "readywidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>
#include <QLineEdit>
#include <QRegularExpressionValidator>

#include <gui/connect/choosewidget.h>

#include <utils/transferhepler.h>

#pragma execution_character_set("utf-8")

ReadyWidget::ReadyWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ReadyWidget::~ReadyWidget() { }

void ReadyWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    QLabel *mainLabel = new QLabel("准备连接", this);
    mainLabel->setStyleSheet("opacity: 1;"
                             "color: rgba(0,26,46,1);"
                             "font-family: \"SourceHanSansSC-Bold\";"
                             "font-size: 24px;"
                             "font-weight: 700;"
                             "font-style: normal;"
                             "text-align: left;");
    mainLabel->setAlignment(Qt::AlignTop | Qt::AlignCenter);

    QLabel *ipLabel = new QLabel("IP地址", this);
    QHBoxLayout *ipLayout = new QHBoxLayout(this);
    ipLayout->addSpacing(200);
    ipLayout->addWidget(ipLabel);
    ipLayout->setAlignment(Qt::AlignBottom);

    ipInput = new QLineEdit(this);
    ipInput->setPlaceholderText("请输入您想要连接的电脑IP地址");
    ipInput->setStyleSheet("border-radius: 8px;"
                           "opacity: 1;"
                           "padding-left: 10px;"
                           "background-color: rgba(0,0,0, 0.08);");
    ipInput->setFixedSize(340, 36);

    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(QRegularExpression(
            "^((\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])\\.){3}(\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])$"));

    ipInput->setValidator(ipValidator);

    QObject::connect(ipInput, &QLineEdit::textChanged, [=]() {
        bool isEmpty = ipInput->text().isEmpty();
        ipInput->setClearButtonEnabled(!isEmpty);
    });
    QHBoxLayout *editLayout1 = new QHBoxLayout(this);
    editLayout1->setAlignment(Qt::AlignLeft);
    editLayout1->addSpacing(200);
    editLayout1->addWidget(ipInput);

    QLabel *cue = new QLabel("您可以通过在另一台电脑上的迁移助手查看IP地址", this);
    QHBoxLayout *cueLayout = new QHBoxLayout(this);
    cueLayout->addSpacing(200);
    cueLayout->addWidget(cue);
    cueLayout->setAlignment(Qt::AlignTop);

    QLabel *Captcha = new QLabel("验证码：", this);
    QHBoxLayout *captchaLayout = new QHBoxLayout(this);
    captchaLayout->addSpacing(200);
    captchaLayout->addWidget(Captcha);
    captchaLayout->setAlignment(Qt::AlignBottom);

    captchaInput = new QLineEdit(this);
    QRegularExpressionValidator *captchaValidator =
            new QRegularExpressionValidator(QRegularExpression("^\\d{6}$"));
    captchaInput->setValidator(captchaValidator);
    captchaInput->setPlaceholderText("请输入PC上显示的验证码");
    captchaInput->setStyleSheet("border-radius: 8px;"
                                "opacity: 1;"
                                "padding-left: 10px;"
                                "background-color: rgba(0,0,0, 0.08);");
    captchaInput->setFixedSize(340, 36);
    QObject::connect(captchaInput, &QLineEdit::textChanged, [=]() {
        bool isEmpty = captchaInput->text().isEmpty();
        captchaInput->setClearButtonEnabled(!isEmpty);
    });

    QHBoxLayout *editLayout2 = new QHBoxLayout(this);
    editLayout2->setAlignment(Qt::AlignLeft);
    editLayout2->addSpacing(200);
    editLayout2->addWidget(captchaInput);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                              "border: 1px solid rgba(0,0,0, 0.03);"
                              "opacity: 1;"
                              "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                              "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                              "font-family: \"SourceHanSansSC-Medium\";"
                              "font-size: 14px;"
                              "font-weight: 500;"
                              "color: rgba(65,77,104,1);"
                              "font-style: normal;"
                              "text-align: center;"
                              ";}");
    connect(backButton, &QToolButton::clicked, this, &ReadyWidget::backPage);

    tiptextlabel = new QLabel(this);
    tiptextlabel->setText("<font size='4' color='#FF5736' >密码或IP错误！请重新输入。</font>");
    tiptextlabel->setVisible(false);
    tiptextlabel->setAlignment(Qt::AlignCenter);

    nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setEnabled(false);
    nextButton->setPalette(palette);
    nextButton->setText("确定");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                              "border: 1px solid rgba(0,0,0, 0.03);"
                              "opacity: 1;"
                              "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                              "rgba(37, 183, 255, 1), stop:1 rgba(0, 152, 255, 1));"
                              "font-family: \"SourceHanSansSC-Medium\";"
                              "font-size: 14px;"
                              "font-weight: 500;"
                              "color: rgba(255,255,255,1);"
                              "font-style: normal;"  
                              "text-align: center;"
                              "}");
    connect(nextButton, &QToolButton::clicked, this, &ReadyWidget::tryConnect);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(1, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addSpacing(20);
    mainLayout->addWidget(mainLabel);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(ipLayout);
    mainLayout->addLayout(editLayout1);
    mainLayout->addLayout(cueLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(captchaLayout);
    mainLayout->addLayout(editLayout2);
    mainLayout->addSpacing(130);
    mainLayout->addWidget(tiptextlabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);

    QObject::connect(ipInput, &QLineEdit::textChanged, this, &ReadyWidget::onLineTextChange);
    QObject::connect(captchaInput, &QLineEdit::textChanged, this, &ReadyWidget::onLineTextChange);
    QObject::connect(TransferHelper::instance(), &TransferHelper::connectSucceed, this,
                     &ReadyWidget::nextPage);
}

void ReadyWidget::tryConnect()
{
    qInfo() << ipInput->text() << " " << captchaInput->text();
    TransferHelper::instance()->tryConnect(ipInput->text(), captchaInput->text());
    tiptextlabel->setVisible(true);
}

void ReadyWidget::nextPage()
{
    tiptextlabel->setVisible(false);
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void ReadyWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void ReadyWidget::onLineTextChange()
{
    QRegularExpression ipRegex(
            "^((\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])\\.){3}(\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])$");
    QRegularExpressionMatch ipMatch = ipRegex.match(ipInput->text());

    if (!ipMatch.hasMatch()) {
        nextButton->setEnabled(false);
        return;
    }

    QRegularExpression captchaRegex("^\\d{6}$");
    QRegularExpressionMatch captchaMatch = captchaRegex.match(captchaInput->text());

    if (!captchaMatch.hasMatch()) {
        nextButton->setEnabled(false);
        return;
    }

    nextButton->setEnabled(true);
}
