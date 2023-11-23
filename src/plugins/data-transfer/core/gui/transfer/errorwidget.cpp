#include "errorwidget.h"
#include "transferringwidget.h"
#include "../type_defines.h"

#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <gui/connect/choosewidget.h>

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
    QPixmap icon(":/icon/transfer.png");
    QPixmap transparentPixmap(icon.size());
    iconLabel->setPixmap(icon);
    iconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    QLabel *errorLabel = new QLabel(this);
    errorLabel->setStyleSheet(".QLabel{"
                              "background-color: transparent;"
                              "}");
    QPixmap errorPixmap(":/icon/warning.svg");

    errorPixmap.scaled(32, 32, Qt::KeepAspectRatio);
    errorLabel->setPixmap(errorPixmap);
    errorLabel->setGeometry(420, 180, errorPixmap.width(), errorPixmap.height());

    QString titleStr = internetError;
    titleLabel = new QLabel(titleStr, this);
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
    timeLabel->setText(QString("%1 - -").arg(tr("Transfer will be completed in")));
    timeLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    QFont timefont;
    font.setPointSize(5);
    timeLabel->setFont(timefont);

    promptLabel = new QLabel(this);
    promptLabel->setText(QString("<font size='2' color='#FF5736'>%1</font>").arg(tr("Try again")));
    promptLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(backButton, &QToolButton::clicked, this, &ErrorWidget::backPage);

    QToolButton *retryButton = new QToolButton(this);

    retryButton->setText(tr("Try again"));
    retryButton->setStyleSheet("background-color: #0098FF;");
    QPalette palette = retryButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    retryButton->setPalette(palette);
    retryButton->setFixedSize(120, 35);

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

    setErrorType(ErrorType::outOfStorageError);
}

void ErrorWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void ErrorWidget::retryPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
void ErrorWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        // dark
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
    }
}

void ErrorWidget::setErrorType(ErrorType type, int size)
{
    if (type == ErrorType::networkError) {
        titleLabel->setText(internetError);
        promptLabel->setText(
                QString("<font size='2' color='#FF5736'>%1</font>").arg(internetErrorPrompt));
    } else {
        titleLabel->setText(transferError);
        QString prompt;
        if (size == 0)
            prompt = QString(transferErrorPromptWin);
        else
            prompt = QString(transferErrorPromptUOS).arg(size);
        promptLabel->setText(QString("<font size='2' color='#FF5736'>%1</font>").arg(prompt));
    }
}
