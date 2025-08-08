// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deviceitem.h"
#include "buttonboxwidget.h"
#include "gui/utils/cooperationguihelper.h"
#include "common/log.h"

#ifdef linux
#    include <DPalette>
#endif
#ifdef DTKWIDGET_CLASS_DSizeMode
#    include <DSizeMode>
DWIDGET_USE_NAMESPACE
#endif

#include <QIcon>
#include <QVBoxLayout>
#include <QPainter>
#include <QBitmap>

using namespace cooperation_core;

#ifdef __linux__
static constexpr char Kcomputer_connected[] = "computer_connected";
static constexpr char Kcomputer_can_connect[] = "computer_can_connect";
static constexpr char Kcomputer_off_line[] = "computer_off_line";
#else
const char *Kcomputer_connected = ":/icons/deepin/builtin/icons/computer_connected_52px.svg";
const char *Kcomputer_can_connect = ":/icons/deepin/builtin/icons/computer_can_connect_52px.svg";
const char *Kcomputer_off_line = ":/icons/deepin/builtin/icons/computer_off_line_52px.svg";
#endif

StateLabel::StateLabel(QWidget *parent)
    : CooperationLabel(parent)
{
    DLOG << "Initializing StateLabel";
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
        DLOG << "Device status: Connected";
        brushColor.setRgb(241, 255, 243);
        textColor.setRgb(51, 202, 78);
        if (CooperationGuiHelper::isDarkTheme()) {
            DLOG << "Dark theme detected for Connected status";
            brushColor.setRgb(63, 70, 64, static_cast<int>(255 * 0.2));
            textColor.setRgb(67, 159, 83);
        }
        break;
    case DeviceInfo::Connectable:
        DLOG << "Device status: Connectable";
        brushColor.setRgb(56, 127, 247, 22);
        textColor.setRgb(0, 130, 250);
        if (CooperationGuiHelper::isDarkTheme()) {
            DLOG << "Dark theme detected for Connectable status";
            brushColor.setRgb(26, 84, 182, static_cast<int>(255 * 0.2));
            textColor.setRgb(0, 105, 202);
        }
        break;
    case DeviceInfo::Offline:
    default:
        DLOG << "Device status: Offline or Unknown";
        brushColor.setRgb(0, 0, 0, 25);
        textColor.setRgb(0, 0, 0, 128);
        if (CooperationGuiHelper::isDarkTheme()) {
            DLOG << "Dark theme detected for Offline status";
            brushColor.setRgb(255, 255, 255, static_cast<int>(255 * 0.05));
            textColor.setRgb(255, 255, 255, static_cast<int>(255 * 0.4));
        }
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
    DLOG << "Creating device item";
    initUI();
    initConnect();
    DLOG << "Device item created";
}

DeviceItem::~DeviceItem()
{
    DLOG << "Destroying device item";
    isDestroyed.store(true, std::memory_order_release);
    devInfo.reset();
    disconnect(btnBoxWidget, &ButtonBoxWidget::buttonClicked, this, &DeviceItem::onButtonClicked);
    DLOG << "Device item destroyed";
}

void DeviceItem::setDeviceInfo(const DeviceInfoPointer info)
{
    DLOG << "Setting device info for:" << info->ipAddress().toStdString();
    devInfo = info;
    setDeviceName(info->deviceName());
    setDeviceStatus(info->connectStatus());
    ipLabel->setText(info->ipAddress());

    update();
    updateOperations();
    DLOG << "Device info set";
}

DeviceInfoPointer DeviceItem::deviceInfo() const
{
    return devInfo;
}

void DeviceItem::initUI()
{
    DLOG << "Initializing device item";
    setFixedSize(480, qApp->property("onlyTransfer").toBool() ? 72 : 90);
    setBackground(8, NoType, TopAndBottom);

    iconLabel = new CooperationLabel(this);
    nameLabel = new CooperationLabel(this);
    nameLabel->installEventFilter(this);
    CooperationGuiHelper::setAutoFont(nameLabel, 14, QFont::Medium);

    ipLabel = new CooperationLabel(this);
    CooperationGuiHelper::setAutoFont(ipLabel, 12, QFont::Medium);
#ifdef linux
    ipLabel->setForegroundRole(DTK_GUI_NAMESPACE::DPalette::TextTips);
#endif

    stateLabel = new StateLabel();
    stateLabel->setContentsMargins(8, 2, 8, 2);
    CooperationGuiHelper::setAutoFont(stateLabel, 11, QFont::Medium);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->setSpacing(2);
    vLayout->setContentsMargins(0, 10, 0, 10);
    vLayout->addWidget(nameLabel);
    vLayout->addWidget(ipLabel);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    if (!qApp->property("onlyTransfer").toBool()) {
        DLOG << "onlyTransfer property is false, adding stateLabel to layout";
        hLayout->addWidget(stateLabel);
    }
    hLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding));
    vLayout->addLayout(hLayout);

    btnBoxWidget = new ButtonBoxWidget(this);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(10, 0, 10, 0);
    mainLayout->addWidget(iconLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    mainLayout->addLayout(vLayout, 0);
    mainLayout->addWidget(btnBoxWidget, 0, Qt::AlignRight);
    setLayout(mainLayout);
    DLOG << "Device item layout initialized";
}

