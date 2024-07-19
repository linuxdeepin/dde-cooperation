// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationcoreplugin.h"
#include "utils/cooperationutil.h"
#include "discover/discovercontroller.h"
#include "net/networkutil.h"
#include "net/helper/transferhelper.h"
#include "net/helper/sharehelper.h"
#include "discover/deviceinfo.h"


#include "common/commonutils.h"
#include "configs/settings/configmanager.h"
#include "singleton/singleapplication.h"

#include <QCommandLineParser>
#include <QCommandLineOption>
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
    initialize();
}

CooperaionCorePlugin::~CooperaionCorePlugin()
{
}

void CooperaionCorePlugin::initialize()
{
    dMain = QSharedPointer<MainWindow>::create();

    onlyTransfer = qApp->property("onlyTransfer").toBool();
    if (onlyTransfer) {
        auto appName = qApp->applicationName();
        qApp->setApplicationName(MainAppName);
        ConfigManager::instance();
        qApp->setApplicationName(appName);
    }

#ifdef linux
    ReportLogManager::instance()->init();
#endif

    CooperationUtil::instance();

    CommonUitls::initLog();
    CommonUitls::loadTranslator();
}

bool CooperaionCorePlugin::isMinilize()
{
    QCommandLineParser parser;
    // 添加自定义选项和参数"-m"
    QCommandLineOption option("m", "Launch with minimize UI");
    parser.addOption(option);

    // 解析命令行参数
    const auto &args = qApp->arguments();
    if (args.size() != 2 || !args.contains("-m"))
        return false;

    parser.process(args);
    return parser.isSet(option);;
}

bool CooperaionCorePlugin::start()
{
    CooperationUtil::instance()->mainWindow(dMain);
    DiscoverController::instance();

    CooperationUtil::instance()->initNetworkListener();
    CooperationUtil::instance()->initHistory();
    TransferHelper::instance()->registBtn();
    if (onlyTransfer) {
        // transfer deepend cooperation, not need network & share. bind the sendfile command.
        connect(TransferHelper::instance(), &TransferHelper::deliverMessage, qApp, &SingleApplication::onDeliverMessage);
    } else {
        NetworkUtil::instance();
        ShareHelper::instance()->registConnectBtn();

#ifdef __linux__
        if (CommonUitls::isFirstStart()) {
            DFeatureDisplayDialog *dlg = qApp->featureDisplayDialog();
            auto btn = dlg->getButton(0);
            connect(btn, &QAbstractButton::clicked, qApp, &SingleApplication::helpActionTriggered);
            CooperationUtil::instance()->showFeatureDisplayDialog(dlg);
        }
#endif
    }

    if (isMinilize()) {
        dMain->minimizedAPP();
    } else {
        dMain->show();
    }

    return true;
}

void CooperaionCorePlugin::stop()
{
}

void CooperaionCorePlugin::handleForwardCommand(const QStringList &forward)
{
    if (forward.size() >= 3) {
        auto ip = forward.at(0);
        auto name = forward.at(1);
        auto files = forward.mid(2);
        qWarning() << "---------IP: " << ip;
        qWarning() << "---------name: " << name;
        qWarning() << "---------files: " << files;
        TransferHelper::instance()->sendFiles(ip, name, files);
    }
}
