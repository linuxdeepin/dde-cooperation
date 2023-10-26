// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicelistwidget.h"

using namespace cooperation_workspace;

DeviceListWidget::DeviceListWidget(QWidget *parent)
    : QScrollArea(parent)
{
    initUI();
}

void DeviceListWidget::initUI()
{
    QWidget *mainWidget = new QWidget(this);

    mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->addSpacing(8);
    mainWidget->setLayout(mainLayout);

    setWidget(mainWidget);
    setWidgetResizable(true);
    setFrameShape(NoFrame);
}

void DeviceListWidget::appendItem(const DeviceInfo &info)
{
    insertItem(mainLayout->count(), info);
}

void DeviceListWidget::insertItem(int index, const DeviceInfo &info)
{
    DeviceItem *item = new DeviceItem(this);
    item->setDeviceName(info.deviceName);
    item->setIPText(info.ipStr);
    item->setDeviceState(info.state);
    item->setOperations(operationList);

    mainLayout->insertWidget(index, item);
}

void DeviceListWidget::removeItem(int index)
{
    QLayoutItem *item = mainLayout->takeAt(index);
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

    QLayoutItem *item = mainLayout->takeAt(srcIndex);
    if (!item)
        return;

    mainLayout->insertItem(toIndex, item);
}

int DeviceListWidget::indexOf(const QString &ipStr)
{
    const int count = mainLayout->count();
    for (int i = 0; i != count; ++i) {
        QLayoutItem *item = mainLayout->takeAt(i);
        DeviceItem *w = qobject_cast<DeviceItem *>(item->widget());
        if (!w)
            continue;

        if (w->ipText() == ipStr)
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