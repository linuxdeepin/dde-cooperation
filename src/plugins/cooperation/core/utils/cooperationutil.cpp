// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "gui/mainwindow.h"
#include "maincontroller/maincontroller.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/comstruct.h"

using namespace cooperation_core;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    localIPCStart();
    rpcClient = std::shared_ptr<rpc::Client>(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false));

    go([this] {
        backendOk = pingBacked();
    });
}

bool CooperationUtilPrivate::pingBacked()
{
    co::Json req, res;
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    int port = onlyTransfer
            ? UNI_IPC_FRONTEND_TRANSFER_PORT
            : UNI_IPC_FRONTEND_COOPERATION_PORT;
    //PingBackParam
    req = {
        { "who", qApp->applicationName().toStdString() },
        { "version", UNI_IPC_PROTO },
        { "cb_port", port },
    };

    req.add_member("api", "Backend.ping");   //BackendImpl::ping

    rpcClient->call(req, res);
    sessionId = res.get("msg").as_string().c_str();   // save the return session.

    //CallResult
    return res.get("result").as_bool() && !sessionId.isEmpty();
}

void CooperationUtilPrivate::localIPCStart()
{
    if (frontendIpcSer) return;

    frontendIpcSer = new FrontendService(this);

    go([this]() {
        while (!thisDestruct) {
            BridgeJsonData bridge;
            frontendIpcSer->bridgeChan()->operator>>(bridge);   //300ms超时
            if (!frontendIpcSer->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                qWarning() << "parse error from: " << bridge.json.c_str();
                continue;
            }

            switch (bridge.type) {
            case PING: {
                ipc::PingFrontParam param;
                param.from_json(json_obj);

                bool result = false;
                fastring my_ver(FRONTEND_PROTO_VERSION);
                if (my_ver.compare(param.version) == 0 && param.session.compare(sessionId.toStdString()) == 0) {
                    result = true;
                } else {
                    qWarning() << param.version.c_str() << " =version not match= " << my_ver.c_str();
                }

                BridgeJsonData res;
                res.type = PING;
                res.json = result ? param.session : "";   // 成功则返回session，否则为空

                frontendIpcSer->bridgeResult()->operator<<(res);
            } break;
            case FRONT_PEER_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);

                qInfo() << param.result << " peer : " << param.msg.c_str();

                co::Json cj;
                if (!cj.parse_from(param.msg))
                    break;

                QString appInfo = cj.get("apps").as_c_str();
                if (appInfo.isEmpty())
                    break;

                auto osInfo = cj.get("os");
                QString ip = osInfo.get("ipv4").as_c_str();

                q->metaObject()->invokeMethod(MainController::instance(),
                                              "updateDeviceStatus",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, ip),
                                              Q_ARG(QString, appInfo),
                                              Q_ARG(bool, param.result));
            } break;
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(frontendIpcSer);

    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    rpc::Server().add_service(frontendimp).start("0.0.0.0", onlyTransfer ? UNI_IPC_FRONTEND_TRANSFER_PORT : UNI_IPC_FRONTEND_COOPERATION_PORT, "/frontend", "", "");
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

QWidget *CooperationUtil::mainWindow()
{
    if (!d->window)
        d->window = new MainWindow;

    return d->window;
}

void CooperationUtil::destroyMainWindow()
{
    if (d->window)
        delete d->window;
}

void CooperationUtil::registerDeviceOperation(const QVariantMap &map)
{
    d->window->onRegistOperations(map);
}

void CooperationUtil::registAppInfo(const QString &infoJson)
{
    if (!d->backendOk)
        return;

    go([this, infoJson] {
        co::Json req, res;

        QString appName = qApp->applicationName();
        req = {
            //AppPeerInfo
            { "appinfo",
              {
                      { "appname", appName.toStdString() },
                      { "json", infoJson.toStdString() }   //一定是一个自定义的json格式
              } }
        };

        req.add_member("api", "Backend.registerDiscovery");
        d->rpcClient->call(req, res);
    });
}

void CooperationUtil::unregistAppInfo()
{
    if (!d->backendOk)
        return;

    go([this] {
        co::Json req, res;
        QString appName = qApp->applicationName();

        req = {
            //AppPeerInfo
            { "appinfo",
              { { "appname", appName.toStdString() },
                { "json", "" } } }
        };

        req.add_member("api", "Backend.unregisterDiscovery");
        d->rpcClient->call(req, res);
    });
}

QString CooperationUtil::onlineDeviceInfo()
{
    if (!d->backendOk)
        return {};

    co::wait_group g_wg;
    g_wg.add(1);
    QString info;

    go([this, &info, &g_wg] {
        co::Json req, res;

        req.add_member("api", "Backend.getDiscovery");
        d->rpcClient->call(req, res);
        d->rpcClient->close();

        bool ok = res.get("result").as_bool();
        if (!ok) {
            qWarning() << "discovery devices failed!";
        } else {
            info = res.get("msg").str().c_str();
        }

        g_wg.done();
    });
    g_wg.wait();

    return info;
}
