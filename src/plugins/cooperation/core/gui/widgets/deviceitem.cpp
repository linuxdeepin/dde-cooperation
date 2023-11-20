// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deviceitem.h"
#include "buttonboxwidget.h"

#include <QIcon>
#include <QVBoxLayout>
#include <QPainter>

using namespace cooperation_core;

StateLabel::StateLabel(QWidget *parent)
    : QLabel(parent)
{
}

void StateLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QColor brushColor;
    QColor textColor;
    switch (st) {
    case DeviceInfo::Connected:
        brushColor.setRgb(241, 255, 243);
        textColor.setRgb(51, 202, 78);
        break;
    case DeviceInfo::Connectable:
        brushColor.setRgb(56, 127, 247, 22);
        textColor.setRgb(0, 130, 250);
        break;
    case DeviceInfo::Offline:
        brushColor.setRgb(0, 0, 0, 25);
        textColor.setRgb(0, 0, 0, 128);
        break;
    }

    painter.setBrush(brushColor);
    painter.drawRoundedRect(rect(), 8, 8);

    painter.setPen(textColor);
    painter.drawText(rect(), Qt::AlignCenter, text());
}

DeviceItem::DeviceItem(QWidget *parent)
    : BackgroundWidget(parent)
{
    initUI();
    initConnect();
}

DeviceItem::~DeviceItem()
{
}

void DeviceItem::setDeviceInfo(const DeviceInfoPointer info)
{
    devInfo = info;
    setDeviceName(info->deviceName());
    setDeviceStatus(info->connectStatus());
    ipLabel->setText(info->ipAddress());

    update();
}

DeviceInfoPointer DeviceItem::deviceInfo() const
{
    return devInfo;
}

void DeviceItem::initUI()
{
    setFixedSize(460, 90);
    setBackground(8, NoType, TopAndBottom);

    iconLabel = new QLabel(this);
    nameLabel = new QLabel(this);
    setLabelFont(nameLabel, 14, QFont::Medium);

    ipLabel = new QLabel(this);
    setLabelFont(ipLabel, 12, QFont::Medium);

    stateLabel = new StateLabel(this);
    stateLabel->setContentsMargins(8, 2, 8, 2);
    setLabelFont(stateLabel, 10, QFont::Medium);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSpacing(2);
    vLayout->setContentsMargins(0, 10, 0, 10);
    vLayout->addWidget(nameLabel);
    vLayout->addWidget(ipLabel);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->addWidget(stateLabel);
    hLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    vLayout->addLayout(hLayout);

    btnBoxWidget = new ButtonBoxWidget(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(10, 0, 10, 0);
    mainLayout->addWidget(iconLabel, 0, Qt::AlignLeft);
    mainLayout->addLayout(vLayout, 0);
    mainLayout->addWidget(btnBoxWidget, 1, Qt::AlignRight);
    setLayout(mainLayout);
}

void DeviceItem::initConnect()
{
    connect(btnBoxWidget, &ButtonBoxWidget::buttonClicked, this, &DeviceItem::onButtonClicked);
}

void DeviceItem::setLabelFont(QLabel *label, int pointSize, int weight)
{
    QFont font = this->font();
    font.setPointSize(pointSize);
    font.setWeight(weight);

    label->setFont(font);
}

void DeviceItem::setDeviceName(const QString &name)
{
    QFontMetrics fm(nameLabel->font());
    int width = 385 - (btnBoxWidget->isVisible() ? btnBoxWidget->width() : 0);
    auto showName = fm.elidedText(name, Qt::ElideMiddle, width);

    nameLabel->setText(showName);
    if (showName != name)
        nameLabel->setToolTip(name);
}

void DeviceItem::setDeviceStatus(DeviceInfo::ConnectStatus status)
{
    stateLabel->setState(status);
    switch (status) {
    case DeviceInfo::Connected: {
        QIcon icon = QIcon::fromTheme("computer_connected");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("connected"));
    } break;
    case DeviceInfo::Connectable: {
        QIcon icon = QIcon::fromTheme("computer_can_connect");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("connectable"));
    } break;
    case DeviceInfo::Offline: {
        QIcon icon = QIcon::fromTheme("computer_off_line");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("offline"));
    } break;
    }
}

void DeviceItem::setOperations(const QList<Operation> &operations)
{
    auto tmpOperaList = operations;
    tmpOperaList << indexOperaMap.values();

    qSort(tmpOperaList.begin(), tmpOperaList.end(),
          [](const Operation &op1, const Operation &op2) {
              if (op1.location < op2.location)
                  return true;

              return false;
          });

    for (auto op : tmpOperaList) {
        int index = btnBoxWidget->addButton(QIcon::fromTheme(op.icon), op.description,
                                            static_cast<ButtonBoxWidget::ButtonStyle>(op.style));
        indexOperaMap.insert(index, op);
    }
}

void DeviceItem::updateOperations()
{
    auto iter = indexOperaMap.begin();
    for (; iter != indexOperaMap.end(); ++iter) {
        if (!iter.value().visibleCb)
            continue;

        bool visible = iter.value().visibleCb(iter.value().id, devInfo);
        btnBoxWidget->setButtonVisible(iter.key(), visible);

        if (!iter.value().clickableCb)
            continue;

        bool clickable = iter.value().clickableCb(iter.value().id, devInfo);
        btnBoxWidget->setButtonClickable(iter.key(), clickable);
    }
}

void DeviceItem::onButtonClicked(int index)
{
    if (!indexOperaMap.contains(index))
        return;

    if (indexOperaMap[index].clickedCb)
        indexOperaMap[index].clickedCb(indexOperaMap[index].id, devInfo);

    updateOperations();
}

void DeviceItem::enterEvent(QEvent *event)
{
    updateOperations();
    btnBoxWidget->setVisible(true);
    setDeviceName(devInfo->deviceName());
    BackgroundWidget::enterEvent(event);
}

void DeviceItem::leaveEvent(QEvent *event)
{
    btnBoxWidget->setVisible(false);
    setDeviceName(devInfo->deviceName());
    BackgroundWidget::leaveEvent(event);
}

void DeviceItem::showEvent(QShowEvent *event)
{
    if (hasFocus()) {
        updateOperations();
    } else {
        btnBoxWidget->setVisible(false);
    }

    BackgroundWidget::showEvent(event);
}
