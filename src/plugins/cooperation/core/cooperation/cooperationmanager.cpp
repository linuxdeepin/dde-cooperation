﻿// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationmanager.h"
#include "cooperationmanager_p.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "info/deviceinfo.h"
#include "maincontroller/maincontroller.h"

#include "configs/settings/configmanager.h"
#include "common/constant.h"
#include "common/commonstruct.h"
#include "common/commonutils.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/backend.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#ifdef linux
#    include "base/reportlog/reportlogmanager.h"
#    include <QDBusReply>
#endif

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

#ifdef linux
inline constexpr char Kconnect[] { "connect" };
inline constexpr char Kdisconnect[] { "disconnect" };
#else
inline constexpr char Kconnect[] { ":/icons/deepin/builtin/texts/connect_18px.svg" };
inline constexpr char Kdisconnect[] { ":/icons/deepin/builtin/texts/disconnect_18px.svg" };
#endif

using namespace cooperation_core;
using namespace deepin_cross;

CooperationManagerPrivate::CooperationManagerPrivate(CooperationManager *qq)
    : q(qq)
{
#ifdef linux
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
#endif
    confirmTimer.setInterval(10 * 1000);
    confirmTimer.setSingleShot(true);
    connect(&confirmTimer, &QTimer::timeout, q, &CooperationManager::onVerifyTimeout);
    connect(qApp, &QApplication::aboutToQuit, this, &CooperationManagerPrivate::stopCooperation);
}

