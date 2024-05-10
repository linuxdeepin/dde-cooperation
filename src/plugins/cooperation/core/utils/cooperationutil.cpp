// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "gui/mainwindow.h"
#include "maincontroller/maincontroller.h"
#include "transfer/transferhelper.h"
#include "cooperation/cooperationmanager.h"

#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"

#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"
#include "cooconstrants.h"

#include <QJsonDocument>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>

#define COO_CORE_SESSION_PORT 51566
#define COO_CORE_HARD_PIN "515616"

#ifdef linux
#    include <DFeatureDisplayDialog>
DWIDGET_USE_NAMESPACE
#endif
using namespace cooperation_core;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    // FIXME: 启动不同服务， 根据localIPCStart里处理通信
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    LOG << "This is only transfer?" << onlyTransfer;

    ExtenMessageHandler msg_cb([this](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
        // trans_req; share_req; rejected_req; share_interrupt;
        switch (mask)
        {
        case APPLY_INFO: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;

            // response my device info.
            res.nick = q->deviceInfoStr().toStdString();
            *res_msg = res.as_json().serialize();

            // update this device info to discovery list
            auto devInfo = q->parseDeviceInfo(QString(req.nick.c_str()));
            if (devInfo && devInfo->isValid() && devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
                QList<DeviceInfoPointer> devInfoList;
                devInfoList << devInfo;
                Q_EMIT q->discoveryFinished(devInfoList);
            }
        }
        return true;
        case APPLY_TRANS: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, req.flag),
                                          Q_ARG(QString, QString(req.host.c_str())),
                                          Q_ARG(bool, true));
        }
        return true;
        case APPLY_TRANS_RESULT: {
            ApplyMessage req, res;
            req.from_json(json_value);
            bool agree = (req.flag == REPLY_ACCEPT);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(TransferHelper::instance(),
                                          agree ? "accepted" : "rejected",
                                          Qt::QueuedConnection);
        }
        return true;
        case APPLY_SHARE: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "notifyConnectRequest",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.host.c_str())));
        }
        return true;
        case APPLY_SHARE_RESULT: {
            ApplyMessage req, res;
            req.from_json(json_value);
            bool agree = (req.flag == REPLY_ACCEPT);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, agree ? 1 : 0));
        }
        return true;
        case APPLY_SHARE_STOP: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleDisConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.host.c_str())));
        }
        return true;
        case APPLY_CANCELED: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleCancelCooperApply",
                                          Qt::QueuedConnection);
        }
        return true;
        }

        // unhandle message
        return false;
    });

    sessionManager = new SessionManager(this);
    sessionManager->setSessionExtCallback(msg_cb);

    sessionManager->sessionListen(COO_CORE_SESSION_PORT);

    connect(sessionManager, &SessionManager::notifyConnection, this, &CooperationUtilPrivate::handleConnectStatus);
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}

void CooperationUtilPrivate::handleConnectStatus(int result, QString reason)
{
//    if (result == -2) {
//        // error hanppend
//        int code = reason.toInt();
//        if (code == 113) {
//            // host unreachable
//            q->metaObject()->invokeMethod(CooperationManager::instance(),
//                                                          "handleSearchDeviceResult",
//                                                          Qt::QueuedConnection,
//                                                          Q_ARG(bool, true));
//        }
//    } else if (result == 2) {
//        // connected
//        q->metaObject()->invokeMethod(CooperationManager::instance(),
//                                                      "handleSearchDeviceResult",
//                                                      Qt::QueuedConnection,
//                                                      Q_ARG(bool, true));
//    }

    //TODO: request target info for each found nodes. now temp for test.
    QList<DeviceInfoPointer> infoList;

    auto myselfInfo = DeviceInfo::fromVariantMap(q->deviceInfo());
    myselfInfo->setIpAddress(q->localIPAddress());

    if (myselfInfo && myselfInfo->isValid() && myselfInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone)
        infoList << myselfInfo;
    Q_EMIT q->discoveryFinished(infoList);
//    q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
//                                  Qt::QueuedConnection,
//                                  Q_ARG(int, 0),
//                                  Q_ARG(QString, reason),
//                                  Q_ARG(bool, result == 2));
}

