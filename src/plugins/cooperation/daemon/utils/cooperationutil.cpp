// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "maincontroller/maincontroller.h"
#include "configs/settings/configmanager.h"
#include "maincontroller/maincontroller.h"

#include "common/constant.h"

#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>

using namespace daemon_cooperation;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    // FIXME: 启动不同服务， 根据localIPCStart里处理通信
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}

//void CooperationUtilPrivate::localIPCStart()
//{
//    if (frontendIpcSer) return;

//    frontendIpcSer = new FrontendService(this);

//    UNIGO([this]() {
//        while (!thisDestruct) {
//            BridgeJsonData bridge;
//            frontendIpcSer->bridgeChan()->operator>>(bridge);   //300ms超时
//            if (!frontendIpcSer->bridgeChan()->done()) {
//                // timeout, next read
//                continue;
//            }

//            co::Json json_obj = json::parse(bridge.json);
//            if (json_obj.is_null()) {
//                WLOG << "parse error from: " << bridge.json.c_str();
//                continue;
//            }

//            switch (bridge.type) {
//            case IPC_PING: {
//                ipc::PingFrontParam param;
//                param.from_json(json_obj);

//                bool result = false;
//                fastring my_ver(FRONTEND_PROTO_VERSION);
//                // test ping 服务测试用

//                if (my_ver.compare(param.version) == 0 && (param.session.compare(sessionId.toStdString()) == 0 || param.session.compare("backendServerOnline") == 0)) {
//                    result = true;
//                } else {
//                    WLOG << param.version.c_str() << " =version not match= " << my_ver.c_str();
//                }

//                BridgeJsonData res;
//                res.type = IPC_PING;
//                res.json = result ? param.session : "";   // 成功则返回session，否则为空

//                frontendIpcSer->bridgeResult()->operator<<(res);
//            } break;
//            case FRONT_TRANS_STATUS_CB: {
//                ipc::GenericResult param;
//                param.from_json(json_obj);
//                QString msg(param.msg.c_str());   // job path

//                q->metaObject()->invokeMethod(MainController::instance(), "onTransJobStatusChanged",
//                                              Qt::QueuedConnection,
//                                              Q_ARG(int, param.id),
//                                              Q_ARG(int, param.result),
//                                              Q_ARG(QString, msg));
//            } break;
//            case FRONT_NOTIFY_FILE_STATUS: {
//                QString objstr(bridge.json.c_str());
//                q->metaObject()->invokeMethod(MainController::instance(),
//                                              "onFileTransStatusChanged",
//                                              Qt::QueuedConnection,
//                                              Q_ARG(QString, objstr));
//            } break;
//            case FRONT_APPLY_TRANS_FILE: {
//                ApplyTransFiles transferInfo;
//                transferInfo.from_json(json_obj);
//                LOG << "apply transfer info: " << json_obj;

//                switch (transferInfo.type) {
//                case ApplyTransType::APPLY_TRANS_APPLY:
//                    q->metaObject()->invokeMethod(MainController::instance(),
//                                                  "waitForConfirm",
//                                                  Qt::QueuedConnection,
//                                                  Q_ARG(QString, QString(transferInfo.machineName.c_str())));
//                    break;
//                default:
//                    break;
//                }
//            } break;
//            case FRONT_SERVER_ONLINE:
//                pingBackend();
//                MainController::instance()->regist();
//                break;
//            case FRONT_SEND_STATUS:
//            {
//                SendStatus param;
//                param.from_json(json_obj);
//                if (REMOTE_CLIENT_OFFLINE == param.status && param.curstatus == CURRENT_STATUS_TRAN_FILE_RCV) {
//                    q->metaObject()->invokeMethod(MainController::instance(),
//                                                  "onNetworkMiss",
//                                                  Qt::QueuedConnection);
//                }
//                break;
//            }
//            default:
//                break;
//            }
//        }
//    });

//    // start ipc services
//    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
//    frontendimp->setInterface(frontendIpcSer);

//    rpc::Server().add_service(frontendimp).start("0.0.0.0", UNI_IPC_BACKEND_COOPER_PLUGIN_PORT, "/frontend", "", "");
//}

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

void CooperationUtil::registAppInfo(const QString &infoJson)
{
    //TODO: ?registerDiscovery
}

void CooperationUtil::unregistAppInfo()
{
    //TODO: ?unregisterDiscovery
}

void CooperationUtil::setAppConfig(const QString &key, const QString &value)
{
    //TODO: ?setAppConfig
}

void CooperationUtil::replyTransRequest(int type)
{
    //TODO: ?applyTransFiles
}

void CooperationUtil::cancelTrans()
{
    //TODO: ?cancelTransJob
}
