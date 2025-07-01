// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferplugin.h"

#include "common/commonutils.h"
#include "configs/settings/configmanager.h"
#include "gui/mainwindow.h"
#include "helper/transferhelper.h"
#include "utils/cooperationutil.h"

#include <QCommandLineParser>
#include <QCommandLineOption>

using namespace cooperation_transfer;
using namespace cooperation_core;
using namespace deepin_cross;

TransferPlugin::TransferPlugin(QObject *parent)
    : QObject(parent)
{
    DLOG << "TransferPlugin constructor entered";
    initialize();
    DLOG << "TransferPlugin initialized";
}

TransferPlugin::~TransferPlugin()
{
    DLOG << "TransferPlugin destructor entered";
}

void TransferPlugin::initialize()
{
    DLOG << "Initializing TransferPlugin...";
    dMain = QSharedPointer<cooperation_core::MainWindow>::create();
    DLOG << "MainWindow created";

    auto appName = qApp->applicationName();
    qApp->setApplicationName(MainAppName);
    DLOG << "Application name set to:" << MainAppName;

    ConfigManager::instance();
    DLOG << "Configuration manager initialized";

    CommonUitls::initLog();
    DLOG << "Log system initialized";

    CommonUitls::loadTranslator();
    DLOG << "Translations loaded";

    qApp->setApplicationName(appName);
    DLOG << "Application name restored to original";
}

bool TransferPlugin::start()
{
    DLOG << "Starting TransferPlugin...";
    
    TransferHelper::instance()->registBtn(dMain.get());
    DLOG << "Transfer buttons registered to MainWindow";

    // bind search&refresh click and other status
    connect(dMain.get(), &MainWindow::searchDevice, TransferHelper::instance(), &TransferHelper::search);
    connect(dMain.get(), &MainWindow::refreshDevices, TransferHelper::instance(), &TransferHelper::refresh);
    DLOG << "Search and refresh signals connected";

    connect(CooperationUtil::instance(), &CooperationUtil::onlineStateChanged, dMain.get(), &MainWindow::onlineStateChanged);
    connect(TransferHelper::instance(), &TransferHelper::onlineDevices, dMain.get(), &MainWindow::addDevice);
    connect(TransferHelper::instance(), &TransferHelper::offlineDevice, dMain.get(), &MainWindow::removeDevice);
    connect(TransferHelper::instance(), &TransferHelper::finishDiscovery, dMain.get(), &MainWindow::onDiscoveryFinished);
    DLOG << "All device status signals connected";

    // start network status listen after all ready
    CooperationUtil::instance()->initNetworkListener();
    DLOG << "Network listener initialized";

    dMain->show();
    DLOG << "MainWindow shown";

    return true;
}

void TransferPlugin::stop()
{
    DLOG << "TransferPlugin stopped";
}
