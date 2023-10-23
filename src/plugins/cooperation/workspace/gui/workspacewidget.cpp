// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workspacewidget.h"
#include "workspacewidget_p.h"
#include "widgets/cooperationstatewidget.h"
#include "widgets/devicelistwidget.h"

using namespace cooperation_workspace;

WorkspaceWidgetPrivate::WorkspaceWidgetPrivate(WorkspaceWidget *qq)
    : q(qq),
      sortFilterWorker(new SortFilterWorker),
      workThread(new QThread)
{
    sortFilterWorker->moveToThread(workThread.data());
    workThread->start();
}

WorkspaceWidgetPrivate::~WorkspaceWidgetPrivate()
{
    sortFilterWorker->stop();
    workThread->quit();
    workThread->wait();
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

void WorkspaceWidgetPrivate::initConnect()
{
    connect(searchEdit, &CooperationSearchEdit::textChanged, this, &WorkspaceWidgetPrivate::onSearchValueChanged);
    connect(this, &WorkspaceWidgetPrivate::deviceAdded, sortFilterWorker.data(), &SortFilterWorker::sortDevice, Qt::QueuedConnection);
    connect(this, &WorkspaceWidgetPrivate::filterDevice, sortFilterWorker.data(), &SortFilterWorker::filterDevice, Qt::QueuedConnection);
    connect(this, &WorkspaceWidgetPrivate::clearDevice, sortFilterWorker.data(), &SortFilterWorker::clear, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::sortFilterResult, this, &WorkspaceWidgetPrivate::onSortFilterResult, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::filterFinished, this, &WorkspaceWidgetPrivate::onFilterFinished, Qt::QueuedConnection);
}

void WorkspaceWidgetPrivate::onSearchValueChanged(const QString &text)
{
    stackedLayout->setCurrentWidget(dlWidget);
    dlWidget->clear();
    Q_EMIT filterDevice(text);
}

void WorkspaceWidgetPrivate::onSortFilterResult(int index, const DeviceInfo &info)
{
    dlWidget->insertItem(index, info);
}

void WorkspaceWidgetPrivate::onFilterFinished()
{
    if (dlWidget->itemCount() == 0) {
        if (searchEdit->text().isEmpty()) {
            stackedLayout->setCurrentIndex(currentPage);
            return;
        }

        stackedLayout->setCurrentWidget(nrWidget);
    }
}

WorkspaceWidget::WorkspaceWidget(QWidget *parent)
    : QWidget(parent),
      d(new WorkspaceWidgetPrivate(this))
{
    d->initUI();
    d->initConnect();
}

void WorkspaceWidget::switchWidget(PageName page)
{
    d->currentPage = page;
    d->stackedLayout->setCurrentIndex(page);
}

void WorkspaceWidget::addDeviceInfo(const DeviceInfo &info)
{
    Q_EMIT d->deviceAdded(info);
}

void WorkspaceWidget::addDeviceOperation(const QVariantMap &map)
{
    d->dlWidget->addItemOperation(map);
}

void WorkspaceWidget::clear()
{
    d->dlWidget->clear();
    Q_EMIT d->clearDevice();
}
