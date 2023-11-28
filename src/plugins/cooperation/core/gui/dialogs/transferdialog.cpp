// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferdialog.h"

using namespace cooperation_core;

TransferDialog::TransferDialog(QWidget *parent)
    : CooperationDialog(parent)
{
    initUI();
}

void TransferDialog::initUI()
{
    setFixedSize(380, 234);
    setContentsMargins(0, 0, 0, 0);

    QWidget *contentWidget = new QWidget(this);
    stackedLayout = new QStackedLayout;
    okBtn = new QPushButton(this);
    connect(okBtn, &QPushButton::clicked, this, &TransferDialog::close);

    QVBoxLayout *vLayout = new QVBoxLayout(contentWidget);
    vLayout->setMargin(0);
    vLayout->addLayout(stackedLayout);
    vLayout->addWidget(okBtn, 0, Qt::AlignBottom);

#ifdef linux
    setIcon(QIcon::fromTheme("dde-cooperation"));
    setTitle(tr("File Transfer"));
    addContent(contentWidget);
#else
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(contentWidget);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#endif
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
    spinner->setFixedSize(48, 48);
    spinner->setAttribute(Qt::WA_TransparentForMouseEvents);
    spinner->setFocusPolicy(Qt::NoFocus);

    QLabel *label = new QLabel(tr("Wait for confirmation..."), this);
    label->setAlignment(Qt::AlignHCenter);

    vLayout->addWidget(spinner, 0, Qt::AlignHCenter);
    vLayout->addSpacing(15);
    vLayout->addWidget(label, 0, Qt::AlignHCenter);
    vLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void TransferDialog::createResultPage()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *vLayout = new QVBoxLayout(widget);
    stackedLayout->addWidget(widget);

    iconLabel = new QLabel(this);
    msgLabel = new QLabel(this);
    msgLabel->setAlignment(Qt::AlignHCenter);
    msgLabel->setWordWrap(true);

    vLayout->addWidget(iconLabel, 0, Qt::AlignHCenter);
    vLayout->addWidget(msgLabel);
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
#ifdef linux
    spinner->start();
#endif
    okBtn->setVisible(false);
}

void TransferDialog::switchResultPage(bool success, const QString &msg)
{
#ifdef linux
    spinner->stop();
#endif
    stackedLayout->setCurrentIndex(1);

    if (success) {
        auto icon = QIcon::fromTheme("transfer_success");
        iconLabel->setPixmap(icon.pixmap(48, 48));
    } else {
        auto icon = QIcon::fromTheme("transfer_fail");
        iconLabel->setPixmap(icon.pixmap(48, 48));
    }
    msgLabel->setText(msg);
    okBtn->setText(tr("Ok", "button"));
    okBtn->setVisible(true);
}

void TransferDialog::switchProgressPage(const QString &title)
{
    if (stackedLayout->currentIndex() == 2)
        return;

#ifdef linux
    spinner->stop();
#endif
    stackedLayout->setCurrentIndex(2);

    progressBar->setValue(1);
    titleLabel->setText(title);
    okBtn->setText(tr("Cancel", "button"));
    okBtn->setVisible(true);
}

void TransferDialog::updateProgress(int value, const QString &remainTime)
{
    if (progressBar->value() < value)
        progressBar->setValue(value);

    QString remainTimeMsg(tr("Remaining time %1 | %2%").arg(remainTime, QString::number(value)));
    progressMsgLael->setText(remainTimeMsg);
}

void TransferDialog::closeEvent(QCloseEvent *event)
{
    Q_EMIT cancel();
    CooperationAbstractDialog::closeEvent(event);
}
