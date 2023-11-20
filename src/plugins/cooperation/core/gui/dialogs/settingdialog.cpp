// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingdialog.h"
#include "settingdialog_p.h"
#include "global_defines.h"
#include "config/configmanager.h"

#include <QPainter>
#include <QEvent>
#include <QDir>
#include <QStandardPaths>

using namespace cooperation_core;

SettingDialogPrivate::SettingDialogPrivate(SettingDialog *qq)
    : QObject(qq),
      q(qq)
{
    findComboBoxInfo << tr("Everyone in the same LAN")
                     << tr("Not allow");

    connectComboBoxInfo << QPair<QString, QString>("display_right", tr("Screen right"))
                        << QPair<QString, QString>("display_left", tr("Screen left"));

    transferComboBoxInfo << tr("Everyone in the same LAN")
                         << tr("Only those who are collaborating are allowed")
                         << tr("Not allow");

    mainLayout = new QVBoxLayout(q);
    mainLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->setSpacing(10);
}

SettingDialogPrivate::~SettingDialogPrivate()
{
}

void SettingDialogPrivate::initWindow()
{
    q->setFixedSize(650, 520);

    contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    QScrollArea *contentArea = new QScrollArea(q);
    contentArea->setFrameShape(QFrame::NoFrame);
    contentArea->setWidgetResizable(true);

    QWidget *contentWidget = new QWidget(contentArea);
    contentWidget->installEventFilter(q);
    contentArea->setWidget(contentWidget);
    contentWidget->setLayout(contentLayout);

    mainWidget = new QWidget(q);
    mainWidget->installEventFilter(q);
    QHBoxLayout *layout = new QHBoxLayout(mainWidget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(contentArea);

    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(10, 0, 10, 0);
    hLayout->addWidget(mainWidget);

    mainLayout->addLayout(hLayout);

    createBasicWidget();
    createDeviceShareWidget();
    createTransferWidget();
    createClipboardShareWidget();
}

void SettingDialogPrivate::createBasicWidget()
{
    QLabel *basicLable = new QLabel(tr("Basic Settings"), q);
    QFont font = basicLable->font();
    font.setPointSize(16);
    font.setWeight(QFont::Medium);
    basicLable->setFont(font);

    findCB = new QComboBox(q);
    findCB->addItems(findComboBoxInfo);
    findCB->setFixedWidth(280);
    connect(findCB, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingDialogPrivate::onFindComboBoxValueChanged);
    SettingItem *findItem = new SettingItem(q);
    findItem->setItemInfo(tr("Discovery mode"), findCB);

    QLabel *tipLabel = new QLabel(tr("Discover and connect with you through the \"Cooperation\" app"), q);
    auto margins = tipLabel->contentsMargins();
    margins.setLeft(10);
    tipLabel->setContentsMargins(margins);
    tipLabel->setWordWrap(true);

    nameEdit = new CooperationLineEdit(q);
    nameEdit->setFixedWidth(280);
    connect(nameEdit, &CooperationLineEdit::editingFinished, this, &SettingDialogPrivate::onNameEditingFinished);
    connect(nameEdit, &CooperationLineEdit::textChanged, this, &SettingDialogPrivate::onNameChanged);
    SettingItem *nameItem = new SettingItem(q);
    nameItem->setItemInfo(tr("Device name"), nameEdit);

    contentLayout->addWidget(basicLable);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(findItem);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(tipLabel);
    contentLayout->addSpacing(16);
    contentLayout->addWidget(nameItem);
    contentLayout->addSpacing(10);
}

void SettingDialogPrivate::createDeviceShareWidget()
{
    devShareSwitchBtn = new CooperationSwitchButton(q);
    connect(devShareSwitchBtn, &CooperationSwitchButton::clicked, this, &SettingDialogPrivate::onDeviceShareButtonClicked);

    SettingItem *deviceShareItem = new SettingItem(q);
    deviceShareItem->setItemInfo(tr("Device share"), devShareSwitchBtn);

    QLabel *tipLabel = new QLabel(tr("Allows peripherals that have been established "
                                     "to collaborate across devices to control this "
                                     "device, including keyboard, mouse, trackpad, etc"),
                                  q);
    auto margins = tipLabel->contentsMargins();
    margins.setLeft(10);
    tipLabel->setContentsMargins(margins);
    tipLabel->setWordWrap(true);

    connectCB = new QComboBox(q);
    connectCB->setFixedWidth(280);
    for (const auto &info : connectComboBoxInfo) {
        connectCB->addItem(QIcon::fromTheme(info.first), info.second);
    }
    connect(connectCB, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingDialogPrivate::onConnectComboBoxValueChanged);
    connectItem = new SettingItem(q);
    connectItem->setItemInfo(tr("Connection direction"), connectCB);

    contentLayout->addWidget(deviceShareItem);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(tipLabel);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(connectItem);
    contentLayout->addSpacing(10);
}

void SettingDialogPrivate::createTransferWidget()
{
    transferCB = new QComboBox(q);
    transferCB->addItems(transferComboBoxInfo);
    transferCB->setFixedWidth(280);
    connect(transferCB, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingDialogPrivate::onTransferComboBoxValueChanged);
    SettingItem *transferItem = new SettingItem(q);
    transferItem->setItemInfo(tr("Allows the following users to send files to me"), transferCB);

    chooserEdit = new FileChooserEdit(q);
    chooserEdit->setFixedWidth(280);
    connect(chooserEdit, &FileChooserEdit::fileChoosed, this, &SettingDialogPrivate::onFileChoosed);

    SettingItem *fileSaveItem = new SettingItem(q);
    fileSaveItem->setItemInfo(tr("File save location"), chooserEdit);

    contentLayout->addWidget(transferItem);
    contentLayout->addSpacing(10);
    contentLayout->addWidget(fileSaveItem);
    contentLayout->addSpacing(10);
}

void SettingDialogPrivate::createClipboardShareWidget()
{
    clipShareSwitchBtn = new CooperationSwitchButton(q);
    connect(clipShareSwitchBtn, &CooperationSwitchButton::clicked, this, &SettingDialogPrivate::onClipboardShareButtonClicked);
    SettingItem *clipShareItem = new SettingItem(q);
    clipShareItem->setItemInfo(tr("Share clipboard"), clipShareSwitchBtn);

    QLabel *tipLabel = new QLabel(tr("The clipboard is shared between devices"), q);
    auto margins = tipLabel->contentsMargins();
    margins.setLeft(10);
    tipLabel->setContentsMargins(margins);
    tipLabel->setWordWrap(true);

    contentLayout->addWidget(clipShareItem);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(tipLabel);
}

void SettingDialogPrivate::onFindComboBoxValueChanged(int index)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey, index);
}

