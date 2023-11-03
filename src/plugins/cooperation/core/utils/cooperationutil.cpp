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

#include <QJsonDocument>

using namespace cooperation_core;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    localIPCStart();
    rpcClient = std::shared_ptr<rpc::Client>(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false));

    UNIGO([this] {
        backendOk = pingBackend();
        qInfo() << "The result of ping backend is " << backendOk;
    });
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}

bool CooperationUtilPrivate::pingBackend()
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

                co::Json obj = json::parse(param.msg);
                NodeInfo nodeInfo;
                nodeInfo.from_json(obj);
                for (const auto &appInfo : nodeInfo.apps) {
                    // 上线，非跨端应用无需处理
                    if (param.result && appInfo.appname.compare(kMainAppName) != 0)
                        continue;

                    // 下线，跨端应用未下线
                    if (!param.result && appInfo.appname.compare(kMainAppName) == 0)
                        continue;

                    q->metaObject()->invokeMethod(MainController::instance(),
                                                  "updateDeviceList",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QString, QString(nodeInfo.os.ipv4.c_str())),
                                                  Q_ARG(QString, QString(appInfo.json.c_str())),
                                                  Q_ARG(bool, param.result));
                }
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

QList<DeviceInfo> CooperationUtilPrivate::parseDeviceInfo(const co::Json &obj)
{
    NodeList nodeList;
    nodeList.from_json(obj);

    QList<DeviceInfo> devInfoList;
    for (const auto &node : nodeList.peers) {
        DeviceInfo devInfo;
        devInfo.state = ConnectState::kConnectable;
        devInfo.ipStr = node.os.ipv4.c_str();

        for (const auto &app : node.apps) {
            if (app.appname != kMainAppName)
                continue;

            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(app.json.c_str(), &error);
            if (error.error != QJsonParseError::NoError)
                continue;

            auto map = doc.toVariant().toMap();
            if (!map.contains(AppSettings::kDeviceNameKey))
                continue;

            devInfo.deviceName = map.value(AppSettings::kDeviceNameKey).toString();
            break;
        }

        if (!devInfo.deviceName.isEmpty())
            devInfoList << devInfo;
    }

    return devInfoList;
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

    UNIGO([this, infoJson] {
        co::Json req, res;

        QString appName = qApp->applicationName();
        AppPeerInfo peerInfo;
        peerInfo.appname = appName.toStdString();
        peerInfo.json = infoJson.toStdString();

        req = peerInfo.as_json();
        req.add_member("api", "Backend.registerDiscovery");
        d->rpcClient->call(req, res);
    });
}

void CooperationUtil::unregistAppInfo()
{
    if (!d->backendOk)
        return;

    UNIGO([this] {
        co::Json req, res;
        QString appName = qApp->applicationName();

        AppPeerInfo peerInfo;
        peerInfo.appname = appName.toStdString();

        req = peerInfo.as_json();
        req.add_member("api", "Backend.unregisterDiscovery");
        d->rpcClient->call(req, res);
    });
}

void CooperationUtil::asyncDiscoveryDevice()
{
    if (!d->backendOk)
        return;

    UNIGO([this] {
        co::Json req, res;

        req.add_member("api", "Backend.getDiscovery");
        d->rpcClient->call(req, res);
        d->rpcClient->close();

        bool ok = res.get("result").as_bool();
        if (!ok) {
            qWarning() << "discovery devices failed!";
        } else {
            qInfo() << "all device: " << res.get("msg").as_c_str();
            co::Json obj;
            obj.parse_from(res.get("msg").as_string());
            QList<DeviceInfo> infoList = d->parseDeviceInfo(obj);
            Q_EMIT discoveryFinished(infoList);
        }
    });
}
