// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationtaskdialog.h"

#include <QLabel>
#include <QVBoxLayout>

CooperationTaskDialog::CooperationTaskDialog(QWidget *parent)
    : CooperationDialog(parent)
{
    init();
}

void CooperationTaskDialog::switchWaitPage(const QString &dev)
{
    static QString title(tr("Requesting collaborate to \"%1\""));
    setTaskTitle(title.arg(dev));
    mainLayout->setCurrentIndex(0);
}

void CooperationTaskDialog::switchFailPage(const QString &dev, const QString &msg, bool retry)
{
    static QString title(tr("Unable to collaborate to \"%1\""));
    setTaskTitle(title.arg(dev));

    msgLabel->setText(msg);
    retryBtn->setVisible(retry);
    mainLayout->setCurrentIndex(1);
}

void CooperationTaskDialog::init()
{
#ifdef linux
    setIcon(QIcon::fromTheme("dde-cooperation"));
#else
    setWindowIcon(QIcon(":/icons/deepin/builtin/icons/dde-cooperation_128px.svg"));
#endif
    setFixedWidth(380);

    QWidget *contentWidget = new QWidget(this);
    mainLayout = new QStackedLayout(contentWidget);

    mainLayout->addWidget(createWaitPage());
    mainLayout->addWidget(createFailPage());

    addContent(contentWidget);
}

void CooperationTaskDialog::setTaskTitle(const QString &title)
{
#ifdef linux
    setTitle(title);
#else
    setWindowTitle(title);
#endif
}

QWidget *CooperationTaskDialog::createWaitPage()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *vlayout = new QVBoxLayout(widget);
    vlayout->setContentsMargins(0, 0, 0, 0);

    CooperationSpinner *spinner = new CooperationSpinner(this);
    spinner->setFixedSize(48, 48);
    spinner->setAttribute(Qt::WA_TransparentForMouseEvents);
    spinner->setFocusPolicy(Qt::NoFocus);
    spinner->start();

    QPushButton *celBtn = new QPushButton(tr("Cancel", "button"), this);
    connect(celBtn, &QPushButton::clicked, this, &CooperationTaskDialog::waitCanceled);

    vlayout->addWidget(spinner, 0, Qt::AlignHCenter);
    vlayout->addWidget(celBtn);

    return widget;
}

QWidget *CooperationTaskDialog::createFailPage()
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *vlayout = new QVBoxLayout(widget);
    vlayout->setContentsMargins(0, 0, 0, 0);

    msgLabel = new QLabel(this);
    msgLabel->setAlignment(Qt::AlignHCenter);
    msgLabel->setWordWrap(true);

    cancelBtn = new QPushButton(tr("Cancel", "button"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &CooperationTaskDialog::close);

    retryBtn = new QPushButton(tr("Retry", "button"), this);
    connect(cancelBtn, &QPushButton::clicked, this, &CooperationTaskDialog::retryConnected);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(cancelBtn);
    hlayout->addWidget(retryBtn);

    vlayout->addWidget(msgLabel);
    vlayout->addLayout(hlayout);

    return widget;
}
