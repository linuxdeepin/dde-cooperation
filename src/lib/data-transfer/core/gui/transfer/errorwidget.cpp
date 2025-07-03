#include "errorwidget.h"
#include "transferringwidget.h"
#include "../type_defines.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <net/helper/transferhepler.h>
#include "common/commonutils.h"

ErrorWidget::ErrorWidget(QWidget *parent)
    : QFrame(parent)
{
    DLOG << "Initializing error widget";

    initUI();
}

ErrorWidget::~ErrorWidget()
{
    DLOG << "Destroying error widget";
}

void ErrorWidget::initUI()
{
    DLOG << "Initializing UI";

    setStyleSheet(".ErrorWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/transfer.png").pixmap(200, 160));
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *errorLabel = new QLabel(this);
    errorLabel->setStyleSheet(".QLabel{"
                              "background-color: transparent;"
                              "}");

    errorLabel->setPixmap(QIcon(":/icon/warning.svg").pixmap(48, 48));
    errorLabel->setGeometry(420, 200, 48, 48);

    QString titleStr = internetError;
    titleLabel = new QLabel(titleStr, this);
    titleLabel->setFixedHeight(50);
    titleLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ProgressBarLabel *progressLabel = new ProgressBarLabel(this);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setProgress(50);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->addWidget(progressLabel, Qt::AlignCenter);

    QLabel *timeLabel = new QLabel(this);

    timeLabel->setText(QString("%1 - -").arg(tr("Transfer will be completed in")));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    promptLabel = new QLabel(this);
    promptLabel->setStyleSheet(StyleHelper::textStyle(StyleHelper::error));
    promptLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    StyleHelper::setAutoFont(titleLabel, 17, QFont::DemiBold);
    StyleHelper::setAutoFont(timeLabel, 12, QFont::Normal);

    ButtonLayout *buttonLayout = new ButtonLayout();
    QPushButton *backButton = buttonLayout->getButton1();
    backButton->setText(tr("Back"));
    backButton->setFixedSize(120, 36);
    QPushButton *retryButton = buttonLayout->getButton2();
    retryButton->setText(tr("Try again"));

    connect(backButton, &QPushButton::clicked, this, &ErrorWidget::backPage);
    connect(retryButton, &QPushButton::clicked, this, &ErrorWidget::retryPage);

    IndexLabel *indelabel = new IndexLabel(3, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(progressLayout);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(timeLabel);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(promptLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);

    // 默认设置为无错误状态
    currentErrorType = noError;
    DLOG << "Error widget initialized";
}

bool ErrorWidget::checkNetworkAndUpdate()
{
    DLOG << "Checking network and updating state";
    // 检查当前网络状态
    bool isNetworkAvailable = deepin_cross::CommonUitls::getFirstIp().size() > 0;

    // 如果是网络错误类型，且现在网络已恢复，通知网络状态变更
    if (currentErrorType == ErrorType::networkError && isNetworkAvailable) {
        DLOG << "Network is now available, updating online state";
        // 只通知网络状态变更，不触发其他操作
        emit TransferHelper::instance()->onlineStateChanged(true);
        return true;
    }

    DLOG << "Network is not available, updating offline state";
    return isNetworkAvailable;
}

void ErrorWidget::backPage()
{
    DLOG << "Back button clicked, returning to choose page";

    // 检查网络状态并更新
    checkNetworkAndUpdate();

    emit TransferHelper::instance()->clearWidget();
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void ErrorWidget::retryPage()
{
    DLOG << "Retry button clicked, returning to choose page";

    // 检查网络状态并更新
    checkNetworkAndUpdate();

    emit TransferHelper::instance()->clearWidget();
    emit TransferHelper::instance()->changeWidget(PageName::choosewidget);
}

void ErrorWidget::themeChanged(int theme)
{
    DLOG << "Theme changed to:" << (theme == 1 ? "Light" : "Dark");

    // light
    if (theme == 1) {
        DLOG << "Theme is light, setting stylesheet";
        setStyleSheet(".ErrorWidget{background-color: white; border-radius: 10px;}");
    } else {
        // dark
        DLOG << "Theme is dark, setting stylesheet";
        setStyleSheet(".ErrorWidget{background-color: rgb(37, 37, 37); border-radius: 10px;}");
    }
}

void ErrorWidget::setErrorType(ErrorType type, int size)
{
    DLOG << "Setting error type to:" << (int)type;
    currentErrorType = type;

    if (type == ErrorType::networkError) {
        DLOG << "Setting error type to networkError";
        titleLabel->setText(internetError);
        promptLabel->setText(internetErrorPrompt);
    } else if (type == ErrorType::outOfStorageError) {
        DLOG << "Setting error type to outOfStorageError";
        titleLabel->setText(transferError);
        QString prompt;
        if (size == 0) {
            DLOG << "Size is 0, setting prompt for Windows";
            prompt = QString(transferErrorPromptWin);
        } else {
            DLOG << "Size is not 0, setting prompt for UOS with size";
            prompt = QString(transferErrorPromptUOS).arg(size);
        }
        promptLabel->setText(prompt);
    } else {
        DLOG << "Unknown error type:" << type;
    }
}
