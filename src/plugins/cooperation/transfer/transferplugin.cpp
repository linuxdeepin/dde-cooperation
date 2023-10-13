// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferplugin.h"
#include "global_defines.h"
#include "utils/transferhelper.h"

using ButtonStateCallback = std::function<bool(const QVariantMap &)>;
using ClickedCallback = std::function<void(const QVariantMap &)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

using namespace cooperation_transfer;

void TransferPlugin::initialize()
{
}

bool TransferPlugin::start()
{
    ClickedCallback clickedCb = TransferHelper::buttonClicked;
    ButtonStateCallback visibleCb = TransferHelper::buttonVisible;
    ButtonStateCallback clickableCb = TransferHelper::buttonClickable;
    QVariantMap historyInfo { { "id", kHistoryId },
                              { "description", QObject::tr("View transfer history") },
                              { "icon-name", "history" },
                              { "location", 2 },
                              { "button-style", 0 },
                              { "clicked-callback", QVariant::fromValue(clickedCb) },
                              { "visible-callback", QVariant::fromValue(visibleCb) },
                              { "clickable-callback", QVariant::fromValue(clickableCb) } };

    QVariantMap transferInfo { { "id", kTransferId },
                               { "description", QObject::tr("Send files") },
                               { "icon-name", "send" },
                               { "location", 3 },
                               { "button-style", 1 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) },
                               { "clickable-callback", QVariant::fromValue(clickableCb) } };

    dpfSlotChannel->push("cooperation_workspace", "slot_Register_Operation", historyInfo);
    dpfSlotChannel->push("cooperation_workspace", "slot_Register_Operation", transferInfo);

    TransferHelper::instance()->init();

    return true;
}

void TransferPlugin::stop()
{
}
