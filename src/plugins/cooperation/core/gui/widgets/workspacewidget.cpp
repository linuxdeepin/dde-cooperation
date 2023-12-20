// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workspacewidget.h"
#include "workspacewidget_p.h"
#include "cooperationstatewidget.h"
#include "devicelistwidget.h"

#include <QMouseEvent>

using namespace cooperation_core;

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
    searchEdit->setContentsMargins(20, 0, 20, 0);

    searchEdit->setPlaceholderText(tr("Please enter the device name or IP"));
    stackedLayout = new QStackedLayout;

    lfdWidget = new LookingForDeviceWidget(q);
    nnWidget = new NoNetworkWidget(q);
    nrWidget = new NoResultWidget(q);
    nrWidget->setContentsMargins(20, 0, 20, 0);
    dlWidget = new DeviceListWidget(q);
    dlWidget->setContentsMargins(20, 0, 20, 0);

    stackedLayout->addWidget(lfdWidget);
    stackedLayout->addWidget(nnWidget);
    stackedLayout->addWidget(nrWidget);
    stackedLayout->addWidget(dlWidget);
    stackedLayout->setCurrentIndex(0);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 15, 0, 15);
#ifndef linux
    mainLayout->addSpacing(50);
    mainLayout->addWidget(searchEdit, 0, Qt::AlignHCenter);
#else
    mainLayout->addWidget(searchEdit);
#endif
    mainLayout->addSpacing(16);
    mainLayout->addLayout(stackedLayout);
    q->setLayout(mainLayout);
}

void WorkspaceWidgetPrivate::initConnect()
{
    connect(searchEdit, &CooperationSearchEdit::textChanged, this, &WorkspaceWidgetPrivate::onSearchValueChanged);
    connect(this, &WorkspaceWidgetPrivate::devicesAdded, sortFilterWorker.data(), &SortFilterWorker::addDevice, Qt::QueuedConnection);
    connect(this, &WorkspaceWidgetPrivate::devicesRemoved, sortFilterWorker.data(), &SortFilterWorker::removeDevice, Qt::QueuedConnection);
    connect(this, &WorkspaceWidgetPrivate::filterDevice, sortFilterWorker.data(), &SortFilterWorker::filterDevice, Qt::QueuedConnection);
    connect(this, &WorkspaceWidgetPrivate::clearDevice, sortFilterWorker.data(), &SortFilterWorker::clear, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::sortFilterResult, this, &WorkspaceWidgetPrivate::onSortFilterResult, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::filterFinished, this, &WorkspaceWidgetPrivate::onFilterFinished, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::deviceRemoved, this, &WorkspaceWidgetPrivate::onDeviceRemoved, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::deviceUpdated, this, &WorkspaceWidgetPrivate::onDeviceUpdated, Qt::QueuedConnection);
    connect(sortFilterWorker.data(), &SortFilterWorker::deviceMoved, this, &WorkspaceWidgetPrivate::onDeviceMoved, Qt::QueuedConnection);
}

void WorkspaceWidgetPrivate::onSearchValueChanged(const QString &text)
{
    if (currentPage == WorkspaceWidget::kNoNetworkWidget)
        return;

    dlWidget->clear();
    Q_EMIT filterDevice(text);
}

void WorkspaceWidgetPrivate::onSortFilterResult(int index, const DeviceInfoPointer info)
{
    q->switchWidget(WorkspaceWidget::kDeviceListWidget);
    dlWidget->insertItem(index, info);
}

void WorkspaceWidgetPrivate::onFilterFinished()
{
    if (dlWidget->itemCount() == 0) {
        if (searchEdit->text().isEmpty()) {
            stackedLayout->setCurrentIndex(currentPage);
            return;
        }

        q->switchWidget(WorkspaceWidget::kNoResultWidget);
    }
}

void WorkspaceWidgetPrivate::onDeviceRemoved(int index)
{
    dlWidget->removeItem(index);
    if (dlWidget->itemCount() == 0)
        q->switchWidget(WorkspaceWidget::kNoResultWidget);
}

void WorkspaceWidgetPrivate::onDeviceUpdated(int index, const DeviceInfoPointer info)
{
    dlWidget->updateItem(index, info);
}

void WorkspaceWidgetPrivate::onDeviceMoved(int from, int to, const DeviceInfoPointer info)
{
    dlWidget->updateItem(from, info);
    dlWidget->moveItem(from, to);
}

WorkspaceWidget::WorkspaceWidget(QWidget *parent)
    : QWidget(parent),
      d(new WorkspaceWidgetPrivate(this))
{
    d->initUI();
    d->initConnect();
}

int WorkspaceWidget::itemCount()
{
    return d->dlWidget->itemCount();
}

void WorkspaceWidget::switchWidget(PageName page)
{
    if (d->currentPage == page)
        return;

    d->currentPage = page;
    d->stackedLayout->setCurrentIndex(page);
}

void WorkspaceWidget::addDeviceInfos(const QList<DeviceInfoPointer> &infoList)
{
    Q_EMIT d->devicesAdded(infoList);
}

void WorkspaceWidget::removeDeviceInfos(const QString &ip)
{
    Q_EMIT d->devicesRemoved(ip);
}

void WorkspaceWidget::addDeviceOperation(const QVariantMap &map)
{
    d->dlWidget->addItemOperation(map);
}

DeviceInfoPointer WorkspaceWidget::findDeviceInfo(const QString &ip)
{
    return d->dlWidget->findDeviceInfo(ip);
}

void WorkspaceWidget::clear()
{
    d->dlWidget->clear();
    Q_EMIT d->clearDevice();
}

bool WorkspaceWidget::event(QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget *widget = childAt(mouseEvent->pos());
            if (widget) {
                widget->setFocus();
            }
        }
    }
    return QWidget::event(event);
}
