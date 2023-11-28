// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sharemanager.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"

#include <QApplication>

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr char ConnectButtonId[] { "connect-button" };
inline constexpr char DisconnectButtonId[] { "disconnect-button" };

using namespace cooperation_core;

#ifdef linux
inline constexpr char Kconnect[] { "connect" };
inline constexpr char Kdisconnect[] { "disconnect" };
#else
inline constexpr char Kconnect[] { ":/icons/deepin/builtin/texts/connect_18px.svg" };
inline constexpr char Kdisconnect[] { ":/icons/deepin/builtin/texts/disconnect_18px.svg" };
#endif

ShareManager::ShareManager(QObject *parent)
    : QObject(parent)
{
}

ShareManager *ShareManager::instance()
{
    static ShareManager ins;
    return &ins;
}

void ShareManager::regist()
{
    ClickedCallback clickedCb = ShareManager::buttonClicked;
    ButtonStateCallback visibleCb = ShareManager::buttonVisible;
    QVariantMap historyInfo { { "id", ConnectButtonId },
                              { "description", tr("connect") },
                              { "icon-name", Kconnect },
                              { "location", 0 },
                              { "button-style", 0 },
                              { "clicked-callback", QVariant::fromValue(clickedCb) },
                              { "visible-callback", QVariant::fromValue(visibleCb) } };

    QVariantMap transferInfo { { "id", DisconnectButtonId },
                               { "description", tr("Disconnect") },
                               { "icon-name", Kdisconnect },
                               { "location", 1 },
                               { "button-style", 0 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) } };

    CooperationUtil::instance()->registerDeviceOperation(historyInfo);
    CooperationUtil::instance()->registerDeviceOperation(transferInfo);
}

void ShareManager::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
}

bool ShareManager::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    if (qApp->property("onlyTransfer").toBool())
        return false;

    if (id == ConnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connectable)
        return true;

    if (id == DisconnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connected)
        return true;

    return false;
}
