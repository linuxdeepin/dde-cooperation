#include "errorwidget.h"
#include "transferringwidget.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

inline constexpr char internetError[]{ "网络异常" };
inline constexpr char transferError[]{ "传输中断" };
inline constexpr char internetErrorPrompt[]{ "网络断开,传输失败,请连接网络后重试" };
inline constexpr char transferErrorPrompt[]{ "UOS空间不足,请清除至少10G后重试" };
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
    iconLabel->setPixmap(QPixmap(":/icon/transfer.png"));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *errorLabel = new QLabel(this);
    errorLabel->setStyleSheet(".QLabel{"
                              "background-color: transparent;"
                              "}");
    QPixmap errorPixmap(":/icon/warning.svg");
    errorPixmap.scaled(32, 32, Qt::KeepAspectRatio);
    errorLabel->setPixmap(errorPixmap);
    errorLabel->setGeometry(420, 180, errorPixmap.width(), errorPixmap.height());

    QString titleStr;
    if (state == 1) {
        titleStr = internetError;
    } else if (state == 2) {
        titleStr = transferError;
    }
    QLabel *titleLabel = new QLabel(titleStr, this);
    titleLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titleLabel->setFont(font);
    titleLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ProgressBarLabel *progressLabel = new ProgressBarLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setProgress(50);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    QLabel *timeLabel = new QLabel(this);
    timeLabel->setText(QString("预计迁移时间还剩 - -"));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    font.setPointSize(7);
    timeLabel->setFont(timefont);

    QString promptStr;
    if (state == 1) {
        promptStr = internetErrorPrompt;
    } else if (state == 2) {
        promptStr = transferErrorPrompt;
    }
    QLabel *promptLabel = new QLabel(this);
    promptLabel->setText(promptStr);
    promptLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

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
                              "letter-spacing: 3px;"
                              "text-align: center;"
                              ";}");
    QObject::connect(backButton, &QToolButton::clicked, this, &ErrorWidget::backPage);

    QToolButton *retryButton = new QToolButton(this);

    retryButton->setText("重试");
    retryButton->setFixedSize(120, 35);
    retryButton->setStyleSheet(".QToolButton{"
                               "border-radius: 8px;"
                               "border: 1px solid rgba(0,0,0, 0.03);"
                               "opacity: 1;"
                               "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                               "rgba(37, 183, 255, 1), stop:1 rgba(0, 152, 255, 1));"
                               "font-family: \"SourceHanSansSC-Medium\";"
                               "font-size: 14px;"
                               "font-weight: 500;"
                               "color: rgba(255,255,255,1);"
                               "font-style: normal;"
                               "letter-spacing: 3px;"
                               "text-align: center;"
                               "}");

    QObject::connect(retryButton, &QToolButton::clicked, this, &ErrorWidget::retryPage);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(retryButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(timeLabel);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(promptLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
}

void ErrorWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void ErrorWidget::retryPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
