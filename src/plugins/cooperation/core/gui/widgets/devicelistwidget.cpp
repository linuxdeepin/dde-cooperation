﻿// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicelistwidget.h"

#include "co/co.h"

#include <QScrollBar>
#include <QVariantMap>

using namespace cooperation_core;

DeviceListWidget::DeviceListWidget(QWidget *parent)
    : QScrollArea(parent)
{
    initUI();
}

void DeviceListWidget::initUI()
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    horizontalScrollBar()->setDisabled(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *mainWidget = new QWidget(this);

    mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    mainLayout->addSpacing(8);
    mainWidget->setLayout(mainLayout);

    setWidget(mainWidget);
    setWidgetResizable(true);
    setFrameShape(NoFrame);
}

void DeviceListWidget::appendItem(const DeviceInfoPointer info)
{
    insertItem(mainLayout->count(), info);
}

void DeviceListWidget::insertItem(int index, const DeviceInfoPointer info)
{
    DeviceItem *item = new DeviceItem(this);
    item->setDeviceInfo(info);
    item->setOperations(operationList);

    mainLayout->insertWidget(index, item);
}

void DeviceListWidget::updateItem(int index, const DeviceInfoPointer info)
{
    QLayoutItem *item = mainLayout->itemAt(index);
    DeviceItem *devItem = qobject_cast<DeviceItem *>(item->widget());
    if (!devItem) {
        LOG << "Can not find this item, index: " << index << " ip address: " << info->ipAddress().toStdString();
        return;
    }

    devItem->setDeviceInfo(info);
}

void DeviceListWidget::removeItem(int index)
{
    QLayoutItem *item = mainLayout->takeAt(index);
    if(!item)
        return;
    QWidget *w = item->widget();
    if (w) {
        w->setParent(nullptr);
        w->deleteLater();
    }

    delete item;
}

void DeviceListWidget::moveItem(int srcIndex, int toIndex)
{
    if (srcIndex == toIndex)
        return;

    QLayoutItem *item = mainLayout->itemAt(srcIndex);
    if (!item)
        return;

    mainLayout->insertItem(toIndex, item);
    removeItem(srcIndex);
}

int DeviceListWidget::indexOf(const QString &ipStr)
{
    const int count = mainLayout->count();
    for (int i = 0; i != count; ++i) {
        QLayoutItem *item = mainLayout->itemAt(i);
        DeviceItem *w = qobject_cast<DeviceItem *>(item->widget());
        if (!w)
            continue;

        if (w->deviceInfo()->ipAddress() == ipStr)
            return i;
    }

    return -1;
}

int DeviceListWidget::itemCount()
{
    return mainLayout->count();
}

void DeviceListWidget::addItemOperation(const QVariantMap &map)
{
    DeviceItem::Operation op;
    op.description = map.value(OperationKey::kDescription).toString();
    op.id = map.value(OperationKey::kID).toString();
    op.icon = map.value(OperationKey::kIconName).toString();
    op.style = map.value(OperationKey::kButtonStyle).toInt();
    op.location = map.value(OperationKey::kLocation).toInt();
    op.clickedCb = map.value(OperationKey::kClickedCallback).value<DeviceItem::ClickedCallback>();
    op.visibleCb = map.value(OperationKey::kVisibleCallback).value<DeviceItem::ButtonStateCallback>();
    op.clickableCb = map.value(OperationKey::kClickableCallback).value<DeviceItem::ButtonStateCallback>();

    operationList << op;
}

void DeviceListWidget::clear()
{
    const int count = mainLayout->count();
    for (int i = 0; i != count; ++i) {
        removeItem(0);
    }
}
