// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "maincontroller/maincontroller.h"
#include "configs/settings/configmanager.h"
#include "maincontroller/maincontroller.h"

#include "common/constant.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/chan.h"
#include "ipc/bridge.h"

#include <CuteIPCInterface.h>

#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>

using namespace daemon_cooperation;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    ipcInterface = new CuteIPCInterface();

    backendOk = ipcInterface->connectToServer("cooperation-daemon");
    if (backendOk) {
        // bind SIGNAL to SLOT
        ipcInterface->remoteConnect(SIGNAL(ddeTransSignal(int, QString)), this, SLOT(backendMessageSlot(int, QString)));

        ipcInterface->call("bindSignal", Q_RETURN_ARG(QString, sessionId), Q_ARG(QString, PluginName), Q_ARG(QString, "ddeTransSignal"));

        DLOG << "ping return ID:" << sessionId.toStdString();
    } else {
        WLOG << "can not connect to: cooperation-daemon";
    }
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}


void CooperationUtilPrivate::backendMessageSlot(int type, const QString& msg)
{
    co::Json json_obj = json::parse(msg.toStdString());
    if (json_obj.is_null()) {
        WLOG << "parse error from: " << msg.toStdString();
        return;
    }

    switch (type) {
    case IPC_PING: {

    } break;
    case FRONT_TRANS_STATUS_CB: {
        ipc::GenericResult param;
        param.from_json(json_obj);
        QString msg(param.msg.c_str());   // job path

        q->metaObject()->invokeMethod(MainController::instance(), "onTransJobStatusChanged",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, param.id),
                                      Q_ARG(int, param.result),
                                      Q_ARG(QString, msg));
    } break;
    case FRONT_NOTIFY_FILE_STATUS: {
        q->metaObject()->invokeMethod(MainController::instance(),
                                      "onFileTransStatusChanged",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, msg));
    } break;
    case FRONT_APPLY_TRANS_FILE: {
        ApplyTransFiles transferInfo;
        transferInfo.from_json(json_obj);
        LOG << "apply transfer info: " << json_obj;

        switch (transferInfo.type) {
        case ApplyTransType::APPLY_TRANS_APPLY:
            q->metaObject()->invokeMethod(MainController::instance(),
                                          "waitForConfirm",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(transferInfo.machineName.c_str())));
            break;
        default:
            break;
        }
    } break;
    case FRONT_SERVER_ONLINE:
        MainController::instance()->regist();
        break;
    case FRONT_SEND_STATUS:
    {
        SendStatus param;
        param.from_json(json_obj);
        if (REMOTE_CLIENT_OFFLINE == param.status && param.curstatus == CURRENT_STATUS_TRAN_FILE_RCV) {
            q->metaObject()->invokeMethod(MainController::instance(),
                                          "onNetworkMiss",
                                          Qt::QueuedConnection);
        }
        break;
    }
    default:
        break;
    }
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

CuteIPCInterface *CooperationUtil::ipcInterface()
{
    if (!d->ipcInterface)
        d->ipcInterface = new CuteIPCInterface;

    return d->ipcInterface;
}

void CooperationUtil::registAppInfo(const QString &infoJson)
{
    //LOG << "regist app info: " << infoJson.toStdString();
    if (!d->backendOk) {
        LOG << "The ping backend is false";
        return;
    }

    ipcInterface()->callNoReply("registerDiscovery", Q_ARG(bool, false), Q_ARG(QString, PluginName), Q_ARG(QString, infoJson));
}

void CooperationUtil::unregistAppInfo()
{
    if (!d->backendOk) {
        LOG << "The ping backend is false";
        return;
    }

    ipcInterface()->call("registerDiscovery", Q_ARG(bool, true), Q_ARG(QString, PluginName), Q_ARG(QString, ""));
}

void CooperationUtil::setAppConfig(const QString &key, const QString &value)
{
    if (!d->backendOk) {
        LOG << "The ping backend is false";
        return;
    }

    ipcInterface()->call("saveAppConfig", Q_ARG(QString, PluginName), Q_ARG(QString, key), Q_ARG(QString, value));
}

void CooperationUtil::replyTransRequest(int type)
{
    UNIGO([=] {
        // 获取设备名称
        auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
        QString deviceName = value.isValid()
                ? value.toString()
                : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1);

        ipcInterface()->call("doApplyTransfer", Q_ARG(QString, PluginName), Q_ARG(QString, PluginName),
                             Q_ARG(QString, deviceName), Q_ARG(int, type));
    });
}

void CooperationUtil::cancelTrans()
{
    UNIGO([this] {
        // TRANS_CANCEL 1008; coopertion jobid: 1000
        bool res =  false;
        ipcInterface()->call("doOperateJob", Q_RETURN_ARG(bool, res), Q_ARG(int, 1008), Q_ARG(int, 1000), Q_ARG(QString, PluginName));
        LOG << "cancelTransferJob result=" << res;
    });
}
