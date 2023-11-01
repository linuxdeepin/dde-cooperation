// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferplugin.h"
#include "global_defines.h"
#include "utils/transferhelper.h"
#include "base/baseutils.h"
#include "config/configmanager.h"

#include <co/co.h>

using ButtonStateCallback = std::function<bool(const QVariantMap &)>;
using ClickedCallback = std::function<void(const QVariantMap &)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

using namespace cooperation_transfer;

void TransferPlugin::initialize()
{
    if (qApp->property("onlyTransfer").toBool()) {
        auto appName = qApp->applicationName();
        qApp->setApplicationName("dde-cooperation");
        ConfigManager::instance();
        qApp->setApplicationName(appName);
    }

    flag::set_value("rpc_log", "false"); //rpc日志关闭

#ifdef QT_DEBUG
    flag::set_value("cout", "true"); //终端日志输出
#else
    fastring logdir = deepin_cross::BaseUtils::logDir().toStdString();
    qInfo() << "set logdir: " << logdir.c_str();
    flag::set_value("log_dir", logdir); //日志保存目录
#endif
}

bool TransferPlugin::start()
{
    ClickedCallback clickedCb = TransferHelper::buttonClicked;
    ButtonStateCallback visibleCb = TransferHelper::buttonVisible;
    ButtonStateCallback clickableCb = TransferHelper::buttonClickable;
    QVariantMap historyInfo { { "id", kHistoryButtonId },
                              { "description", QObject::tr("View transfer history") },
                              { "icon-name", "history" },
                              { "location", 2 },
                              { "button-style", 0 },
                              { "clicked-callback", QVariant::fromValue(clickedCb) },
                              { "visible-callback", QVariant::fromValue(visibleCb) },
                              { "clickable-callback", QVariant::fromValue(clickableCb) } };

    QVariantMap transferInfo { { "id", kTransferButtonId },
                               { "description", QObject::tr("Send files") },
                               { "icon-name", "send" },
                               { "location", 3 },
                               { "button-style", 1 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) },
                               { "clickable-callback", QVariant::fromValue(clickableCb) } };

    dpfSlotChannel->push("cooperation_core", "slot_Register_Operation", historyInfo);
    dpfSlotChannel->push("cooperation_core", "slot_Register_Operation", transferInfo);

    TransferHelper::instance()->init();

    return true;
}

void TransferPlugin::stop()
{
}
