// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "gui/mainwindow.h"

#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"
#include "net/networkutil.h"

#include <QJsonDocument>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QTimer>

#include <mutex>

#ifdef linux
#    include <DFeatureDisplayDialog>
DWIDGET_USE_NAMESPACE
#endif
using namespace cooperation_core;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}

CooperationUtil::CooperationUtil(QObject *parent)
    : QObject(parent),
      d(new CooperationUtilPrivate(this))
{
    DLOG << "CooperationUtil constructor";
}

CooperationUtil::~CooperationUtil()
{
    DLOG << "CooperationUtil destructor";
}

CooperationUtil *CooperationUtil::instance()
{
    DLOG << "Getting CooperationUtil instance";
    static CooperationUtil ins;
    return &ins;
}

void CooperationUtil::mainWindow(QSharedPointer<MainWindow> window)
{
    DLOG << "Setting main window";
    d->window = window;
}

QWidget* CooperationUtil::mainWindowWidget()
{
    if (d->window) {
        DLOG << "Returning registered main window";
        return d->window->window();
    }
    DLOG << "Searching for main window in top level widgets";
    auto windowList = qApp->topLevelWidgets();
    for (auto w : windowList) {
        if (w->objectName() == "MainWindow") {
            DLOG << "Found main window by object name";
            return w->window();
        }
    }
    WLOG << "No main window found";
    return nullptr;
}

void CooperationUtil::activateWindow()
{
    if (d->window) {
        DLOG << "Activating main window";
        d->window->activateWindow();
    } else {
        WLOG << "No main window to activate";
    }
}

void CooperationUtil::registerDeviceOperation(const QVariantMap &map)
{
    DLOG << "Registering device operation";
    if (d->window) {
        DLOG << "Main window exists, registering operation";
        d->window->onRegistOperations(map);
    } else {
        WLOG << "No main window to register operation";
    }
}

void CooperationUtil::setStorageConfig(const QString &value)
{
    DLOG << "Emitting storage config:" << value.toStdString();
    emit storageConfig(value);
}

QVariantMap CooperationUtil::deviceInfo()
{
    DLOG << "Getting device info";
    QVariantMap info;
#ifdef linux
    DLOG << "Linux platform, getting discovery mode from DConfigManager";
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 1) ? 1 : mode;
    info.insert(AppSettings::DiscoveryModeKey, mode);
    DLOG << "Discovery mode:" << mode;
#else
    DLOG << "Non-Linux platform, getting discovery mode from ConfigManager";
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    info.insert(AppSettings::DiscoveryModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    info.insert(AppSettings::DeviceNameKey,
                value.isValid()
                        ? value.toString()
                        : QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0)).dirName());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    info.insert(AppSettings::PeripheralShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    info.insert(AppSettings::LinkDirectionKey, value.isValid() ? value.toInt() : 0);

#ifdef linux
    DLOG << "Linux platform, getting transfer mode from DConfigManager";
    value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::TransferModeKey, 0);
    mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    info.insert(AppSettings::TransferModeKey, mode);
    DLOG << "Transfer mode:" << mode;
#else
    DLOG << "Non-Linux platform, getting transfer mode from ConfigManager";
    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    info.insert(AppSettings::TransferModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);
    DLOG << "Storage path:" << storagePath.toStdString();
    
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setStorageConfig(storagePath); });

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled);
    info.insert(AppSettings::CooperationEnabled, true);

    value = deepin_cross::BaseUtils::osType();
    info.insert(AppSettings::OSType, value);

    info.insert(AppSettings::IPAddress, localIPAddress());

    DLOG << "Device info collected successfully";
    return info;
}

QString CooperationUtil::localIPAddress()
{
    QString ip = deepin_cross::CommonUitls::getFirstIp().data();
    DLOG << "Local IP address:" << ip.toStdString();
    return ip;
}

QString CooperationUtil::closeOption()
{
    QString option = ConfigManager::instance()->appAttribute(AppSettings::CacheGroup, AppSettings::CloseOptionKey).toString();
    return option;
}

void CooperationUtil::saveOption(bool exit)
{
    ConfigManager::instance()->setAppAttribute(AppSettings::CacheGroup, AppSettings::CloseOptionKey,
                                               exit ? "Exit" : "Minimise");
    DLOG << "Saved close option:" << (exit ? "Exit" : "Minimise");
}

void CooperationUtil::initNetworkListener()
{
    DLOG << "Initializing network listener";
    QTimer *networkMonitorTimer = new QTimer(this);
    networkMonitorTimer->setInterval(1000);
    connect(networkMonitorTimer, &QTimer::timeout, this, &CooperationUtil::checkNetworkState);
    networkMonitorTimer->start();
    emit onlineStateChanged(deepin_cross::CommonUitls::getFirstIp().c_str());
}

void CooperationUtil::checkNetworkState()
{
    DLOG << "Checking network state";
    // 网络状态检测
    bool isConnected = deepin_cross::CommonUitls::getFirstIp().size() > 0;

    if (isConnected != d->isOnline) {
        DLOG << "Network state changed from" << d->isOnline << "to" << isConnected;
        d->isOnline = isConnected;
        Q_EMIT onlineStateChanged(deepin_cross::CommonUitls::getFirstIp().c_str());
    }
}

void CooperationUtil::showFeatureDisplayDialog(QDialog *dlg1)
{
#ifdef linux
    DFeatureDisplayDialog *dlg = static_cast<DFeatureDisplayDialog *>(dlg1);
    DLOG << "Setting up feature display dialog";
    
    auto btn = dlg->getButton(0);
    btn->setText(tr("View Help Manual"));
    dlg->setTitle(tr("Welcome to dde-cooperation"));
    
    DLOG << "Adding feature items to dialog";
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_kvm.png"),
                                  tr("Keyboard and mouse sharing"), tr("When a connection is made between two devices, the initiator's keyboard and mouse can be used to control the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_clipboard.png"),
                                  tr("Clipboard sharing"), tr("Once a connection is made between two devices, the clipboard will be shared and can be copied on one device and pasted on the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_file.png"),
                                  tr("Delivery of documents"), tr("After connecting between two devices, you can initiate a file delivery to the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_more.png"),
                                  tr("Usage"), tr("For detailed instructions, please click on the Help Manual below"), dlg));
    dlg->show();
#else
    DLOG << "Feature display dialog not supported on this platform";
#endif
}
