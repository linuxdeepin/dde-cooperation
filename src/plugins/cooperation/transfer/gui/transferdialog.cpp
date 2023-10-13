// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferdialog.h"

using namespace cooperation_transfer;

TransferDialog::TransferDialog(QWidget *parent)
    : CooperationDialog(parent)
{
    initUI();
}

void TransferDialog::initUI()
{
    setFixedSize(380, 223);
    setIcon(QIcon::fromTheme("collaboration"));

    QWidget *contentWidget = new QWidget(this);
    stackedLayout = new QStackedLayout;
    okBtn = new QPushButton(this);

    QVBoxLayout *vLayout = new QVBoxLayout(contentWidget);
    vLayout->addLayout(stackedLayout);
    vLayout->addWidget(okBtn);

    addContent(contentWidget);

    createWaitConfirmPage();
    createResultPage();
    createProgressPage();
}

void TransferDialog::createWaitConfirmPage()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    stackedLayout->addWidget(widget);

    spinner = new CooperationSpinner(this);
    spinner->setFixedSize(36, 36);
    spinner->setAttribute(Qt::WA_TransparentForMouseEvents);
    spinner->setFocusPolicy(Qt::NoFocus);

    QLabel *label = new QLabel(tr("Wait for confirmation"), this);
    label->setAlignment(Qt::AlignHCenter);

    vLayout->addWidget(spinner, 0, Qt::AlignHCenter);
    vLayout->addWidget(label, 0, Qt::AlignHCenter);
}

void TransferDialog::createResultPage()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    stackedLayout->addWidget(widget);

    iconLabel = new QLabel(this);
    msgLabel = new QLabel(this);
    msgLabel->setAlignment(Qt::AlignHCenter);

    vLayout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    vLayout->addWidget(msgLabel, 0, Qt::AlignHCenter);
}

void TransferDialog::createProgressPage()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    stackedLayout->addWidget(widget);

    titleLabel = new QLabel(this);
    titleLabel->setAlignment(Qt::AlignHCenter);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(1);
    progressBar->setTextVisible(false);
    progressBar->setFixedSize(339, 8);

    progressMsgLael = new QLabel(this);
    progressMsgLael->setAlignment(Qt::AlignHCenter);

    vLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);
    vLayout->addWidget(progressBar, 0, Qt::AlignHCenter);
    vLayout->addWidget(progressMsgLael, 0, Qt::AlignHCenter);
}

void TransferDialog::switchWaitConfirmPage()
{
    stackedLayout->setCurrentIndex(0);
    spinner->start();
    okBtn->setText(tr("Cancel"));
}

void TransferDialog::switchResultPage(bool success, const QString &msg)
{
    spinner->stop();
    stackedLayout->setCurrentIndex(1);

    if (success) {
        auto icon = QIcon::fromTheme("transfer_success");
        iconLabel->setPixmap(icon.pixmap(36, 36));
    } else {
        auto icon = QIcon::fromTheme("transfer_fail");
        iconLabel->setPixmap(icon.pixmap(36, 36));
    }
    msgLabel->setText(msg);
    okBtn->setText(tr("Ok"));
}

void TransferDialog::switchProgressPage(const QString &title)
{
    spinner->stop();
    stackedLayout->setCurrentIndex(2);

    titleLabel->setText(title);
    okBtn->setText(tr("Cancel"));
}

void TransferDialog::updateProgress(int value, const QString &msg)
{
    progressBar->setValue(value);
    progressMsgLael->setText(msg);
}
