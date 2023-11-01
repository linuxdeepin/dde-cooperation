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
    case kConnected:
        brushColor.setRgb(241, 255, 243);
        textColor.setRgb(51, 202, 78);
        break;
    case kConnectable:
        brushColor.setRgb(56, 127, 247, 22);
        textColor.setRgb(0, 130, 250);
        break;
    case kOffline:
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

void DeviceItem::initUI()
{
    setFixedHeight(90);
    setBackground(8, kNoType);

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
    mainLayout->setContentsMargins(10, 0, 0, 0);
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
    devName = name;
    QFontMetrics fm(nameLabel->font());
    int width = 390 - (btnBoxWidget->isVisible() ? btnBoxWidget->width() : 0);
    auto showName = fm.elidedText(name, Qt::ElideMiddle, width);

    nameLabel->setText(showName);
    if (showName != devName)
        nameLabel->setToolTip(devName);
}

QString DeviceItem::deviceName() const
{
    return devName;
}

void DeviceItem::setIPText(const QString &ipStr)
{
    ipLabel->setText(ipStr);
}

QString DeviceItem::ipText() const
{
    return ipLabel->text();
}

void DeviceItem::setDeviceState(ConnectState state)
{
    stateLabel->setState(state);
    switch (state) {
    case kConnected: {
        QIcon icon = QIcon::fromTheme("computer_connected");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("connected"));
    } break;
    case kConnectable: {
        QIcon icon = QIcon::fromTheme("computer_can_connect");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("connectable"));
    } break;
    case kOffline: {
        QIcon icon = QIcon::fromTheme("computer_off_line");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("offline"));
    } break;
    }
}

ConnectState DeviceItem::deviceState() const
{
    return stateLabel->state();
}

void DeviceItem::setOperations(const QList<Operation> &operations)
{
    btnBoxWidget->clear();
    indexOperaMap.clear();

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
    QVariantMap map;
    map.insert("device", nameLabel->text());
    map.insert("ip", ipLabel->text());
    map.insert("state", stateLabel->state());

    auto iter = indexOperaMap.begin();
    for (; iter != indexOperaMap.end(); ++iter) {
        if (!iter.value().visibleCb)
            continue;

        map.insert("id", iter.value().id);
        bool visible = iter.value().visibleCb(map);
        btnBoxWidget->setButtonVisible(iter.key(), visible);

        if (!iter.value().clickableCb)
            continue;

        bool clickable = iter.value().clickableCb(map);
        btnBoxWidget->setButtonClickable(iter.key(), clickable);
    }
}

void DeviceItem::onButtonClicked(int index)
{
    if (!indexOperaMap.contains(index))
        return;

    QVariantMap map;
    map.insert("device", nameLabel->text());
    map.insert("ip", ipLabel->text());
    map.insert("state", stateLabel->state());
    map.insert("id", indexOperaMap[index].id);

    if (indexOperaMap[index].clickedCb)
        indexOperaMap[index].clickedCb(map);

    updateOperations();
}

void DeviceItem::enterEvent(QEvent *event)
{
    updateOperations();
    btnBoxWidget->setVisible(true);
    setDeviceName(devName);
    BackgroundWidget::enterEvent(event);
}

void DeviceItem::leaveEvent(QEvent *event)
{
    btnBoxWidget->setVisible(false);
    setDeviceName(devName);
    BackgroundWidget::leaveEvent(event);
}

void DeviceItem::showEvent(QShowEvent *event)
{
    if (hasFocus()) {
        updateOperations();
        setDeviceName(devName);
    } else {
        btnBoxWidget->setVisible(false);
        setDeviceName(devName);
    }

    BackgroundWidget::showEvent(event);
}
