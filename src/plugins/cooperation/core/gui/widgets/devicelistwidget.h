// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEVICELISTWIDGET_H
#define DEVICELISTWIDGET_H

#include "global_defines.h"
#include "deviceitem.h"

#include <QScrollArea>
#include <QVBoxLayout>

namespace cooperation_core {

class DeviceListWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit DeviceListWidget(QWidget *parent = nullptr);

    void appendItem(const DeviceInfo &info);
    void insertItem(int index, const DeviceInfo &info);
    void removeItem(int index);
    void moveItem(int srcIndex, int toIndex);
    int indexOf(const QString &ipStr);
    int itemCount();

    void addItemOperation(const QVariantMap &map);

    void clear();

private:
    void initUI();

private:
    QVBoxLayout *mainLayout { nullptr };
    QList<DeviceItem::Operation> operationList;
};

}   // namespace cooperation_core

#endif   // DEVICELISTWIDGET_H
