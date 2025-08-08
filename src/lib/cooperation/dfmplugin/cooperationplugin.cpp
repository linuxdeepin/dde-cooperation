// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationplugin.h"
#include "menu/cooperationmenuscene.h"
#include "utils/cooperationhelper.h"
#include "configs/settings/configmanager.h"

#include <dfm-base/settingdialog/settingjsongenerator.h>
#include <dfm-base/settingdialog/customsettingitemregister.h>

#include <QTranslator>

#include <reportlog/reportlogmanager.h>

#define COOPERATION_SETTING_GROUP "10_advance.03_cooperation"
inline constexpr char kCooperationSettingGroup[] { COOPERATION_SETTING_GROUP };
inline constexpr char kCooperationSettingTransfer[] { "00_file_transfer" };
inline constexpr char kParentScene[] { "ExtendMenu" };

using namespace dfmbase;
using namespace dfmplugin_cooperation;

void CooperationPlugin::initialize()
{
    qDebug() << "Initializing cooperation plugin";

    deepin_cross::ReportLogManager::instance()->init();
    auto translator = new QTranslator(this);
    translator->load(QLocale(), "cooperation-transfer", "_", "/usr/share/dde-file-manager/translations");
    QCoreApplication::installTranslator(translator);

    if (DPF_NAMESPACE::LifeCycle::isAllPluginsStarted()) {
        qDebug() << "All plugins started, binding menu scene";
        bindMenuScene();
    } else {
        qDebug() << "Plugins not all started, connecting to pluginsStarted signal";
        connect(dpfListener, &DPF_NAMESPACE::Listener::pluginsStarted, this, &CooperationPlugin::bindMenuScene, Qt::DirectConnection);
    }
}

bool CooperationPlugin::start()
{
    qDebug() << "Starting cooperation plugin";

    // 加载跨端配置
    auto appName = qApp->applicationName();
    qDebug() << "Current application name:" << appName;

    qApp->setApplicationName("dde-cooperation");
    ConfigManager::instance();
    qApp->setApplicationName(appName);

    // 添加文管设置
    if (appName == "dde-file-manager") {
        qDebug() << "Application is dde-file-manager, adding cooperation setting item";
        addCooperationSettingItem();
    } else {
        qDebug() << "Application is not dde-file-manager, skipping cooperation setting item addition";
    }

    qInfo() << "Cooperation plugin started successfully";
    return true;
}

void CooperationPlugin::addCooperationSettingItem()
{
    qDebug() << "Adding cooperation setting items";
    
    SettingJsonGenerator::instance()->addGroup(kCooperationSettingGroup, tr("File transfer"));

    CustomSettingItemRegister::instance()->registCustomSettingItemType("pushbutton", CooperationHelper::createSettingButton);
    QVariantMap config {
        { "key", kCooperationSettingTransfer },
        { "name", QObject::tr("File transfer settings") },
        { "type", "pushbutton" },
        { "default", QObject::tr("Settings", "button") }
    };

    QString key = QString("%1.%2").arg(kCooperationSettingGroup, kCooperationSettingTransfer);
    SettingJsonGenerator::instance()->addConfig(key, config);
}

void CooperationPlugin::bindMenuScene()
{
    qDebug() << "Binding cooperation menu scene";
    
    dpfSlotChannel->push("dfmplugin_menu", "slot_MenuScene_RegisterScene", CooperationMenuCreator::name(), new CooperationMenuCreator);

    bool ret = dpfSlotChannel->push("dfmplugin_menu", "slot_MenuScene_Contains", QString(kParentScene)).toBool();
    if (ret) {
        qDebug() << "Parent menu scene exists, binding immediately";
        dpfSlotChannel->push("dfmplugin_menu", "slot_MenuScene_Bind", CooperationMenuCreator::name(), QString(kParentScene));
    } else {
        qDebug() << "Parent menu scene not found, subscribing to scene added signal";
        dpfSignalDispatcher->subscribe("dfmplugin_menu", "signal_MenuScene_SceneAdded", this, &CooperationPlugin::onMenuSceneAdded);
    }
}

void CooperationPlugin::onMenuSceneAdded(const QString &scene)
{
    qDebug() << "Menu scene added event received:" << scene;

    if (scene == kParentScene) {
        qDebug() << "Parent menu scene added, binding cooperation menu";
        dpfSlotChannel->push("dfmplugin_menu", "slot_MenuScene_Bind", CooperationMenuCreator::name(), QString(kParentScene));
        dpfSignalDispatcher->unsubscribe("dfmplugin_menu", "signal_MenuScene_SceneAdded", this, &CooperationPlugin::onMenuSceneAdded);
    }
}
