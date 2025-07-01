// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicelistwidget.h"
#include "common/log.h"

#include <QScrollBar>
#include <QVariantMap>

using namespace cooperation_core;

DeviceListWidget::DeviceListWidget(QWidget *parent)
    : QScrollArea(parent)
{
    DLOG << "Initializing device list widget";
    initUI();
    DLOG << "Initialization completed";
}

void DeviceListWidget::initUI()
{
    DLOG << "Initializing UI components";
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    horizontalScrollBar()->setDisabled(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
    mainLayout->setSpacing(10);

#ifndef linux
    QString scrollBarStyle = "QScrollBar:vertical {"
                             "    background: lightGrey;"
                             "    width: 8px;"
                             "    border-radius: 4px;"
                             "}"
                             "QScrollBar::handle:vertical {"
                             "    background: darkGrey;"
                             "    min-height: 30px;"
                             "    border-radius: 4px;"
                             "}"
                             "QScrollBar::sub-line:vertical, QScrollBar::add-line:vertical {"
                             "    height: 0px;"
                             "}";
    verticalScrollBar()->setStyleSheet(scrollBarStyle);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int min, int max) {
        DLOG << "Scroll bar range changed, min:" << min << "max:" << max;
        if (max) {
            DLOG << "Max scroll value is not zero, setting margins";
            mainLayout->setContentsMargins(10, 0, 0, 10);
        } else {
            DLOG << "Max scroll value is zero, setting default margins";
            mainLayout->setContentsMargins(0, 0, 0, 10);
        }
    });
    mainLayout->setContentsMargins(0, 0, 0, 10);
#else
    DLOG << "Linux platform, setting default margins";
    mainLayout->setContentsMargins(0, 0, 0, 10);
#endif

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);
    setWidgetResizable(true);
    setFrameShape(NoFrame);
    DLOG << "UI initialization completed";
}

bool DeviceListWidget::event(QEvent *e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        DLOG << "Mouse button press event detected";
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
        if (mouseEvent->button() == Qt::LeftButton) {
            DLOG << "Left mouse button pressed, accepting event";
            return true;
        }
    }

    return QScrollArea::event(e);
}

void DeviceListWidget::appendItem(const DeviceInfoPointer info)
{
    insertItem(mainLayout->count(), info);
    DLOG << "Device appended";
}

void DeviceListWidget::insertItem(int index, const DeviceInfoPointer info)
{
    DLOG << "Inserting device at index:" << index << "IP:" << info->ipAddress().toStdString();
    DeviceItem *item = new DeviceItem(this);
    item->setDeviceInfo(info);
    item->setOperations(operationList);

    mainLayout->insertWidget(index, item);
    DLOG << "Device inserted";
}

void DeviceListWidget::updateItem(int index, const DeviceInfoPointer info)
{
    DLOG << "Updating device at index:" << index << "IP:" << info->ipAddress().toStdString();
    if (!info || index < 0 || index >= mainLayout->count()) {
        WLOG << "Invalid index or info, index: " << index;
        return;
    }

    QLayoutItem *item = mainLayout->itemAt(index);
    DeviceItem *devItem = qobject_cast<DeviceItem *>(item->widget());
    if (!devItem) {
        LOG << "Can not find this item, index: " << index << " ip address: " << info->ipAddress().toStdString();
        return;
    }

    devItem->setDeviceInfo(info);
    DLOG << "Device updated";
}

void DeviceListWidget::removeItem(int index)
{
    DLOG << "Removing device at index:" << index;
    QLayoutItem *item = mainLayout->takeAt(index);
    if (!item) {
        WLOG << "No item found at index:" << index;
        return;
    }
    QWidget *w = item->widget();
    if (w) {
        w->setParent(nullptr);
        w->deleteLater();
    }

    delete item;
    DLOG << "Device removed";
}

void DeviceListWidget::moveItem(int srcIndex, int toIndex)
{
    DLOG << "Moving device from index:" << srcIndex << "to:" << toIndex;
    if (srcIndex == toIndex) {
        DLOG << "Source and target indexes are same, no move needed";
        return;
    }

    QLayoutItem *item = mainLayout->takeAt(srcIndex);
    if (!item) {
        WLOG << "No item found at source index:" << srcIndex;
        return;
    }

    mainLayout->insertItem(toIndex, item);
    DLOG << "Device moved";
}

int DeviceListWidget::indexOf(const QString &ipStr)
{
    DLOG << "Searching for device with IP:" << ipStr.toStdString();
    const int count = mainLayout->count();
    for (int i = 0; i != count; ++i) {
        QLayoutItem *item = mainLayout->itemAt(i);
        DeviceItem *w = qobject_cast<DeviceItem *>(item->widget());
        if (!w) {
            DLOG << "Item at index" << i << "is not a DeviceItem, skipping";
            continue;
        }

        if (w->deviceInfo()->ipAddress() == ipStr) {
            DLOG << "Device found at index:" << i;
            return i;
        }
    }

    DLOG << "Device not found";
    return -1;
}

DeviceInfoPointer DeviceListWidget::findDeviceInfo(const QString &ipStr)
{
    DLOG << "Searching for device with IP:" << ipStr.toStdString();
    const int count = mainLayout->count();
    for (int i = 0; i != count; ++i) {
        QLayoutItem *item = mainLayout->itemAt(i);
        DeviceItem *w = qobject_cast<DeviceItem *>(item->widget());
        if (!w) {
            DLOG << "Item at index" << i << "is not a DeviceItem, skipping";
            continue;
        }

        if (w->deviceInfo()->ipAddress() == ipStr) {
            DLOG << "Device found at index:" << i;
            return w->deviceInfo();
        }
    }

    DLOG << "Device not found";
    return nullptr;
}

int DeviceListWidget::itemCount()
{
    DLOG << "Current item count:" << mainLayout->count();
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
    DLOG << "Clearing device list";
    const int count = mainLayout->count();
    DLOG << "Removing" << count << "devices";
    for (int i = 0; i != count; ++i) {
        removeItem(0);
    }
    DLOG << "Device list cleared";
}
