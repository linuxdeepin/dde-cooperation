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
#include "discover/discovercontroller.h"
#include "net/networkutil.h"
#include "historymanager.h"

#include <QJsonDocument>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QVBoxLayout>

#include <mutex>

#ifdef linux
#    include <DFeatureDisplayDialog>
DWIDGET_USE_NAMESPACE
#endif
using namespace cooperation_core;

#ifdef __linux__
const char *Kicon = "dde-cooperation";
#else
const char *Kicon = ":/icons/deepin/builtin/icons/dde-cooperation_128px.svg";
#endif

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
}

CooperationUtil::~CooperationUtil()
{
}

CooperationUtil *CooperationUtil::instance()
{
    static CooperationUtil ins;
    return &ins;
}

QWidget *CooperationUtil::mainWindow()
{
    if (!d->window)
        d->window = new MainWindow;

    return d->window;
}

DeviceInfoPointer CooperationUtil::findDeviceInfo(const QString &ip)
{
    if (!d->window)
        return nullptr;

    return d->window->findDeviceInfo(ip);
}

void CooperationUtil::destroyMainWindow()
{
    if (d->window)
        delete d->window;
}

void CooperationUtil::registerDeviceOperation(const QVariantMap &map)
{
    d->window->onRegistOperations(map);
}

void CooperationUtil::setStorageConfig(const QString &value)
{
    NetworkUtil::instance()->updateStorageConfig(value);
}

QVariantMap CooperationUtil::deviceInfo()
{
    QVariantMap info;
#ifdef linux
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 1) ? 1 : mode;
    info.insert(AppSettings::DiscoveryModeKey, mode);
#else
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
    value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::TransferModeKey, 0);
    mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    info.insert(AppSettings::TransferModeKey, mode);
#else
    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    info.insert(AppSettings::TransferModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setStorageConfig(storagePath); });

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled);
    info.insert(AppSettings::CooperationEnabled, true);

    value = deepin_cross::BaseUtils::osType();
    info.insert(AppSettings::OSType, value);

    info.insert(AppSettings::IPAddress, localIPAddress());

    return info;
}

QString CooperationUtil::localIPAddress()
{
    QString ip;
    ip = deepin_cross::CommonUitls::getFirstIp().data();
    return ip;
}

void CooperationUtil::initNetworkListener()
{
    QTimer *networkMonitorTimer = new QTimer(this);
    networkMonitorTimer->setInterval(1000);
    connect(networkMonitorTimer, &QTimer::timeout, this, &CooperationUtil::checkNetworkState);
    networkMonitorTimer->start();
    emit onlineStateChanged(deepin_cross::CommonUitls::getFirstIp().c_str());
}

void CooperationUtil::initHistory()
{
    HistoryManager::instance()->refreshHistory();

    connect(DiscoverController::instance(), &DiscoverController::discoveryFinished, this, [](bool hasFound) {
        Q_UNUSED(hasFound);
        HistoryManager::instance()->refreshHistory();
    });
}

void CooperationUtil::checkNetworkState()
{
    // 网络状态检测
    bool isConnected = deepin_cross::CommonUitls::getFirstIp().size() > 0;

    if (isConnected != d->isOnline) {
        d->isOnline = isConnected;
        Q_EMIT onlineStateChanged(deepin_cross::CommonUitls::getFirstIp().c_str());
    }
}

void CooperationUtil::showFeatureDisplayDialog(QDialog *dlg1)
{
#ifdef linux
    DFeatureDisplayDialog *dlg = static_cast<DFeatureDisplayDialog *>(dlg1);
    auto btn = dlg->getButton(0);
    btn->setText(tr("View Help Manual"));
    dlg->setTitle(tr("Welcome to dde-cooperation"));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_kvm.png"),
                                  tr("Keyboard and mouse sharing"), tr("When a connection is made between two devices, the initiator's keyboard and mouse can be used to control the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_clipboard.png"),
                                  tr("Clipboard sharing"), tr("Once a connection is made between two devices, the clipboard will be shared and can be copied on one device and pasted on the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_file.png"),
                                  tr("Delivery of documents"), tr("After connecting between two devices, you can initiate a file delivery to the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_more.png"),
                                  tr("Usage"), tr("For detailed instructions, please click on the Help Manual below"), dlg));
    dlg->show();
#endif
}

void CooperationUtil::showCloseDialog()
{
    QString option = ConfigManager::instance()->appAttribute(AppSettings::CacheGroup, AppSettings::CloseOptionKey).toString();
    if (option == "Minimise") {
        minimizedAPP();
        return;
    }
    if (option == "Exit")
        QApplication::quit();

    CooperationDialog dlg;

    QVBoxLayout *layout = new QVBoxLayout();
    QCheckBox *op1 = new QCheckBox(tr("Minimise to system tray"));
    op1->setChecked(true);
    QCheckBox *op2 = new QCheckBox(tr("Exit"));

    connect(op1, &QCheckBox::stateChanged, op1, [op2](int state) {
        op2->setChecked(state != Qt::Checked);
    });
    connect(op2, &QCheckBox::stateChanged, op2, [op1](int state) {
        op1->setChecked(state != Qt::Checked);
    });

    QCheckBox *op3 = new QCheckBox(tr("No more enquiries"));

    layout->addWidget(op1);
    layout->addWidget(op2);
    layout->addWidget(op3);

#ifdef __linux__
    dlg.setIcon(QIcon::fromTheme("dde-cooperation"));
    dlg.addButton(tr("Cancel"));
    dlg.addButton(tr("Confirm"), true, DDialog::ButtonWarning);
    dlg.setTitle(tr("Please select your operation"));
    QWidget *content = new QWidget();

    content->setLayout(layout);
    dlg.addContent(content);
#else
    dlg.setWindowIcon(QIcon::fromTheme(Kicon));
    dlg.setWindowTitle(tr("Please select your operation"));
    QPushButton *okButton = new QPushButton(tr("Confirm"));
    QPushButton *cancelButton = new QPushButton(tr("Cancel"));

    QObject::connect(okButton, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dlg, &QDialog::reject);

    layout->addWidget(okButton);
    layout->addWidget(cancelButton);
    dlg.setLayout(layout);
    dlg.setFixedSize(400, 200);
#endif

    int code = dlg.exec();
    if (code == QDialog::Accepted) {
        bool isExit = op2->checkState() == Qt::Checked;

        if (op3->checkState() == Qt::Checked)
            ConfigManager::instance()->setAppAttribute(AppSettings::CacheGroup, AppSettings::CloseOptionKey,
                                                       isExit ? "Exit" : "Minimise");
        if (isExit)
            QApplication::quit();
        else
            minimizedAPP();
    }
}

void CooperationUtil::minimizedAPP()
{
    mainWindow()->hide();
    if (d->trayIcon)
        return;
    d->trayIcon = new QSystemTrayIcon(QIcon::fromTheme(Kicon), mainWindow());

    QMenu *trayMenu = new QMenu(mainWindow());
    QAction *restoreAction = trayMenu->addAction(tr("Restore"));
    QAction *quitAction = trayMenu->addAction(tr("Quit"));

    d->trayIcon->setContextMenu(trayMenu);
    d->trayIcon->show();

    QObject::connect(restoreAction, &QAction::triggered, mainWindow(), &QMainWindow::show);
    QObject::connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    QObject::connect(d->trayIcon, &QSystemTrayIcon::activated, mainWindow(), [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            mainWindow()->show();
    });
}
