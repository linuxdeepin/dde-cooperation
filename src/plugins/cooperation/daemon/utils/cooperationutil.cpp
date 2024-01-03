// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "maincontroller/maincontroller.h"
#include "configs/settings/configmanager.h"
#include "maincontroller/maincontroller.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/backend.h"

#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>

using namespace daemon_cooperation;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    localIPCStart();

    UNIGO([this] {
        backendOk = pingBackend();
        LOG << "The result of ping backend is " << backendOk;
    });
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}

bool CooperationUtilPrivate::pingBackend()
{
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_PORT, false);
    co::Json req, res;

    ipc::PingBackParam backParam;
    backParam.who = PluginName;
    backParam.version = fastring(UNI_IPC_PROTO);
    backParam.cb_port = UNI_IPC_BACKEND_COOPER_PLUGIN_PORT;

    req = backParam.as_json();
    req.add_member("api", "Backend.ping");   //BackendImpl::ping

    rpcClient.call(req, res);
    rpcClient.close();
    sessionId = res.get("msg").as_string().c_str();   // save the return session.

    //CallResult
    return res.get("result").as_bool() && !sessionId.isEmpty();
}

void CooperationUtilPrivate::localIPCStart()
{
    if (frontendIpcSer) return;

    frontendIpcSer = new FrontendService(this);

    UNIGO([this]() {
        while (!thisDestruct) {
            BridgeJsonData bridge;
            frontendIpcSer->bridgeChan()->operator>>(bridge);   //300ms超时
            if (!frontendIpcSer->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                WLOG << "parse error from: " << bridge.json.c_str();
                continue;
            }

            switch (bridge.type) {
            case IPC_PING: {
                ipc::PingFrontParam param;
                param.from_json(json_obj);

                bool result = false;
                fastring my_ver(FRONTEND_PROTO_VERSION);
                // test ping 服务测试用

                if (my_ver.compare(param.version) == 0 && (param.session.compare(sessionId.toStdString()) == 0 || param.session.compare("backendServerOnline") == 0)) {
                    result = true;
                } else {
                    WLOG << param.version.c_str() << " =version not match= " << my_ver.c_str();
                }

                BridgeJsonData res;
                res.type = IPC_PING;
                res.json = result ? param.session : "";   // 成功则返回session，否则为空

                frontendIpcSer->bridgeResult()->operator<<(res);
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
                QString objstr(bridge.json.c_str());
                q->metaObject()->invokeMethod(MainController::instance(),
                                              "onFileTransStatusChanged",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, objstr));
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
                pingBackend();
                MainController::instance()->regist();
                break;
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(frontendIpcSer);

    rpc::Server().add_service(frontendimp).start("0.0.0.0", UNI_IPC_BACKEND_COOPER_PLUGIN_PORT, "/frontend", "", "");
}

CooperationUtil::CooperationUtil(QObject *parent)
    : QObject(parent),
      d(new CooperationUtilPrivate(this))
{
}

CooperationUtil::~CooperationUtil()
{
    d->thisDestruct = true;
}

CooperationUtil *CooperationUtil::instance()
{
    static CooperationUtil ins;
    return &ins;
}

void CooperationUtil::registAppInfo(const QString &infoJson)
{
    LOG << "regist app info: " << infoJson.toStdString();
    if (!d->backendOk) {
        LOG << "The ping backend is false";
        return;
    }

    UNIGO([infoJson] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_PORT, false);
        co::Json req, res;

        QString appName = PluginName;
        AppPeerInfo peerInfo;
        peerInfo.appname = appName.toStdString();
        peerInfo.json = infoJson.toStdString();

        req = peerInfo.as_json();
        req.add_member("api", "Backend.registerDiscovery");
        rpcClient.call(req, res);
        rpcClient.close();
    });
}

void CooperationUtil::unregistAppInfo()
{
    if (!d->backendOk) {
        LOG << "The ping backend is false";
        return;
    }

    UNIGO([] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_PORT, false);
        co::Json req, res;
        QString appName = PluginName;

        AppPeerInfo peerInfo;
        peerInfo.appname = appName.toStdString();

        req = peerInfo.as_json();
        req.add_member("api", "Backend.unregisterDiscovery");
        rpcClient.call(req, res);
        rpcClient.close();
    });
}

void CooperationUtil::setAppConfig(const QString &key, const QString &value)
{
    if (!d->backendOk) {
        LOG << "The ping backend is false";
        return;
    }

    UNIGO([=] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_PORT, false);
        co::Json req, res;

        req = {
            { "appname", PluginName },
            { "key", key.toStdString() },
            { "value", value.toStdString() }
        };
        req.add_member("api", "Backend.setAppConfig");

        rpcClient.call(req, res);
        rpcClient.close();
    });
}

void CooperationUtil::replyTransRequest(int type)
{
    UNIGO([=] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
        co::Json res;
        // 获取设备名称
        auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
        QString deviceName = value.isValid()
                ? value.toString()
                : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1);

        ApplyTransFiles transInfo;
        transInfo.appname = PluginName;
        transInfo.type = type;
        transInfo.machineName = deviceName.toStdString();

        co::Json req = transInfo.as_json();
        req.add_member("api", "Backend.applyTransFiles");
        rpcClient.call(req, res);
        rpcClient.close();
    });
}

void CooperationUtil::cancelTrans()
{
    UNIGO([this] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
        co::Json req, res;

        ipc::TransJobParam jobParam;
        jobParam.session = d->sessionId.toStdString();
        jobParam.job_id = 1000;
        jobParam.appname = PluginName;

        req = jobParam.as_json();
        req.add_member("api", "Backend.cancelTransJob");   //BackendImpl::cancelTransJob
        rpcClient.call(req, res);
        rpcClient.close();
        LOG << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
    });
}
