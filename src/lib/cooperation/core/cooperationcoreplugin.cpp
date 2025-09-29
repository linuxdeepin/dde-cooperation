// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreplugin.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "discover/discovercontroller.h"
#include "net/networkutil.h"
#ifdef ENABLE_COMPAT
#include "net/transferwrapper.h"
#endif
#include "net/helper/transferhelper.h"
#include "net/helper/sharehelper.h"
#ifdef ENABLE_PHONE
#include "net/helper/phonehelper.h"
#endif
#include "discover/deviceinfo.h"

#include "common/commonutils.h"
#include "configs/settings/configmanager.h"
#include "singleton/singleapplication.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QStandardPaths>
#ifdef __linux__
#    include "base/reportlog/reportlogmanager.h"
#    include <DFeatureDisplayDialog>
#    include <QFile>
DWIDGET_USE_NAMESPACE
#endif

using namespace cooperation_core;
using namespace deepin_cross;

CooperaionCorePlugin::CooperaionCorePlugin(QObject *parent)
    : QObject(parent)
{
    DLOG << "CooperationCorePlugin constructor";
    initialize();
    DLOG << "CooperationCorePlugin initialized";
}

CooperaionCorePlugin::~CooperaionCorePlugin()
{
    DLOG << "CooperationCorePlugin destructor";
}

void CooperaionCorePlugin::initialize()
{
    DLOG << "Initializing cooperation core plugin";
    CommonUitls::initLog();
    CommonUitls::loadTranslator();
    DLOG << "Logging and translation initialized";

    dMain = QSharedPointer<MainWindow>::create();
    DLOG << "Main window created";

    onlyTransfer = qApp->property("onlyTransfer").toBool();
    DLOG << "Application mode - onlyTransfer:" << onlyTransfer;
    if (onlyTransfer) {
        auto appName = qApp->applicationName();
        qApp->setApplicationName(MainAppName);
        ConfigManager::instance();
        DLOG << "Config manager initialized for transfer-only mode";
        qApp->setApplicationName(appName);
    }

#ifdef linux
    ReportLogManager::instance()->init();
    DLOG << "Report log manager initialized";
#endif

    CooperationUtil::instance();
    DLOG << "Cooperation utility initialized";
}

bool CooperaionCorePlugin::isMinilize()
{
    DLOG << "Checking for minimize option";
    QCommandLineParser parser;
    // 添加自定义选项和参数"-m"
    QCommandLineOption option("m", "Launch with minimize UI");
    parser.addOption(option);

    // 解析命令行参数
    const auto &args = qApp->arguments();
    if (args.size() != 2 || !args.contains("-m")) {
        DLOG << "Arguments do not match minimize condition, not minimizing";
        return false;
    }

    parser.process(args);
    bool isSet = parser.isSet(option);
    DLOG << "Minimize option is set:" << isSet;
    return isSet;
}

#ifdef ENABLE_PHONE
void CooperaionCorePlugin::initMobileModule()
{
    DLOG << "Initializing mobile module";
    connect(PhoneHelper::instance(), &PhoneHelper::addMobileInfo, dMain.get(), &MainWindow::addMobileDevice);
    connect(PhoneHelper::instance(), &PhoneHelper::disconnectMobile, dMain.get(), &MainWindow::disconnectMobile);
    connect(PhoneHelper::instance(), &PhoneHelper::setQRCode, dMain.get(), &MainWindow::onSetQRCode);

    PhoneHelper::instance()->registConnectBtn(dMain.get());
    DLOG << "Mobile module initialized";
}
#endif

