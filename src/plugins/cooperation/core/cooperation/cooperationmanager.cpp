// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationmanager.h"
#include "cooperationmanager_p.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "info/deviceinfo.h"

#include "config/configmanager.h"
#include "common/constant.h"
#include "common/commonstruct.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/backend.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDBusReply>

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr char NotifyServerName[] { "org.freedesktop.Notifications" };
inline constexpr char NotifyServerPath[] { "/org/freedesktop/Notifications" };
inline constexpr char NotifyServerIfce[] { "org.freedesktop.Notifications" };

inline constexpr char NotifyRejectAction[] { "reject" };
inline constexpr char NotifyAcceptAction[] { "accept" };

inline constexpr char ConnectButtonId[] { "connect-button" };
inline constexpr char DisconnectButtonId[] { "disconnect-button" };

using namespace cooperation_core;

CooperationManagerPrivate::CooperationManagerPrivate(CooperationManager *qq)
    : q(qq)
{
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &CooperationManagerPrivate::onAppAttributeChanged);
}

void CooperationManagerPrivate::backendShareEvent(req_type_t type, const DeviceInfoPointer devInfo, bool accepted)
{
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
    co::Json req, res;

    auto myselfInfo = DeviceInfo::fromVariantMap(CooperationUtil::deviceInfo());
    ShareEvents event;
    event.eventType = type;
    switch (type) {
    case BACK_SHARE_CONNECT: {
        ShareConnectApply conEvent;
        conEvent.appName = MainAppName;
        conEvent.tarAppname = MainAppName;
        conEvent.tarIp = devInfo->ipAddress().toStdString();

        QStringList dataInfo({ myselfInfo->deviceName(),
                               myselfInfo->ipAddress() });
        conEvent.data = dataInfo.join(',').toStdString();

        event.data = conEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_START: {
        if (!devInfo->peripheralShared())
            return;

        ShareServerConfig config;
        config.server_screen = myselfInfo->deviceName().toStdString();
        config.client_screen = devInfo->deviceName().toStdString();
        switch (myselfInfo->linkMode()) {
        case DeviceInfo::LinkMode::RightMode:
            config.screen_left = myselfInfo->deviceName().toStdString();
            config.screen_right = devInfo->deviceName().toStdString();
            break;
        case DeviceInfo::LinkMode::LeftMode:
            config.screen_left = devInfo->deviceName().toStdString();
            config.screen_right = myselfInfo->deviceName().toStdString();
            break;
        }
        config.clipboardSharing = devInfo->clipboardShared();

        ShareStart startEvent;
        startEvent.appName = MainAppName;
        startEvent.ip = devInfo->ipAddress().toStdString();
        startEvent.config = config;

        event.data = startEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_STOP: {
        ShareStop stopEvent;
        stopEvent.appName = MainAppName;
        stopEvent.tarAppname = MainAppName;

        event.data = stopEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_CONNECT_REPLY: {
        ShareConnectReply replyEvent;
        replyEvent.appName = MainAppName;
        replyEvent.tarAppname = MainAppName;
        replyEvent.reply = accepted ? 1 : 0;

        event.data = replyEvent.as_json().str();
        req = event.as_json();
    } break;
    default:
        break;
    }

    if (req.empty())
        return;

    req.add_member("api", "Backend.shareEvents");
    rpcClient.call(req, res);
    rpcClient.close();
}

CooperationTaskDialog *CooperationManagerPrivate::taskDialog()
{
    if (!ctDialog) {
        ctDialog = new CooperationTaskDialog(CooperationUtil::instance()->mainWindow());
    }

    return ctDialog;
}

uint CooperationManagerPrivate::notifyMessage(uint replacesId, const QString &body, const QStringList &actions, int expireTimeout)
{
    QDBusReply<uint> reply = notifyIfc->call(QString("Notify"),
                                             MainAppName,   // appname
                                             replacesId,
                                             QString("dde-cooperation"),   // icon
                                             tr("Cooperation"),   // title
                                             body, actions, QVariantMap(), expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
}

void CooperationManagerPrivate::onActionTriggered(uint replacesId, const QString &action)
{
    if (recvReplacesId != replacesId)
        return;

    if (action == NotifyRejectAction) {
        //        CooperationUtil::instance()->replyTransRequest(ApplyTransType::APPLY_TRANS_REFUSED);
    } else if (action == NotifyAcceptAction) {
        backendShareEvent(BACK_SHARE_CONNECT_REPLY, nullptr, true);
    }
}

void CooperationManagerPrivate::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{

}

CooperationManager::CooperationManager(QObject *parent)
    : QObject(parent),
      d(new CooperationManagerPrivate(this))
{
}

CooperationManager *CooperationManager::instance()
{
    static CooperationManager ins;
    return &ins;
}

void CooperationManager::regist()
{
    ClickedCallback clickedCb = CooperationManager::buttonClicked;
    ButtonStateCallback visibleCb = CooperationManager::buttonVisible;
    QVariantMap historyInfo { { "id", ConnectButtonId },
                              { "description", tr("connect") },
                              { "icon-name", "connect" },
                              { "location", 0 },
                              { "button-style", 0 },
                              { "clicked-callback", QVariant::fromValue(clickedCb) },
                              { "visible-callback", QVariant::fromValue(visibleCb) } };

    QVariantMap transferInfo { { "id", DisconnectButtonId },
                               { "description", tr("Disconnect") },
                               { "icon-name", "disconnect" },
                               { "location", 1 },
                               { "button-style", 0 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) } };

    CooperationUtil::instance()->registerDeviceOperation(historyInfo);
    CooperationUtil::instance()->registerDeviceOperation(transferInfo);
}

void CooperationManager::connectToDevice(const DeviceInfoPointer info)
{
    UNIGO([this, info] {
        d->backendShareEvent(BACK_SHARE_CONNECT, info);
    });

    d->tarDeviceInfo = info;
    d->isRecvMode = false;
    d->taskDialog()->switchWaitPage(info->deviceName());
    d->taskDialog()->show();
}

void CooperationManager::disconnectToDevice(const DeviceInfoPointer info)
{
    UNIGO([this, info] {
        d->backendShareEvent(BACK_SHARE_STOP, info);
    });
}

void CooperationManager::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    if (id == ConnectButtonId) {
        CooperationManager::instance()->connectToDevice(info);
        return;
    }

    if (id == DisconnectButtonId) {
        CooperationManager::instance()->disconnectToDevice(info);
        return;
    }
}

bool CooperationManager::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    if (qApp->property("onlyTransfer").toBool())
        return false;

    if (id == ConnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connectable)
        return true;

    if (id == DisconnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connected)
        return true;

    return false;
}

void CooperationManager::notifyConnectRequest(const QString &info)
{
    d->isRecvMode = true;
    d->recvReplacesId = 0;

    static QString body(tr("A cross-end collaboration request was received from \"%1\""));
    QStringList actions { NotifyRejectAction, tr("Reject"),
                          NotifyAcceptAction, tr("Accept") };

    auto infoList = info.split(',');
    if (infoList.isEmpty())
        return;

    d->recvReplacesId = d->notifyMessage(d->recvReplacesId, body.arg(infoList.first()), actions, 10 * 1000);
}

void CooperationManager::handleConnectResult(bool accepted)
{
    if (accepted) {
        UNIGO([this]{
            d->backendShareEvent(BACK_SHARE_START, d->tarDeviceInfo);
        });
    } else {
        static QString msg(tr("\"%1\" has rejected your request for collaboration"));
        d->taskDialog()->switchFailPage(d->tarDeviceInfo->deviceName(), msg, false);
        d->taskDialog()->show();
    }
}
