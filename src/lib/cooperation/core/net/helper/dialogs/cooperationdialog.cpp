// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationdialog.h"

#include <QHBoxLayout>
#include <QCloseEvent>
#include <QMovie>

using namespace cooperation_core;

ConfirmWidget::ConfirmWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void ConfirmWidget::setDeviceName(const QString &name)
{
    static QString msg(tr("\"%1\" send some files to you"));
    msgLabel->setText(msg.arg(name));
}

void ConfirmWidget::init()
{
    msgLabel = new CooperationLabel(this);
    rejectBtn = new QPushButton(tr("Reject", "button"), this);
    acceptBtn = new QPushButton(tr("Accept", "button"), this);

    connect(rejectBtn, &QPushButton::clicked, this, &ConfirmWidget::rejected);
    connect(acceptBtn, &QPushButton::clicked, this, &ConfirmWidget::accepted);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(rejectBtn);
    btnLayout->addWidget(acceptBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(msgLabel, 1, Qt::AlignHCenter);
    mainLayout->addLayout(btnLayout, 1);
}

WaitConfirmWidget::WaitConfirmWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void WaitConfirmWidget::startMovie()
{
#ifdef linux
    spinner->start();
#else
    spinner->movie()->start();
#endif
}

void WaitConfirmWidget::init()
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);

    spinner = new CooperationSpinner(this);
    spinner->setFixedSize(48, 48);
    spinner->setAttribute(Qt::WA_TransparentForMouseEvents);
    spinner->setFocusPolicy(Qt::NoFocus);
#ifndef linux
    QLabel *titleLabel = new QLabel(tr("File Transfer"), this);
    QFont font;
    font.setWeight(QFont::DemiBold);
    font.setPixelSize(18);
    titleLabel->setFont(font);
    titleLabel->setAlignment(Qt::AlignHCenter);

    vLayout->addSpacing(30);
    vLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    vLayout->addSpacing(50);

    QMovie *movie = new QMovie(":/icons/deepin/builtin/icons/spinner.gif");
    spinner->setMovie(movie);
    movie->setSpeed(180);