bool CooperaionCorePlugin::start()
{
    DLOG << "Starting cooperation core plugin";
    CooperationUtil::instance()->mainWindow(dMain);
    DLOG << "Main window set in cooperation utility";

    TransferHelper::instance()->registBtn();
    DLOG << "Transfer buttons registered";

    if (onlyTransfer) {
        DLOG << "Running in transfer-only mode";
        // transfer deepend cooperation, not need network & share. bind the sendfile command.
        connect(TransferHelper::instance(), &TransferHelper::deliverMessage, qApp, &SingleApplication::onDeliverMessage);
        DLOG << "Connected deliver message signal";
    } else {
        DLOG << "Running in full cooperation mode";
        DiscoverController::instance();
        NetworkUtil::instance();
        DLOG << "Discover controller and network util initialized";

#ifdef ENABLE_PHONE
        initMobileModule();
        DLOG << "Mobile module initialized";
#endif

        ShareHelper::instance()->registConnectBtn();
        DLOG << "Share connect buttons registered";

        // bind search&refresh click and other status
        connect(dMain.get(), &MainWindow::searchDevice, NetworkUtil::instance(), &NetworkUtil::trySearchDevice);
        connect(dMain.get(), &MainWindow::refreshDevices, DiscoverController::instance(), &DiscoverController::startDiscover);
        DLOG << "Connected search and refresh signals";

        // bind storage setting
        connect(CooperationUtil::instance(), &CooperationUtil::storageConfig, NetworkUtil::instance(), &NetworkUtil::updateStorageConfig);
        DLOG << "Connected storage config signal";

        // bind device change with UI
        connect(CooperationUtil::instance(), &CooperationUtil::onlineStateChanged, dMain.get(), &MainWindow::onlineStateChanged);
        connect(DiscoverController::instance(), &DiscoverController::deviceOnline, dMain.get(), &MainWindow::addDevice);
        connect(DiscoverController::instance(), &DiscoverController::deviceOffline, dMain.get(), &MainWindow::removeDevice);
        connect(DiscoverController::instance(), &DiscoverController::discoveryFinished, dMain.get(), &MainWindow::onDiscoveryFinished);
        DLOG << "Connected device state change signals";

        // bridge: update the connected history after discover finish.
        connect(DiscoverController::instance(), &DiscoverController::discoveryFinished, HistoryManager::instance(), &HistoryManager::refreshHistory);
        connect(HistoryManager::instance(), &HistoryManager::historyConnected, DiscoverController::instance(), &DiscoverController::updateHistoryDevices);
        DLOG << "Connected history update signals";

        DiscoverController::instance()->init();   // init zeroconf and regist
        DLOG << "Discover controller initialized";

        // start network status listen after all ready
        CooperationUtil::instance()->initNetworkListener();
        DLOG << "Network listener initialized";

#ifdef ENABLE_COMPAT
        // start local ipc listen for transfer app
        QString ipcName = CommonUitls::ipcServerName(qAppName());
        DLOG << "Listening on IPC:" << ipcName.toStdString();
        TransferWrapper::instance()->listen(ipcName);
        DLOG << "Transfer wrapper listening on IPC";
#endif

#ifdef __linux__
        if (CommonUitls::isFirstStart()) {
            DLOG << "First start detected, showing feature dialog";
            DFeatureDisplayDialog *dlg = qApp->featureDisplayDialog();
            auto btn = dlg->getButton(0);
            connect(btn, &QAbstractButton::clicked, qApp, &SingleApplication::helpActionTriggered);
            CooperationUtil::instance()->showFeatureDisplayDialog(dlg);
            DLOG << "Feature dialog shown for first start";
        }
#endif
    }

    if (isMinilize()) {
        DLOG << "Starting minimized";
        dMain->minimizedAPP();
    } else {
        DLOG << "Showing main window";
        dMain->show();
    }

    return true;
}

void CooperaionCorePlugin::stop()
{
    DLOG << "Stopping cooperation core plugin";
    NetworkUtil::instance()->stop();
#ifdef ENABLE_COMPAT
    TransferWrapper::instance()->close();
#endif
    DLOG << "Cleanup complete";
    CommonUitls::shutdownLog();
}
