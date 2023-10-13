// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workspacewidget.h"
#include "workspacewidget_p.h"
#include "widgets/cooperationstatewidget.h"
#include "widgets/devicelistwidget.h"

#include <QPainter>

using namespace cooperation_workspace;

WorkspaceWidgetPrivate::WorkspaceWidgetPrivate(WorkspaceWidget *qq)
    : q(qq)
{
}

void WorkspaceWidgetPrivate::initUI()
{
    searchEdit = new CooperationSearchEdit(q);
    stackedLayout = new QStackedLayout;

    lfdWidget = new LookingForDeviceWidget(q);
    nnWidget = new NoNetworkWidget(q);
    nrWidget = new NoResultWidget(q);
    dlWidget = new DeviceListWidget(q);

    stackedLayout->addWidget(lfdWidget);
    stackedLayout->addWidget(nnWidget);
    stackedLayout->addWidget(nrWidget);
    stackedLayout->addWidget(dlWidget);
    stackedLayout->setCurrentIndex(0);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->addWidget(searchEdit);
    mainLayout->addLayout(stackedLayout);
    q->setLayout(mainLayout);
}

WorkspaceWidget::WorkspaceWidget(QWidget *parent)
    : QWidget(parent),
      d(new WorkspaceWidgetPrivate(this))
{
    d->initUI();
}

void WorkspaceWidget::switchWidget(PageName page)
{
    d->stackedLayout->setCurrentIndex(page);
}

void WorkspaceWidget::addDeviceInfo(const DeviceInfo &info)
{
    d->dlWidget->appendItem(info);
}

void WorkspaceWidget::addDeviceOperation(const QVariantMap &map)
{
    d->dlWidget->addItemOperation(map);
}

void WorkspaceWidget::clear()
{
    d->dlWidget->clear();
}
