// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationdialog.h"

#include <QHBoxLayout>
#include <QCloseEvent>

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
    msgLabel = new QLabel(this);
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
    titleLabel = new QLabel(this);
    msgLabel = new QLabel(this);

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
    if (success) {
        QIcon icon(":/icons/deepin/builtin/icons/transfer_success_128px.svg");
        iconLabel->setPixmap(icon.pixmap(48, 48));
        viewBtn->setVisible(true);
    } else {
        QIcon icon(":/icons/deepin/builtin/icons/transfer_fail_128px.svg");
        iconLabel->setPixmap(icon.pixmap(48, 48));
        viewBtn->setVisible(false);
    }

    msgLabel->setText(msg);
}

void ResultWidget::init()
{
    iconLabel = new QLabel(this);
    msgLabel = new QLabel(this);
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

CooperationTransDialog::CooperationTransDialog(QWidget *parent)
    : QDialog(parent)
{
    init();
}

void CooperationTransDialog::showConfirmDialog(const QString &name)
{
    confirmWidget->setDeviceName(name);
    mainLayout->setCurrentWidget(confirmWidget);
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

void CooperationTransDialog::updateProgressData(int value, const QString &msg)
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
        Q_EMIT canceled();
    }
}

void CooperationTransDialog::init()
{
    setWindowTitle(tr("File transfer"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/icons/deepin/builtin/icons/dde-cooperation_128px.svg"));
    setFixedWidth(380);

    mainLayout = new QStackedLayout(this);
    confirmWidget = new ConfirmWidget(this);
    connect(confirmWidget, &ConfirmWidget::rejected, this, &CooperationTransDialog::rejected);
    connect(confirmWidget, &ConfirmWidget::accepted, this, &CooperationTransDialog::accepted);

    progressWidget = new ProgressWidget(this);
    connect(progressWidget, &ProgressWidget::canceled, this, &CooperationTransDialog::canceled);

    resultWidget = new ResultWidget(this);
    connect(resultWidget, &ResultWidget::completed, this, &CooperationTransDialog::completed);
    connect(resultWidget, &ResultWidget::viewed, this, &CooperationTransDialog::viewed);

    mainLayout->addWidget(confirmWidget);
    mainLayout->addWidget(progressWidget);
    mainLayout->addWidget(resultWidget);
}
