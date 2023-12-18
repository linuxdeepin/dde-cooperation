// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingdialog.h"
#include "settingdialog_p.h"
#include "global_defines.h"
#include "utils/cooperationguihelper.h"
#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"

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
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
}

SettingDialogPrivate::~SettingDialogPrivate()
{
}

void SettingDialogPrivate::initWindow()
{
    q->setFixedSize(650, 520);

    contentLayout = new QVBoxLayout;
    contentLayout->setContentsMargins(10, 0, 10, 0);
    contentLayout->setSpacing(0);

    QScrollArea *contentArea = new QScrollArea(q);
    contentArea->setFrameShape(QFrame::NoFrame);
    contentArea->setWidgetResizable(true);

    QWidget *contentWidget = new QWidget(contentArea);
    contentWidget->installEventFilter(q);
    contentWidget->setObjectName("ContentWidget");
    contentArea->setWidget(contentWidget);
    contentWidget->setLayout(contentLayout);

    QWidget *mainWidget = new QWidget(q);
    mainWidget->installEventFilter(q);
    mainWidget->setObjectName("MainWidget");
    QHBoxLayout *layout = new QHBoxLayout(mainWidget);
    layout->setContentsMargins(0, 10, 0, 10);
    layout->addWidget(contentArea);

    QWidget *backgroundWidget = new QWidget(q);
    backgroundWidget->setObjectName("BackgroundWidget");
    backgroundWidget->installEventFilter(q);
    QHBoxLayout *hLayout = new QHBoxLayout(backgroundWidget);
    hLayout->setContentsMargins(10, 10, 10, 10);
    hLayout->addWidget(mainWidget);

    mainLayout->addWidget(backgroundWidget);

    initFont();
    createBasicWidget();
    createDeviceShareWidget();
    createTransferWidget();
    createClipboardShareWidget();
}

void SettingDialogPrivate::createBasicWidget()
{
    QLabel *basicLable = new QLabel(tr("Basic Settings"), q);
    auto cm = basicLable->contentsMargins();
    cm.setLeft(10);
    basicLable->setContentsMargins(cm);
    basicLable->setFont(groupFont);

    findCB = new QComboBox(q);
    findCB->addItems(findComboBoxInfo);
    findCB->setFixedWidth(280);
    connect(findCB, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingDialogPrivate::onFindComboBoxValueChanged);
    SettingItem *findItem = new SettingItem(q);
    findItem->setItemInfo(tr("Discovery mode"), findCB);

    QLabel *tipLabel = new QLabel(tr("Other devices can discover and connect with you through the \"Cooperation\" app"), q);
    auto margins = tipLabel->contentsMargins();
    margins.setLeft(10);
    tipLabel->setContentsMargins(margins);
    tipLabel->setWordWrap(true);
    tipLabel->setFont(tipFont);
    QList<QColor> colorList { QColor(0, 0, 0, static_cast<int>(255 * 0.5)),
                              QColor(192, 192, 192) };
    CooperationGuiHelper::instance()->autoUpdateTextColor(tipLabel, colorList);

    nameEdit = new CooperationLineEdit(q);
    nameEdit->installEventFilter(q);
    QRegExp regExp("^[a-zA-Z0-9\u4e00-\u9fa5-]+$");   // 正则表达式定义允许的字符范围
    QRegExpValidator *validator = new QRegExpValidator(regExp, nameEdit);
#ifdef linux
    nameEdit->lineEdit()->setValidator(validator);
#else
    nameEdit->setValidator(validator);
#endif
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
    deviceShareItem->setItemInfo(tr("Peripheral share"), devShareSwitchBtn);

    QLabel *tipLabel = new QLabel(tr("Allows peripherals that have been established "
                                     "to collaborate across devices to control this "
                                     "device, including keyboard, mouse, trackpad, etc"),
                                  q);
    auto margins = tipLabel->contentsMargins();
    margins.setLeft(10);
    tipLabel->setContentsMargins(margins);
    tipLabel->setWordWrap(true);
    tipLabel->setFont(tipFont);
    QList<QColor> colorList { QColor(0, 0, 0, static_cast<int>(255 * 0.5)),
                              QColor(192, 192, 192) };
    CooperationGuiHelper::instance()->autoUpdateTextColor(tipLabel, colorList);

    connectCB = new QComboBox(q);
    connectCB->setFixedWidth(280);
    for (const auto &info : connectComboBoxInfo) {
        connectCB->addItem(QIcon::fromTheme(info.first), info.second);
    }
    connect(connectCB, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingDialogPrivate::onConnectComboBoxValueChanged);
    SettingItem *connectItem = new SettingItem(q);
    connectItem->setItemInfo(tr("Connection direction"), connectCB);

    contentLayout->addWidget(deviceShareItem);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(tipLabel);
    contentLayout->addSpacing(16);
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
    tipLabel->setFont(tipFont);
    QList<QColor> colorList { QColor(0, 0, 0, static_cast<int>(255 * 0.5)),
                              QColor(192, 192, 192) };
    CooperationGuiHelper::instance()->autoUpdateTextColor(tipLabel, colorList);

    contentLayout->addWidget(clipShareItem);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(tipLabel);
}