#endif

    CooperationLabel *label = new CooperationLabel(tr("Wait for confirmation..."), this);
    label->setAlignment(Qt::AlignHCenter);
    vLayout->addSpacing(20);
    vLayout->addWidget(spinner, 0, Qt::AlignHCenter);
    vLayout->addSpacing(15);
    vLayout->addWidget(label, 0, Qt::AlignHCenter);
    vLayout->addSpacerItem(new QSpacerItem(1, 80, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

ProgressWidget::ProgressWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void ProgressWidget::setTitle(const QString &title)
{
    titleLabel->setText(title);
}

void ProgressWidget::setProgress(int value, const QString &msg)
{
    progressBar->setValue(value);
    QString remainTimeMsg(tr("Remaining time %1 | %2%").arg(msg, QString::number(value)));
    msgLabel->setText(remainTimeMsg);
}

void ProgressWidget::init()
{
    titleLabel = new CooperationLabel(this);
    msgLabel = new CooperationLabel(this);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(8);

    cancelBtn = new QPushButton(tr("Cancel", "button"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &ProgressWidget::canceled);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel, 1, Qt::AlignHCenter);
    mainLayout->addWidget(progressBar, 1);
    mainLayout->addWidget(msgLabel, 1, Qt::AlignHCenter);
    mainLayout->addWidget(cancelBtn, 1);
}

ResultWidget::ResultWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void ResultWidget::setResult(bool success, const QString &msg)
{
    res = success;
    if (success) {
        QIcon icon(":/icons/deepin/builtin/icons/transfer_success_128px.svg");
        iconLabel->setPixmap(icon.pixmap(48, 48));
#ifndef __linux__
        viewBtn->setVisible(true);
#endif
    } else {
        QIcon icon(":/icons/deepin/builtin/icons/transfer_fail_128px.svg");
        iconLabel->setPixmap(icon.pixmap(48, 48));
        viewBtn->setVisible(false);
    }

    msgLabel->setText(msg);
}

void ResultWidget::init()
{
    iconLabel = new CooperationLabel(this);
    msgLabel = new CooperationLabel(this);
    msgLabel->setWordWrap(true);
    msgLabel->setAlignment(Qt::AlignHCenter);

    okBtn = new QPushButton(tr("Ok", "button"), this);
    connect(okBtn, &QPushButton::clicked, this, &ResultWidget::completed);

    viewBtn = new QPushButton(tr("View", "button"), this);
    viewBtn->setVisible(false);
    connect(viewBtn, &QPushButton::clicked, this, &ResultWidget::viewed);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(viewBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(iconLabel, 1, Qt::AlignHCenter);
    mainLayout->addWidget(msgLabel, 1);
    mainLayout->addLayout(btnLayout, 1);
}

bool ResultWidget::getResult() const
{
    return res;
}

CooperationTransDialog::CooperationTransDialog(QWidget *parent)
    : CooperationDialog(parent)
{
    init();
}

void CooperationTransDialog::showConfirmDialog(const QString &name)
{
    confirmWidget->setDeviceName(name);
    mainLayout->setCurrentWidget(confirmWidget);
    setHidden(false);
}

void CooperationTransDialog::showWaitConfirmDialog()
{
    waitconfirmWidget->startMovie();
    mainLayout->setCurrentWidget(waitconfirmWidget);
    setHidden(false);
}

void CooperationTransDialog::showResultDialog(bool success, const QString &msg)
{
    resultWidget->setResult(success, msg);
    mainLayout->setCurrentWidget(resultWidget);
    setHidden(false);
}

void CooperationTransDialog::showProgressDialog(const QString &title)
{
    if (mainLayout->currentWidget() == progressWidget)
        return;

    progressWidget->setTitle(title);
    mainLayout->setCurrentWidget(progressWidget);
    setHidden(false);
}

void CooperationTransDialog::updateProgress(int value, const QString &msg)
{
    progressWidget->setProgress(value, msg);
}

void CooperationTransDialog::closeEvent(QCloseEvent *e)
{
    if (!isVisible())
        e->accept();

    if (mainLayout->currentWidget() == confirmWidget) {
        Q_EMIT rejected();
    } else if (mainLayout->currentWidget() == progressWidget) {
        Q_EMIT cancel();
    } else if (mainLayout->currentWidget() == resultWidget) {
        if (qApp->property("onlyTransfer").toBool() && resultWidget->getResult())
            qApp->exit();
        Q_EMIT cancel();
    }
}

void CooperationTransDialog::init()
{
    QWidget *contentWidget = new QWidget(this);
    mainLayout = new QStackedLayout(this);

    QVBoxLayout *vLayout = new QVBoxLayout(contentWidget);
    vLayout->setMargin(0);
    vLayout->addLayout(mainLayout);

#ifdef linux
    setIcon(QIcon::fromTheme("dde-cooperation"));
    setTitle(tr("File Transfer"));
    addContent(contentWidget);
#else
    setWindowTitle(tr("File transfer"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/icons/deepin/builtin/icons/dde-cooperation_128px.svg"));
    setFixedWidth(380);
#endif

    confirmWidget = new ConfirmWidget(this);
    connect(confirmWidget, &ConfirmWidget::rejected, this, &CooperationTransDialog::rejected);
    connect(confirmWidget, &ConfirmWidget::accepted, this, &CooperationTransDialog::accepted);

    progressWidget = new ProgressWidget(this);
    connect(progressWidget, &ProgressWidget::canceled, this, &CooperationTransDialog::cancel);

    waitconfirmWidget = new WaitConfirmWidget(this);
    connect(waitconfirmWidget, &WaitConfirmWidget::canceled, this, &CooperationTransDialog::cancel);

    resultWidget = new ResultWidget(this);
    connect(resultWidget, &ResultWidget::viewed, this, &CooperationTransDialog::viewed);
    connect(resultWidget, &ResultWidget::completed, this, &CooperationTransDialog::close);

    mainLayout->addWidget(confirmWidget);
    mainLayout->addWidget(waitconfirmWidget);
    mainLayout->addWidget(progressWidget);
    mainLayout->addWidget(resultWidget);
}