void DeviceItem::initConnect()
{
    DLOG << "Initializing device item connections";
    connect(btnBoxWidget, &ButtonBoxWidget::buttonClicked, this, &DeviceItem::onButtonClicked);
}

void DeviceItem::setDeviceName(const QString &name)
{
    DLOG << "Setting device name:" << name.toStdString();
    QFontMetrics fm(nameLabel->font());
    auto showName = fm.elidedText(name, Qt::ElideMiddle, 385);

    nameLabel->setText(showName);
    if (showName != name) {
        DLOG << "Name truncated, setting tooltip";
        nameLabel->setToolTip(name);
    }
    DLOG << "Device name set to:" << showName.toStdString();
}

void DeviceItem::setDeviceStatus(DeviceInfo::ConnectStatus status)
{
    DLOG << "Setting device status to:" << (int)status;
    stateLabel->setState(status);
    switch (status) {
    case DeviceInfo::Connected: {
        DLOG << "Setting status to Connected";
        bool isPC = devInfo->deviceType() == DeviceInfo::DeviceType::PC;
        QIcon icon = QIcon::fromTheme(isPC ? Kcomputer_connected : "connect_phone");
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("connected"));
        DLOG << "Device status set to Connected";
    } break;
    case DeviceInfo::Connectable: {
        DLOG << "Setting status to Connectable";
        QIcon icon = QIcon::fromTheme(Kcomputer_can_connect);
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("connectable"));
        DLOG << "Device status set to Connectable";
    } break;
    case DeviceInfo::Offline:
    default: {
        DLOG << "Setting status to Offline or Unknown";
        QIcon icon = QIcon::fromTheme(Kcomputer_off_line);
        iconLabel->setPixmap(icon.pixmap(52, 52));
        stateLabel->setText(tr("offline"));
        DLOG << "Device status set to Offline";
    } break;
    }
}

void DeviceItem::setOperations(const QList<Operation> &operations)
{
    DLOG << "Setting" << operations.size() << "operations";
    auto tmpOperaList = operations;
    tmpOperaList << indexOperaMap.values();

    std::sort(tmpOperaList.begin(), tmpOperaList.end(),
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
    DLOG << "Operations set";
}

void DeviceItem::updateOperations()
{
    DLOG << "Updating operations visibility";
    // this device item may be not visible after it's been removed from the list, e.g. network disconnected.
    if (!isAlive()) {
        DLOG << "Device item destroyed during callback, skipping further operations";
        return;
    }

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
    DLOG << "Operations updated";
}

void DeviceItem::onButtonClicked(int index)
{
    DLOG << "Button clicked with index:" << index;
    if (!indexOperaMap.contains(index)) {
        WLOG << "Invalid operation index:" << index;
        return;
    }

    if (indexOperaMap[index].clickedCb) {
        indexOperaMap[index].clickedCb(indexOperaMap[index].id, devInfo);
    }

    updateOperations();
    DLOG << "Operation handled";
}

void DeviceItem::enterEvent(EnterEvent *event)
{
    DLOG << "Mouse entered";
    updateOperations();
    btnBoxWidget->setVisible(true);
    BackgroundWidget::enterEvent(event);
    DLOG << "Mouse enter handled";
}

void DeviceItem::leaveEvent(QEvent *event)
{
    DLOG << "Mouse left";
    btnBoxWidget->setVisible(false);
    BackgroundWidget::leaveEvent(event);
    DLOG << "Mouse leave handled";
}

void DeviceItem::showEvent(QShowEvent *event)
{
    DLOG << "Item show event";
    if (hasFocus()) {
        DLOG << "Item has focus, updating operations";
        updateOperations();
    } else {
        DLOG << "Item has no focus, hiding operations";
        btnBoxWidget->setVisible(false);
    }

    BackgroundWidget::showEvent(event);
}

bool DeviceItem::eventFilter(QObject *watched, QEvent *event)
{
    // Device name mask effect, implemented with gradient font color
    if (watched == nameLabel && event->type() == QEvent::Paint && btnBoxWidget->isVisible()) {
        DLOG << "Painting name label";
        QPainter painter(nameLabel);
        QLinearGradient lg(nameLabel->rect().topLeft(), nameLabel->rect().bottomRight());
        lg.setColorAt(0.8, nameLabel->palette().windowText().color());
        lg.setColorAt(1, nameLabel->palette().window().color());
        painter.setPen(QPen(lg, nameLabel->font().weight()));

        painter.drawText(nameLabel->rect(), static_cast<int>(nameLabel->alignment()), nameLabel->text());
        return true;
    }

    return BackgroundWidget::eventFilter(watched, event);
}