#if 0
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
            DLOG_IF(FLG_log_detail) << "recv IPC:" << bridge.type << " " << bridge.json;

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
            case FRONT_PEER_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);

                LOG << param.result << " peer : " << param.msg.c_str();

                co::Json obj = json::parse(param.msg);
                NodeInfo nodeInfo;
                nodeInfo.from_json(obj);
                if (nodeInfo.apps.empty() && !param.result) {
                    q->metaObject()->invokeMethod(MainController::instance(),
                                                  "updateDeviceList",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QString, QString(nodeInfo.os.ipv4.c_str())),
                                                  Q_ARG(QString, QString("")),
                                                  Q_ARG(int, nodeInfo.os.os_type),
                                                  Q_ARG(QString, QString("")),
                                                  Q_ARG(bool, param.result));
                    break;
                }

                for (const auto &appInfo : nodeInfo.apps) {
                    // 上线，非跨端应用无需处理
                    if (param.result && appInfo.appname.compare(CooperRegisterName) != 0)
                        continue;

                    // 下线，跨端应用未下线
                    if (!param.result && appInfo.appname.compare(CooperRegisterName) == 0)
                        continue;

                    q->metaObject()->invokeMethod(MainController::instance(),
                                                  "updateDeviceList",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QString, QString(nodeInfo.os.ipv4.c_str())),
                                                  Q_ARG(QString, QString(nodeInfo.os.share_connect_ip.c_str())),
                                                  Q_ARG(int, nodeInfo.os.os_type),
                                                  Q_ARG(QString, QString(appInfo.json.c_str())),
                                                  Q_ARG(bool, param.result));
                }
            } break;
            case FRONT_CONNECT_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString msg(param.msg.c_str());

                q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                                              Qt::QueuedConnection,
                                              Q_ARG(int, param.result),
                                              Q_ARG(QString, msg),
                                              Q_ARG(bool, param.isself));
            } break;
            case FRONT_TRANS_STATUS_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString msg(param.msg.c_str());   // job path

                q->metaObject()->invokeMethod(TransferHelper::instance(), "onTransJobStatusChanged",
                                              Qt::QueuedConnection,
                                              Q_ARG(int, param.id),
                                              Q_ARG(int, param.result),
                                              Q_ARG(QString, msg));
            } break;
            case FRONT_NOTIFY_FILE_STATUS: {
                QString objstr(bridge.json.c_str());
                q->metaObject()->invokeMethod(TransferHelper::instance(),
                                              "onFileTransStatusChanged",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, objstr));
            } break;
            case FRONT_APPLY_TRANS_FILE: {
                ApplyTransFiles transferInfo;
                transferInfo.from_json(json_obj);
                LOG << "apply transfer info: " << json_obj;

                switch (transferInfo.type) {
                case ApplyTransType::APPLY_TRANS_CONFIRM:
                    q->metaObject()->invokeMethod(TransferHelper::instance(),
                                                  "accepted",
                                                  Qt::QueuedConnection);
                    break;
                case ApplyTransType::APPLY_TRANS_REFUSED:
                    q->metaObject()->invokeMethod(TransferHelper::instance(),
                                                  "rejected",
                                                  Qt::QueuedConnection);
                    break;
                default:
                    break;
                }
            } break;
            case FRONT_SERVER_ONLINE:
                backendOk = pingBackend();
                q->metaObject()->invokeMethod(MainController::instance(),
                                              "start",
                                              Qt::QueuedConnection);
                break;
            case FRONT_SHARE_APPLY_CONNECT: {
                ShareConnectApply conApply;
                conApply.from_json(json_obj);
                q->metaObject()->invokeMethod(CooperationManager::instance(),
                                              "notifyConnectRequest",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, QString(conApply.data.c_str())));
            } break;
            case FRONT_SHARE_APPLY_CONNECT_REPLY: {
                ShareConnectReply conReply;
                conReply.from_json(json_obj);

                LOG << "share apply connect info: " << json_obj;
                q->metaObject()->invokeMethod(CooperationManager::instance(),
                                              "handleConnectResult",
                                              Qt::QueuedConnection,
                                              Q_ARG(int, conReply.reply));
            } break;
            case FRONT_SHARE_DISCONNECT: {
                ShareDisConnect disCon;
                disCon.from_json(json_obj);

                LOG << "share disconnect info: " << json_obj;
                q->metaObject()->invokeMethod(CooperationManager::instance(),
                                              "handleDisConnectResult",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString, QString(disCon.msg.c_str())));
            } break;
            case FRONT_SHARE_DISAPPLY_CONNECT: {
                ShareConnectDisApply param;
                param.from_json(json_obj);
                LOG << "share cancel apply : " << json_obj;
                q->metaObject()->invokeMethod(CooperationManager::instance(),
                                              "handleCancelCooperApply",
                                              Qt::QueuedConnection);
            } break;
            case FRONT_SEND_STATUS: {
                SendStatus param;
                param.from_json(json_obj);
                LOG << " FRONT_SEND_STATUS  : " << json_obj;
                if ((param.curstatus == CURRENT_STATUS_TRAN_FILE_SEN || param.curstatus == CURRENT_STATUS_TRAN_FILE_RCV || param.msg.contains("\"protocolType\":1004"))) {
                    q->metaObject()->invokeMethod(CooperationManager::instance(),
                                                  "handleNetworkDismiss",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QString, QString(param.msg.c_str())));
                }
                break;
            }
            case FRONT_SEARCH_IP_DEVICE_RESULT: {
                SearchDeviceResult param;
                param.from_json(json_obj);
                LOG << "SearchDeviceResult : " << json_obj;
                q->metaObject()->invokeMethod(CooperationManager::instance(),
                                              "handleSearchDeviceResult",
                                              Qt::QueuedConnection,
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

QList<DeviceInfoPointer> CooperationUtilPrivate::parseDeviceInfo(const co::Json &obj)
{
    NodeList nodeList;
    nodeList.from_json(obj);

    auto lastInfo = nodeList.peers.pop_back();
    QList<DeviceInfoPointer> devInfoList;
    for (const auto &node : nodeList.peers) {
        DeviceInfoPointer devInfo { nullptr };
        for (const auto &app : node.apps) {
            if (app.appname != CooperRegisterName)
                continue;

            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(app.json.c_str(), &error);
            if (error.error != QJsonParseError::NoError)
                continue;

            auto map = doc.toVariant().toMap();
            if (!map.contains(AppSettings::DeviceNameKey))
                continue;

            map.insert("IPAddress", node.os.ipv4.c_str());
            map.insert("OSType", node.os.os_type);
            devInfo = DeviceInfo::fromVariantMap(map);
            if (lastInfo.os.share_connect_ip == node.os.ipv4)
                devInfo->setConnectStatus(DeviceInfo::Connected);
            else
                devInfo->setConnectStatus(DeviceInfo::Connectable);

            break;
        }

        if (devInfo && devInfo->isValid() && devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone)
            devInfoList << devInfo;
    }

    return devInfoList;
}
#endif


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

QWidget *CooperationUtil::mainWindow()
{
    if (!d->window)
        d->window = new MainWindow;

    return d->window;
}

DeviceInfoPointer CooperationUtil::findDeviceInfo(const QString &ip)
{
    if (!d->window)
        return nullptr;

    return d->window->findDeviceInfo(ip);
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

void CooperationUtil::pingTarget(const QString &ip)
{
    // session connect and then send rpc request
    bool pong = d->sessionManager->sessionPing(ip, COO_CORE_SESSION_PORT);
//    emit CooperationManager::instance()->handleSearchDeviceResult(pong);

    // send info apply, and sync handle
    ApplyMessage msg;
    msg.flag = ASK_QUIET;
    msg.host = ip.toStdString();
    msg.nick = deviceInfoStr().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    QString res = d->sessionManager->sendRpcRequest(APPLY_INFO, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send APPLY_TRANS failed.";
    } else {
        // update this device info to discovery list
        auto devInfo = parseDeviceInfo(res);
        if (devInfo && devInfo->isValid() && devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
            QList<DeviceInfoPointer> devInfoList;
            devInfoList << devInfo;
            Q_EMIT discoveryFinished(devInfoList);
        }
    }
}

void CooperationUtil::sendTransApply(const QString &ip)
{
    // session connect and then send rpc request
    bool logind = d->sessionManager->sessionConnect(ip, COO_CORE_SESSION_PORT, COO_CORE_HARD_PIN);
    if (logind) {
        // send transfer apply, and async handle in RPC recv
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        msg.host = ip.toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        QString res = d->sessionManager->sendRpcRequest(APPLY_TRANS, jsonMsg);
        if (res.isEmpty()) {
            // transfer request send exception, it perhaps network error
            WLOG << "Send APPLY_TRANS failed.";
        }
    }
}

void CooperationUtil::sendShareEvents(int type, const QString &jsonMsg)
{
    // session connect and then send rpc request
//    bool logind = d->sessionManager->sessionConnect(ip, COO_CORE_SESSION_PORT, COO_CORE_HARD_PIN);
//    if (logind) {
//        // send share apply
//    }
//    d->sessionManager->sendRpcRequest();
}

void CooperationUtil::registAppInfo(const QString &infoJson)
{
    LOG << "regist app info: " << infoJson.toStdString();
    //TODO: ?registerDiscovery
}

void CooperationUtil::unregistAppInfo()
{
    //TODO: ?unregisterDiscovery
}

void CooperationUtil::asyncDiscoveryDevice()
{
    //TODO: ?getDiscovery then
//    QList<DeviceInfoPointer> infoList;
//    infoList = d->parseDeviceInfo(obj);
//    Q_EMIT discoveryFinished(infoList);
}

void CooperationUtil::setAppConfig(const QString &key, const QString &value)
{
    //TODO: ?setAppConfig
}

QString CooperationUtil::deviceInfoStr()
{
    auto infomap = deviceInfo();
    infomap.insert("IPAddress", localIPAddress());

    // 将QVariantMap转换为QJsonDocument转换为QString
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(infomap);
    QString jsonString = jsonDocument.toJson(QJsonDocument::Compact);

    return jsonString;
}

DeviceInfoPointer CooperationUtil::parseDeviceInfo(const QString &info)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(info.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        ELOG << "parse device info error";
        return nullptr;
    }

    auto map = doc.toVariant().toMap();
//    map.insert("IPAddress", req.host.c_str());
//                map.insert("OSType", req.flag);
    auto devInfo = DeviceInfo::fromVariantMap(map);
    devInfo->setConnectStatus(DeviceInfo::Connectable);
//    if (lastInfo.os.share_connect_ip == node.os.ipv4)
//        devInfo->setConnectStatus(DeviceInfo::Connected);

    return devInfo;
}

QVariantMap CooperationUtil::deviceInfo()
{
    QVariantMap info;
#ifdef linux
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 1) ? 1 : mode;
    info.insert(AppSettings::DiscoveryModeKey, mode);
#else
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    info.insert(AppSettings::DiscoveryModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    info.insert(AppSettings::DeviceNameKey,
                value.isValid()
                ? value.toString()
                : QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0)).dirName());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    info.insert(AppSettings::PeripheralShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    info.insert(AppSettings::LinkDirectionKey, value.isValid() ? value.toInt() : 0);

#ifdef linux
    value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::TransferModeKey, 0);
    mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    info.insert(AppSettings::TransferModeKey, mode);
#else
    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    info.insert(AppSettings::TransferModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled);
    info.insert(AppSettings::CooperationEnabled, value.isValid() ? value.toBool() : false);

    value = deepin_cross::BaseUtils::osType();
    info.insert("osType", value);

    return info;
}

QString CooperationUtil::localIPAddress()
{
    QString ip;
    ip = deepin_cross::CommonUitls::getFirstIp().data();
    return ip;
}

void CooperationUtil::showFeatureDisplayDialog(QDialog *dlg1)
{
#ifdef linux
    DFeatureDisplayDialog *dlg = static_cast<DFeatureDisplayDialog *>(dlg1);
    auto btn = dlg->getButton(0);
    btn->setText(tr("View Help Manual"));
    dlg->setTitle(tr("Welcome to dde-cooperation"));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_kvm.png"),
                                  tr("Keyboard and mouse sharing"), tr("When a connection is made between two devices, the initiator's keyboard and mouse can be used to control the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_clipboard.png"),
                                  tr("Clipboard sharing"), tr("Once a connection is made between two devices, the clipboard will be shared and can be copied on one device and pasted on the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_file.png"),
                                  tr("Delivery of documents"), tr("After connecting between two devices, you can initiate a file delivery to the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_more.png"),
                                  tr("Usage"), tr("For detailed instructions, please click on the Help Manual below"), dlg));
    dlg->show();
#endif
}