void CooperationManagerPrivate::backendShareEvent(req_type_t type, const DeviceInfoPointer devInfo, QVariant param)
{
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
    co::Json req, res;

    auto myselfInfo = DeviceInfo::fromVariantMap(CooperationUtil::deviceInfo());
    myselfInfo->setIpAddress(CooperationUtil::localIPAddress());
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
    case BACK_SHARE_DISCONNECT: {
        ShareDisConnect disConEvent;
        disConEvent.appName = MainAppName;
        disConEvent.tarAppname = MainAppName;
        disConEvent.msg = myselfInfo->deviceName().toStdString();

        event.data = disConEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_START: {
        if (!devInfo->peripheralShared())
            return;

        ShareServerConfig config;
        QString serverName = myselfInfo->ipAddress();
        QString clientName = devInfo->ipAddress();
        config.server_screen = serverName.toStdString();
        config.client_screen = clientName.toStdString();
        switch (myselfInfo->linkMode()) {
        case DeviceInfo::LinkMode::RightMode:
            config.screen_left = serverName.toStdString();
            config.screen_right = clientName.toStdString();
            break;
        case DeviceInfo::LinkMode::LeftMode:
            config.screen_left = clientName.toStdString();
            config.screen_right = serverName.toStdString();
            break;
        }
        config.clipboardSharing = devInfo->clipboardShared();

        ShareStart startEvent;
        startEvent.appName = MainAppName;
        startEvent.config = config;

        event.data = startEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_STOP: {
        ShareStop stopEvent;
        stopEvent.appName = MainAppName;
        stopEvent.tarAppname = MainAppName;
        stopEvent.flags = param.toInt();

        event.data = stopEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_CONNECT_REPLY: {
        ShareConnectReply replyEvent;
        replyEvent.appName = MainAppName;
        replyEvent.tarAppname = MainAppName;
        replyEvent.reply = param.toBool() ? SHARE_CONNECT_COMFIRM : SHARE_CONNECT_REFUSE;

        event.data = replyEvent.as_json().str();
        req = event.as_json();
    } break;
    case BACK_SHARE_DISAPPLY_CONNECT: {
        ShareConnectDisApply cancelEvent;
        cancelEvent.appName = MainAppName;
        cancelEvent.tarAppname = MainAppName;
        event.data = cancelEvent.as_json().str();
        req = event.as_json();
    } break;
    default:
        break;
    }

    if (req.empty())
        return;

    req.add_member("api", "Backend.shareEvents");
#if defined(WIN32)
    co::wait_group wg;
    wg.add(1);
    UNIGO([&rpcClient, &req, &res, wg]() {
        rpcClient.call(req, res);
        wg.done();
    });
    wg.wait();
#else
    rpcClient.call(req, res);
#endif
    rpcClient.close();
}

CooperationTaskDialog *CooperationManagerPrivate::taskDialog()
{
    if (!ctDialog) {
        ctDialog = new CooperationTaskDialog(CooperationUtil::instance()->mainWindow());
        ctDialog->setModal(true);
        connect(ctDialog, &CooperationTaskDialog::retryConnected, q, [this] { q->connectToDevice(targetDeviceInfo); });
        connect(ctDialog, &CooperationTaskDialog::rejectRequest, this, [this] { onActionTriggered(recvReplacesId, NotifyRejectAction); });
        connect(ctDialog, &CooperationTaskDialog::acceptRequest, this, [this] { onActionTriggered(recvReplacesId, NotifyAcceptAction); });
        connect(ctDialog, &CooperationTaskDialog::waitCanceled, this, &CooperationManagerPrivate::onCancelCooperApply);
    }

    return ctDialog;
}

uint CooperationManagerPrivate::notifyMessage(uint replacesId, const QString &body, const QStringList &actions, int expireTimeout)
{
#ifdef linux
    QDBusReply<uint> reply = notifyIfc->call(QString("Notify"),
                                             MainAppName,   // appname
                                             replacesId,
                                             QString("dde-cooperation"),   // icon
                                             tr("Cooperation"),   // title
                                             body, actions, QVariantMap(), expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
#else
    Q_UNUSED(replacesId)
    Q_UNUSED(actions)
    Q_UNUSED(expireTimeout)

    CooperationUtil::instance()->mainWindow()->activateWindow();
    taskDialog()->switchInfomationPage(tr("Cooperation"), body);
    taskDialog()->show();
    return 0;
#endif
}

void CooperationManagerPrivate::stopCooperation()
{
    if (targetDeviceInfo && targetDeviceInfo->connectStatus() == DeviceInfo::Connected) {
        backendShareEvent(BACK_SHARE_STOP, targetDeviceInfo, 0);
        backendShareEvent(BACK_SHARE_DISCONNECT);

#ifdef linux
        static QString body(tr("Coordination with \"%1\" has ended"));
        notifyMessage(recvReplacesId, body.arg(CommonUitls::elidedText(targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)), {}, 3 * 1000);
#endif
    }
}

void CooperationManagerPrivate::onCancelCooperApply()
{
    confirmTimer.stop();
    backendShareEvent(BACK_SHARE_DISAPPLY_CONNECT);
    taskDialog()->hide();
}

void CooperationManagerPrivate::reportConnectionData()
{
#ifdef linux
    if (!targetDeviceInfo)
        return;

    auto osName = [](BaseUtils::OS_TYPE type) {
        switch (type) {
        case BaseUtils::kLinux:
            return "UOS";
        case BaseUtils::kWindows:
            return "Windows";
        default:
            break;
        }

        return "";
    };

    QString selfOsName = osName(BaseUtils::osType());
    QString targetOsName = osName(targetDeviceInfo->osType());
    if (selfOsName.isEmpty() || targetOsName.isEmpty())
        return;

    QVariantMap map;
    map.insert("osOfDevices", QString("%1&%2").arg(selfOsName, targetOsName));
    ReportLogManager::instance()->commit(ReportAttribute::ConnectionInfo, map);
#endif
}

void CooperationManagerPrivate::onActionTriggered(uint replacesId, const QString &action)
{
    if (recvReplacesId != replacesId || isTimeout)
        return;

    isReplied = true;
    if (action == NotifyRejectAction) {
        backendShareEvent(BACK_SHARE_CONNECT_REPLY, nullptr, false);
    } else if (action == NotifyAcceptAction) {
        backendShareEvent(BACK_SHARE_CONNECT_REPLY, nullptr, true);
        auto info = CooperationUtil::instance()->findDeviceInfo(senderDeviceIp);
        if (!info)
            return;

        // 更新设备列表中的状态
        targetDeviceInfo = DeviceInfoPointer::create(*info.data());
        targetDeviceInfo->setConnectStatus(DeviceInfo::Connected);
        MainController::instance()->updateDeviceState({ targetDeviceInfo });

        // 记录
        HistoryManager::instance()->writeIntoConnectHistory(info->ipAddress(), info->deviceName());

        static QString body(tr("Connection successful, coordinating with \"%1\""));
        notifyMessage(recvReplacesId, body.arg(CommonUitls::elidedText(info->deviceName(), Qt::ElideMiddle, 15)), {}, 3 * 1000);
    }
}

CooperationManager::CooperationManager(QObject *parent)
    : QObject(parent),
      d(new CooperationManagerPrivate(this))
{
}

CooperationManager::~CooperationManager()
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

void CooperationManager::connectToDevice(const DeviceInfoPointer info)
{
    if (d->targetDeviceInfo && d->targetDeviceInfo->connectStatus() == DeviceInfo::Connected) {
        static QString title(tr("Unable to collaborate to \"%1\""));
        d->taskDialog()->switchFailPage(title.arg(CommonUitls::elidedText(info->deviceName(), Qt::ElideMiddle, 15)),
                                        tr("You are connecting to another device"),
                                        false);
        d->taskDialog()->show();
        return;
    }

    d->backendShareEvent(BACK_SHARE_CONNECT, info);

    d->targetDeviceInfo = DeviceInfoPointer::create(*info.data());
    d->isRecvMode = false;
    d->isReplied = false;
    d->isTimeout = false;
    d->targetDevName = info->deviceName();

    d->confirmTimer.start();
    static QString title(tr("Requesting collaborate to \"%1\""));
    d->taskDialog()->switchWaitPage(title.arg(CommonUitls::elidedText(d->targetDevName, Qt::ElideMiddle, 15)));
    d->taskDialog()->show();
}

void CooperationManager::disconnectToDevice(const DeviceInfoPointer info)
{
    d->backendShareEvent(BACK_SHARE_STOP, info, 0);
    d->backendShareEvent(BACK_SHARE_DISCONNECT);

    if (d->targetDeviceInfo) {
        d->targetDeviceInfo->setConnectStatus(DeviceInfo::Connectable);
        MainController::instance()->updateDeviceState({ d->targetDeviceInfo });

        static QString body(tr("Coordination with \"%1\" has ended"));
        d->notifyMessage(d->recvReplacesId, body.arg(CommonUitls::elidedText(d->targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)), {}, 3 * 1000);
    }
}

void CooperationManager::checkAndProcessShare(const DeviceInfoPointer info)
{
    // 未协同、接收端，不进行处理
    if (d->isRecvMode || !d->targetDeviceInfo || d->targetDeviceInfo->connectStatus() != DeviceInfo::Connected)
        return;

    if (d->targetDeviceInfo->ipAddress() != info->ipAddress())
        return;

    if (d->targetDeviceInfo->peripheralShared() != info->peripheralShared()) {
        d->targetDeviceInfo = DeviceInfoPointer::create(*info.data());
        d->targetDeviceInfo->setConnectStatus(DeviceInfo::Connected);

        if (info->peripheralShared())
            d->backendShareEvent(BACK_SHARE_START, info);
        else
            d->backendShareEvent(BACK_SHARE_STOP, info, 1);
    } else if (d->targetDeviceInfo->clipboardShared() != info->clipboardShared()) {
        d->targetDeviceInfo = DeviceInfoPointer::create(*info.data());
        d->targetDeviceInfo->setConnectStatus(DeviceInfo::Connected);

        d->backendShareEvent(BACK_SHARE_START, info);
    }
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
    if (qApp->property("onlyTransfer").toBool() || !info->cooperationEnable())
        return false;

    if (id == ConnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connectable)
        return true;

    if (id == DisconnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connected)
        return true;

    return false;
}

void CooperationManager::notifyConnectRequest(const QString &info)
{
    d->isReplied = false;
    d->isTimeout = false;
    d->isRecvMode = true;
    d->recvReplacesId = 0;
    d->senderDeviceIp.clear();

    static QString body(tr("A cross-end collaboration request was received from \"%1\""));
    QStringList actions { NotifyRejectAction, tr("Reject"),
                          NotifyAcceptAction, tr("Accept") };

    auto infoList = info.split(',');
    if (infoList.size() < 2)
        return;

    d->senderDeviceIp = infoList[1];
    d->targetDevName = infoList[0];

    d->confirmTimer.start();
#ifdef linux
    d->recvReplacesId = d->notifyMessage(d->recvReplacesId, body.arg(CommonUitls::elidedText(d->targetDevName, Qt::ElideMiddle, 15)), actions, 10 * 1000);
#else
    CooperationUtil::instance()->mainWindow()->activateWindow();
    d->taskDialog()->switchConfirmPage(tr("Cooperation"), body.arg(CommonUitls::elidedText(d->targetDevName, Qt::ElideMiddle, 15)));
    d->taskDialog()->show();
#endif
}

void CooperationManager::handleConnectResult(int result)
{
    d->isReplied = true;
    if (!d->targetDeviceInfo)
        return;

    switch (result) {
    case SHARE_CONNECT_COMFIRM: {
        // 上报埋点数据
        d->reportConnectionData();

        d->targetDeviceInfo->setConnectStatus(DeviceInfo::Connected);
        MainController::instance()->updateDeviceState({ d->targetDeviceInfo });
        HistoryManager::instance()->writeIntoConnectHistory(d->targetDeviceInfo->ipAddress(), d->targetDeviceInfo->deviceName());

        d->backendShareEvent(BACK_SHARE_START, d->targetDeviceInfo);

        static QString body(tr("Connection successful, coordinating with  \"%1\""));
        d->notifyMessage(d->recvReplacesId, body.arg(CommonUitls::elidedText(d->targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)), {}, 3 * 1000);
        d->taskDialog()->close();
    } break;
    case SHARE_CONNECT_REFUSE: {
        static QString title(tr("Unable to collaborate to \"%1\""));
        static QString msg(tr("\"%1\" has rejected your request for collaboration"));
        d->taskDialog()->switchFailPage(title.arg(CommonUitls::elidedText(d->targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)),
                                        msg.arg(CommonUitls::elidedText(d->targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)),
                                        false);
        d->taskDialog()->show();
        d->targetDeviceInfo.reset();
    } break;
    case SHARE_CONNECT_ERR_CONNECTED: {
        static QString title(tr("Unable to collaborate to \"%1\""));
        static QString msg(tr("\"%1\" is connecting with other devices"));
        d->taskDialog()->switchFailPage(title.arg(CommonUitls::elidedText(d->targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)),
                                        msg.arg(CommonUitls::elidedText(d->targetDeviceInfo->deviceName(), Qt::ElideMiddle, 15)),
                                        false);
        d->taskDialog()->show();
        d->targetDeviceInfo.reset();
    } break;
    default:
        break;
    }
}

void CooperationManager::handleDisConnectResult(const QString &devName)
{
    if (!d->targetDeviceInfo)
        return;

    static QString body(tr("Coordination with \"%1\" has ended"));
    d->notifyMessage(d->recvReplacesId, body.arg(CommonUitls::elidedText(devName, Qt::ElideMiddle, 15)), {}, 3 * 1000);

    d->targetDeviceInfo->setConnectStatus(DeviceInfo::Connectable);
    MainController::instance()->updateDeviceState({ DeviceInfoPointer::create(*d->targetDeviceInfo.data()) });
    d->targetDeviceInfo.reset();
}

void CooperationManager::onVerifyTimeout()
{
    d->isTimeout = true;
    if (d->isRecvMode) {
        if (d->isReplied)
            return;

        static QString body(tr("The connection request sent to you by \"%1\" was interrupted due to a timeout"));
        d->notifyMessage(d->recvReplacesId, body.arg(CommonUitls::elidedText(d->targetDevName, Qt::ElideMiddle, 15)), {}, 3 * 1000);
    } else {
        if (!d->taskDialog()->isVisible() || d->isReplied)
            return;

        static QString title(tr("Unable to collaborate to \"%1\""));
        d->taskDialog()->switchFailPage(title.arg(CommonUitls::elidedText(d->targetDevName, Qt::ElideMiddle, 15)),
                                        tr("The other party does not confirm, please try again later"),
                                        true);
    }
}

void CooperationManager::handleCancelCooperApply()
{
    d->confirmTimer.stop();
    if (d->isRecvMode) {
        if (d->isReplied)
            return;
        static QString body(tr("The other party has cancelled the connection request !"));
#ifdef linux
        d->notifyMessage(d->recvReplacesId, body, {}, 3 * 1000);
#else
        static QString title(tr("connect failed"));
        d->taskDialog()->switchInfomationPage(title, body);
        d->taskDialog()->show();
#endif
    }
}

void CooperationManager::handleNetworkDismiss(const QString &msg)
{
    if (!msg.contains("\"errorType\":-1")) {
        static QString body(tr("Network not connected, file delivery failed this time.\
                               Please connect to the network and try again!"));
        d->notifyMessage(d->recvReplacesId, body, {}, 5 * 1000);
    } else {
        if (!d->taskDialog()->isVisible())
            return;

        static QString title(tr("File transfer failed"));
        d->taskDialog()->switchFailPage(title,
                                        tr("Network not connected, file delivery failed this time.\
                                           Please connect to the network and try again!"),
                                        true);
    }
}

void CooperationManager::handleSearchDeviceResult(bool res)
{
    if(!res)
        emit MainController::instance()->discoveryFinished(false);
}