void SettingDialogPrivate::onConnectComboBoxValueChanged(int index)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey, index);
}

void SettingDialogPrivate::onTransferComboBoxValueChanged(int index)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey, index);
}

void SettingDialogPrivate::onNameEditingFinished()
{
    int length = nameEdit->text().length();
    if (length < 1 || length > 63) {
        nameEdit->setAlert(true);
        nameEdit->showAlertMessage(tr("The device name must contain 1 to 63 characters"));
        return;
    }

    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey, nameEdit->text());
}

void SettingDialogPrivate::onNameChanged(const QString &text)
{
    Q_UNUSED(text);

    if (nameEdit->isAlert())
        nameEdit->setAlert(false);
}

void SettingDialogPrivate::onDeviceShareButtonClicked(bool clicked)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey, clicked);
    connectItem->setVisible(clicked);
}

void SettingDialogPrivate::onClipboardShareButtonClicked(bool clicked)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey, clicked);
}

void SettingDialogPrivate::onFileChoosed(const QString &path)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey, path);
}

SettingDialog::SettingDialog(QWidget *parent)
    : CooperationAbstractDialog(parent),
      d(new SettingDialogPrivate(this))
{
    d->initWindow();
    d->initTitleBar();
}

SettingDialog::~SettingDialog()
{
}

bool SettingDialog::eventFilter(QObject *watched, QEvent *event)
{
    // 绘制背景
    if (watched == d->mainWidget && event->type() == QEvent::Paint) {
        QPainter painter(d->mainWidget);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(248, 248, 248));

        painter.drawRoundedRect(d->mainWidget->rect(), 8, 8);
        return true;
    }

    return CooperationAbstractDialog::eventFilter(watched, event);
}

void SettingDialog::showEvent(QShowEvent *event)
{
    loadConfig();
    CooperationAbstractDialog::showEvent(event);
}

void SettingDialog::loadConfig()
{
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    d->findCB->setCurrentIndex(value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    d->nameEdit->setText(value.isValid()
                                 ? value.toString()
                                 : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1));

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    d->devShareSwitchBtn->setChecked(value.isValid() ? value.toBool() : false);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    d->connectCB->setCurrentIndex(value.isValid() ? value.toInt() : 0);
    d->connectItem->setVisible(d->devShareSwitchBtn->isChecked());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    d->transferCB->setCurrentIndex(value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    d->chooserEdit->setText(value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    d->clipShareSwitchBtn->setChecked(value.isValid() ? value.toBool() : false);
}