void SettingDialogPrivate::onFindComboBoxValueChanged(int index)
{
#ifdef linux
    DConfigManager::instance()->setValue(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, index);
#else
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey, index);
#endif
}

void SettingDialogPrivate::onConnectComboBoxValueChanged(int index)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey, index);
}

void SettingDialogPrivate::onTransferComboBoxValueChanged(int index)
{
#ifdef linux
    DConfigManager::instance()->setValue(kDefaultCfgPath, DConfigKey::TransferModeKey, index);
#else
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey, index);
#endif
}

void SettingDialogPrivate::onNameEditingFinished()
{
    int length = nameEdit->text().length();
    if (length < 1 || length > 63) {
#ifdef linux
        nameEdit->setAlert(true);
        nameEdit->showAlertMessage(tr("The device name must contain 1 to 63 characters"));
        nameEdit->setFocus();
#endif
        return;
    }
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey, nameEdit->text());
}

void SettingDialogPrivate::onNameChanged(const QString &text)
{
#ifdef linux
    if (nameEdit->isAlert())
        nameEdit->setAlert(false);
#endif

    if (text.isEmpty())
        onNameEditingFinished();
}

void SettingDialogPrivate::onDeviceShareButtonClicked(bool clicked)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey, clicked);
}

void SettingDialogPrivate::onClipboardShareButtonClicked(bool clicked)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey, clicked);
}

void SettingDialogPrivate::onFileChoosed(const QString &path)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey, path);
}

void SettingDialogPrivate::initFont()
{
    groupFont = q->font();
    groupFont.setWeight(QFont::DemiBold);
    groupFont.setPixelSize(16);

    tipFont = q->font();
    tipFont.setWeight(QFont::Normal);
    tipFont.setPixelSize(12);
}

SettingDialog::SettingDialog(QWidget *parent)
    : CooperationAbstractDialog(parent),
      d(new SettingDialogPrivate(this))
{
    d->initWindow();
#ifdef linux
    d->initTitleBar();
#else
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
#endif
}

SettingDialog::~SettingDialog()
{
}

bool SettingDialog::eventFilter(QObject *watched, QEvent *event)
{
    // 绘制背景
    do {
        if (event->type() != QEvent::Paint)
            break;

        QWidget *widget = qobject_cast<QWidget *>(watched);
        if (!widget)
            break;

        QPainter painter(widget);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        if ((watched->objectName() == "ContentWidget" || watched->objectName() == "MainWidget")) {
            QColor color(255, 255, 255);
            if (CooperationGuiHelper::isDarkTheme())
                color.setRgb(41, 41, 41);
            painter.setBrush(color);

            if (watched->objectName() == "MainWidget")
                painter.drawRoundedRect(widget->rect(), 8, 8);
            else
                painter.drawRect(widget->rect());
            return true;
        } else if (watched->objectName() == "BackgroundWidget") {
            QColor color(245, 245, 245);
            if (CooperationGuiHelper::isDarkTheme())
                color.setRgb(36, 36, 36);
            painter.setBrush(color);
            painter.drawRect(widget->rect());
            return true;
        }
#ifdef linux
        // DLineEdit在DAbstractDialog中使用setAlert，绘制无效，所以自绘
        else if (d->nameEdit == watched && d->nameEdit->isAlert()) {
            painter.setBrush(QColor(241, 57, 50, qRound(0.15 * 255)));
            painter.drawRoundedRect(d->nameEdit->lineEdit()->rect(), 8, 8);
            return true;
        }
#endif
    } while (false);

    return CooperationAbstractDialog::eventFilter(watched, event);
}

void SettingDialog::showEvent(QShowEvent *event)
{
    loadConfig();
    CooperationAbstractDialog::showEvent(event);
}

void SettingDialog::loadConfig()
{
#ifdef linux
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 1) ? 1 : mode;
    d->findCB->setCurrentIndex(mode);
#else
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    d->findCB->setCurrentIndex(value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    d->nameEdit->setText(value.isValid()
                                 ? value.toString()
                                 : QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0)).dirName());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    d->devShareSwitchBtn->setChecked(value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    d->connectCB->setCurrentIndex(value.isValid() ? value.toInt() : 0);

#ifdef linux
    value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::TransferModeKey, 0);
    mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    d->transferCB->setCurrentIndex(mode);
#else
    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    d->transferCB->setCurrentIndex(value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    d->chooserEdit->setText(value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    d->clipShareSwitchBtn->setChecked(value.isValid() ? value.toBool() : true);
}
