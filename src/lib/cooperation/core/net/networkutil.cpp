// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkutil.h"
#include "networkutill_p.h"

#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"
#include "common/commonutils.h"

#include "cooconstrants.h"
#include "discover/discovercontroller.h"
#include "helper/transferhelper.h"
#include "helper/sharehelper.h"
#include "share/sharecooperationservicemanager.h"
#ifdef ENABLE_PHONE
#include "helper/phonehelper.h"
#endif
#include "utils/cooperationutil.h"
#ifdef ENABLE_COMPAT
#include "compatwrapper.h"
#endif

#include <QJsonDocument>
#include <QDebug>
#include <QDir>
#include <QScreen>

using namespace cooperation_core;
NetworkUtilPrivate::NetworkUtilPrivate(NetworkUtil *qq)
    : q(qq)
{
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    LOG << "This is only transfer?" << onlyTransfer;

    sessionManager = new SessionManager(this);
    if (onlyTransfer) {
        DLOG << "Running in transfer-only mode, skipping full initialization";
        return;
    }
    servePort = COO_SESSION_PORT;
    DLOG << "Using session port:" << servePort;

    ExtenMessageHandler msg_cb([this](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
#ifdef QT_DEBUG
        DLOG << "NetworkUtil >> " << mask << " msg_cb, json_msg: " << json_value << std::endl;
#endif
        DLOG << "Received message with mask:" << mask;
        switch (mask) {
        case APPLY_INFO: {
            DLOG << "Handling APPLY_INFO message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            WLOG << "msg_cb: " << json_value;
            // response my device info.
            res.nick = q->deviceInfoStr().toStdString();
            *res_msg = res.as_json().serialize();

            // update this device info to discovery list
            q->metaObject()->invokeMethod(DiscoverController::instance(),
                                          "addSearchDeivce",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.nick.c_str())));
        }
            return true;
        case APPLY_TRANS: {
            DLOG << "Handling APPLY_TRANS message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            *res_msg = res.as_json().serialize();
            confirmTargetAddress = QString::fromStdString(req.host);

            q->metaObject()->invokeMethod(TransferHelper::instance(),
                                          "notifyTransferRequest",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.nick.c_str())),
                                          Q_ARG(QString, QString(req.host.c_str())));
        }
            return true;
        case APPLY_TRANS_RESULT: {
            DLOG << "Handling APPLY_TRANS_RESULT message";
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
            DLOG << "Handling APPLY_SHARE message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            QString info = QString::fromStdString(req.host + "," + req.nick + "," + req.fingerprint);
            *res_msg = res.as_json().serialize();
            confirmTargetAddress = QString::fromStdString(req.host);
            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "notifyConnectRequest",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, info));
        }
            return true;
        case APPLY_SHARE_RESULT: {
            DLOG << "Handling APPLY_SHARE_RESULT message";
            ApplyMessage req, res;
            req.from_json(json_value);
            bool agree = (req.flag == REPLY_ACCEPT);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();

            int resultCode;
            if (agree) {
                resultCode = SHARE_CONNECT_COMFIRM;
            } else if (req.nick == "BUSY_COOPERATING") {
                WLOG << "Device is busy, use ERR_CONNECTED to show appropriate message";
                // Device is busy, use ERR_CONNECTED to show appropriate message
                resultCode = SHARE_CONNECT_ERR_CONNECTED;
            } else {
                // Normal rejection
                resultCode = SHARE_CONNECT_REFUSE;
            }

            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "handleConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, resultCode),
                                          Q_ARG(QString, QString(req.fingerprint.c_str())));
        }
            return true;
        case APPLY_SHARE_STOP: {
            DLOG << "Handling APPLY_SHARE_STOP message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "handleDisConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.host.c_str())));
        }
            return true;
        case APPLY_CANCELED: {
            DLOG << "Handling APPLY_CANCELED message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            if (req.nick == "share") {
                DLOG << "Canceled type: share";
                q->metaObject()->invokeMethod(ShareHelper::instance(),
                                              "handleCancelCooperApply",
                                              Qt::QueuedConnection);
            } else if (req.nick == "transfer") {
                DLOG << "Canceled type: transfer";
                q->metaObject()->invokeMethod(TransferHelper::instance(),
                                              "handleCancelTransferApply",
                                              Qt::QueuedConnection);
            } else {
                DLOG << "Unknown canceled type:" << req.nick.c_str();
            }
        }
            return true;
#ifdef ENABLE_PHONE
        case APPLY_SCAN_CONNECT: {
            DLOG << "Handling APPLY_SCAN_CONNECT message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            res.nick = q->deviceInfoStr().toStdString();

            QString ipAddress = req.host.c_str();
            QString deviceName = req.nick.c_str();
            int width, height = 0;
            // take the name and resolution from the device name, like "xiam 12: 2400x1080"
            QStringList splitNick = deviceName.split("=");
            if (splitNick.size() == 2) {
                DLOG << "Device name contains resolution info";
                deviceName = splitNick[0].trimmed();
                QString resolution = splitNick[1].trimmed();
                QStringList resolutionParts = resolution.split("x");
                if (resolutionParts.size() == 2) {
                    DLOG << "Resolution parts found";
                    width = resolutionParts[0].toInt();  // Width
                    height = resolutionParts[1].toInt(); // Height
                } else {
                    DLOG << "Resolution parts not found";
                }
            } else {
                DLOG << "Device name does not contain resolution info";
            }
            *res_msg = res.as_json().serialize();

            DeviceInfoPointer info(new DeviceInfo(ipAddress, deviceName));
            info->setConnectStatus(DeviceInfo::ConnectStatus::Connected);
            info->setDeviceType(DeviceInfo::DeviceType::Mobile);
            q->metaObject()->invokeMethod(PhoneHelper::instance(), "onConnect",
                                          Qt::QueuedConnection,
                                          Q_ARG(DeviceInfoPointer, info),
                                          Q_ARG(int, width),
                                          Q_ARG(int, height));
        }
            return true;
        case APPLY_PROJECTION: {
            DLOG << "Handling APPLY_PROJECTION message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(PhoneHelper::instance(),
                                          "onScreenMirroring",
                                          Qt::QueuedConnection);
        }
            return true;
        case APPLY_PROJECTION_STOP: {
            DLOG << "Handling APPLY_PROJECTION_STOP message";
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(PhoneHelper::instance(), "onScreenMirroringStop",
                                          Qt::QueuedConnection);
        }
            return true;
#endif
        }

        // unhandle message
        DLOG << "Unhandled message mask:" << mask;
        return false;
    });

    sessionManager->setSessionExtCallback(msg_cb);
    sessionManager->updatePin(COO_HARD_PIN);
    sessionManager->sessionListen(servePort);

    connect(sessionManager, &SessionManager::notifyConnection, this, &NetworkUtilPrivate::handleConnectStatus);
    connect(sessionManager, &SessionManager::notifyTransChanged, this, &NetworkUtilPrivate::handleTransChanged);
    connect(sessionManager, &SessionManager::notifyAsyncRpcResult, this, &NetworkUtilPrivate::handleAsyncRpcResult);
}

NetworkUtilPrivate::~NetworkUtilPrivate()
{
    DLOG << "NetworkUtilPrivate destructor";
}

void NetworkUtilPrivate::handleConnectStatus(int result, QString reason)
{
    DLOG << " connect status: " << result << " " << reason.toStdString();
    if (result == 113 || result == 110) {
        DLOG << "Host unreachable or timeout, adding search device";
        // host unreachable or timeout
        q->metaObject()->invokeMethod(DiscoverController::instance(),
                                      "addSearchDeivce",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, ""));
        return;
    }
    if (result == EX_NETWORK_PINGOUT) {
        DLOG << "Network pingout exception";
        // ping <-> pong timeout, network connection exception
        // 1. update the discovery list.
        q->metaObject()->invokeMethod(DiscoverController::instance(),
                                      "compatRemoveDeivce",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, reason));

        // 2. show UI if there is any doing job
        q->metaObject()->invokeMethod(ShareHelper::instance(),
                                      "onShareExcepted",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, EX_NETWORK_PINGOUT),
                                      Q_ARG(QString, reason));
        // show UI for transfering
        q->metaObject()->invokeMethod(TransferHelper::instance(), "onTransferExcepted",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, EX_NETWORK_PINGOUT),
                                      Q_ARG(QString, reason));
    } else if (result == -2) {
        // error hanppend
        DLOG << "connect error, reason = " << reason.toStdString();
    } else if (result == -1) {
        DLOG << "Disconnected";
        // disconnected
        q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, 0),
                                      Q_ARG(QString, reason),
                                      Q_ARG(bool, false));
#ifdef ENABLE_PHONE
        DLOG << "Phone enabled, handling mobile disconnect";
        //mobile
        DeviceInfoPointer info(new DeviceInfo(reason, QString()));
        q->metaObject()->invokeMethod(PhoneHelper::instance(), "onDisconnect",
                                      Qt::QueuedConnection,
                                      Q_ARG(DeviceInfoPointer, info));
#endif
    } else {
        DLOG << "Unknown connection status result:" << result;
    }
}

void NetworkUtilPrivate::handleTransChanged(int status, const QString &path, quint64 size)
{
    q->metaObject()->invokeMethod(TransferHelper::instance(),
                                  "onTransChanged",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, status),
                                  Q_ARG(QString, path),
                                  Q_ARG(quint64, size));
}

void NetworkUtilPrivate::handleAsyncRpcResult(int32_t type, const QString response)
{
    picojson::value json_value;
    bool success = false;
    if (!response.isEmpty()) {
        std::string err = picojson::parse(json_value, response.toStdString());
        success = err.empty();
    }

    switch (type) {
    case APPLY_LOGIN: {
        DLOG << "Handling APPLY_LOGIN result";
        DLOG << "Login return: " << response.toStdString();
        if (success) {
            DLOG << "Login successful";
            LoginMessage login;
            login.from_json(json_value);

            QString ip = login.name.c_str();
            bool logined = !login.auth.empty();
            sessionManager->updateLoginStatus(ip, logined);
            // ApplyMessage reply;
            // reply.from_json(json_value);
            // next combi request
            q->doNextCombiRequest(ip);
        } else {
            DLOG << "Login failed, trying compat login";
            // try connect with old protocol
            q->compatLogin(response);
        }
        break;
    }
    case APPLY_INFO: {
        DLOG << "Handling APPLY_INFO result";
        DLOG << "Apply info return: " << response.toStdString();
        if (success) {
            DLOG << "Apply info successful";
            ApplyMessage reply;
            reply.from_json(json_value);
            // update this device info to discovery list
            q->metaObject()->invokeMethod(DiscoverController::instance(),
                                          "addSearchDeivce",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(reply.nick.c_str())));
        } else {
            DLOG << "Apply info failed";
        }
        break;
    }
    case APPLY_TRANS: {
        DLOG << "Handling APPLY_TRANS result";
        q->metaObject()->invokeMethod(TransferHelper::instance(),
                                      "onConnectStatusChanged",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, success ? 1 : 0),
                                      Q_ARG(QString, confirmTargetAddress),
                                      Q_ARG(bool, true));
        break;
    }
    case APPLY_SHARE: {
        DLOG << "Handling APPLY_SHARE result";
        if (!success) {
            DLOG << "Apply share failed";
            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "handleConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, SHARE_CONNECT_UNABLE));
        } else {
            DLOG << "Apply share successful";
        }
        break;
    }
    default:
        LOG << "unkown rpc callback type: " << type << " response:" << response.toStdString();
        DLOG << "Unknown RPC callback type:" << type;
        break;
    }
}

NetworkUtil::NetworkUtil(QObject *parent)
    : QObject(parent),
      d(new NetworkUtilPrivate(this))
{
    DLOG << "NetworkUtil constructor";
#ifdef ENABLE_COMPAT
    auto wrapper = CompatWrapper::instance();
    auto discover = DiscoverController::instance();
    connect(wrapper, &CompatWrapper::compatConnectResult, this, &NetworkUtil::handleCompatConnectResult, Qt::QueuedConnection);
    connect(discover, &DiscoverController::registCompatAppInfo, this, &NetworkUtil::handleCompatRegister, Qt::QueuedConnection);
    connect(discover, &DiscoverController::startDiscoveryDevice, this, &NetworkUtil::handleCompatDiscover, Qt::QueuedConnection);
    DLOG << "Compat mode connections established";
#endif
}

NetworkUtil::~NetworkUtil()
{
    DLOG << "NetworkUtil destructor";
}

NetworkUtil *NetworkUtil::instance()
{
    DLOG << "Getting NetworkUtil instance";
    static NetworkUtil ins;
    return &ins;
}

#ifdef ENABLE_COMPAT
void NetworkUtil::handleCompatConnectResult(int result, const QString &ip)
{
    DLOG << "Handling compat connect result:" << result << "IP:" << ip.toStdString();
    auto type = _nextCombi.first;
    if (type == APPLY_TRANS) {
        DLOG << "Next combi type is APPLY_TRANS";
        metaObject()->invokeMethod(TransferHelper::instance(),
                                   "onConnectStatusChanged",
                                   Qt::QueuedConnection,
                                   Q_ARG(int, result),
                                   Q_ARG(QString, ip),
                                   Q_ARG(bool, true));
    } else if (type == APPLY_SHARE) {
        DLOG << "Next combi type is APPLY_SHARE";
        if (result <= 0) {
            DLOG << "Result is not positive, handling connect result as unable";
            metaObject()->invokeMethod(ShareHelper::instance(),
                                       "handleConnectResult",
                                       Qt::QueuedConnection,
                                       Q_ARG(int, SHARE_CONNECT_UNABLE));
        }
    } else if (type == APPLY_INFO) {
        DLOG << "Next combi type is APPLY_INFO";
        if (result <= 0) {
            DLOG << "Result is not positive, adding search device as empty";
            metaObject()->invokeMethod(DiscoverController::instance(),
                                       "addSearchDeivce",
                                       Qt::QueuedConnection,
                                       Q_ARG(QString, ""));
        }
    } else {
        DLOG << "Unknown next combi type:" << type;
    }

    if (result > 0) {
        DLOG << "Login successful, doing next combi request";
        // login success, do next request.
        doNextCombiRequest(ip, true);
    } else {
        DLOG << "Login failed, clearing pending request";
        //clearup the pending request if failed.
        _nextCombi.first = 0;
        _nextCombi.second = "";
    }
}

void NetworkUtil::handleCompatRegister(bool reg, const QString &infoJson)
{
    DLOG << "Handling compat register, reg:" << reg << "infoJson:" << infoJson.toStdString();
    auto ipc = CompatWrapper::instance()->ipcInterface();
    if (reg) {
        DLOG << "Registering discovery";
        ipc->call("registerDiscovery", Q_ARG(bool, false), Q_ARG(QString, ipc::CooperRegisterName), Q_ARG(QString, infoJson));
    } else {
        DLOG << "Unregistering discovery";
        ipc->call("registerDiscovery", Q_ARG(bool, true), Q_ARG(QString, ipc::CooperRegisterName), Q_ARG(QString, ""));
    }
}

void NetworkUtil::handleCompatDiscover()
{
    DLOG << "Handling compat discover";
    auto ipc = CompatWrapper::instance()->ipcInterface();
    QString nodesJson;
    ipc->call("getDiscovery", Q_RETURN_ARG(QString, nodesJson));

    // DLOG << "discovery return:" << nodesJson.toStdString();
    if (!nodesJson.isEmpty()) {
        DLOG << "Nodes JSON is not empty";
        picojson::value json_value;
        std::string err = picojson::parse(json_value, nodesJson.toStdString());
        if (!err.empty()) {
            WLOG << "Incorrect node list format: " << nodesJson.toStdString();
            DLOG << "Failed to parse node list JSON";
            return;
        }
        ipc::NodeList nodeList;
        nodeList.from_json(json_value);

        // typedef QMap<QString, QString> StringMap;
        StringMap infoMap;
        for (const auto &peerInfo : nodeList.peers) {
            auto ip = QString::fromStdString(peerInfo.os.ipv4);
            auto sharedip = QString::fromStdString(peerInfo.os.share_connect_ip);
            auto sharing = ShareHelper::instance()->selfSharing(sharedip);
            if (sharing > 0) {
                DLOG << "Self shared IP is running, resetting sharedip";
                // self shared ip but not running, reset the sharedip as empty.
                sharedip = "";
            }
            for (const auto &appInfo : peerInfo.apps) {
                if (appInfo.appname != ipc::CooperRegisterName) {
                    DLOG << "App is not CooperRegisterName, skipping";
                    continue;
                }

                auto info = QString::fromStdString(appInfo.json);
                auto combinedIP = ip + ", " + sharedip;
                infoMap.insert(info, combinedIP);
            }
        }

        if (!infoMap.empty()) {
            DLOG << "Info map is not empty, adding compat devices";
            // update this device info to discovery list
            metaObject()->invokeMethod(DiscoverController::instance(),
                                       "compatAddDeivces",
                                       Qt::QueuedConnection,
                                       Q_ARG(StringMap, infoMap));
        }
    } else {
        DLOG << "Nodes JSON is empty";
    }
}
#endif

void NetworkUtil::updateStorageConfig(const QString &value)
{
    DLOG << "Updating storage config to:" << value.toStdString();
    d->sessionManager->setStorageRoot(value);
    d->storageRoot = value;

#ifdef ENABLE_COMPAT
    DLOG << "Updating compat storage config";
    //update the storage dir for old protocol
    auto ipc = CompatWrapper::instance()->ipcInterface();
    ipc->call("saveAppConfig", Q_ARG(QString, ipc::CooperRegisterName), Q_ARG(QString, "storagedir"), Q_ARG(QString, value));
#else
    DLOG << "Compat mode not enabled, skipping compat storage config update";
#endif
}

void NetworkUtil::setStorageFolder(const QString &folder)
{
    DLOG << "Setting storage folder:" << folder.toStdString();
    d->storageFolder = folder;
}

QString NetworkUtil::getStorageFolder() const
{
    QString path = d->storageRoot + QDir::separator() + d->storageFolder;
    DLOG << "Getting storage folder path:" << path.toStdString();
    return path;
}

QString NetworkUtil::getConfirmTargetAddress() const
{
    return d->confirmTargetAddress;
}

void NetworkUtil::trySearchDevice(const QString &ip)
{
    DLOG << "searching " << ip.toStdString();

    _nextCombi.first = APPLY_INFO;
    _nextCombi.second = ip;
    // session connect and then send rpc request
    int logind = d->sessionManager->sessionConnect(ip, d->servePort, COO_HARD_PIN);
    if (logind < 0) {
        DLOG << "try apply search FAILED, try compat!";
        compatLogin(ip);
    } else if (logind > 0) {
        DLOG << "Already logged in, doing next combi request";
        // has been login, do next.
        doNextCombiRequest(ip);
    } else {
        DLOG << "Login status is 0, waiting for connection";
    }
}

void NetworkUtil::pingTarget(const QString &ip)
{
    // session connect by async, handle status in callback
    d->sessionManager->sessionPing(ip, d->servePort);
}

void NetworkUtil::reqTargetInfo(const QString &ip, bool compat)
{
    DLOG << "Requesting target info for IP:" << ip.toStdString() << "compat:" << compat;
    if (compat) {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doAsyncSearch", Q_ARG(QString, ip), Q_ARG(bool, false));
#else
        DLOG << "Compat mode not enabled, cannot request target info with compat";
#endif
    } else {
        DLOG << "Sending info apply";
        // send info apply, and sync handle
        ApplyMessage msg;
        msg.flag = ASK_QUIET;
        msg.host = ip.toStdString();
        msg.nick = deviceInfoStr().toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        d->sessionManager->sendRpcRequest(ip, APPLY_INFO, jsonMsg);
        // handle callback result in handleAsyncRpcResult
    }
}

void NetworkUtil::disconnectRemote(const QString &ip)
{
    d->sessionManager->sessionDisconnect(ip);
}

void NetworkUtil::compatLogin(const QString &ip)
{
#ifdef ENABLE_COMPAT
    DLOG << "Compat login for IP:" << ip.toStdString();
    auto appName = qAppName();
    // try connect with old protocol by daemon
    auto ipc = CompatWrapper::instance()->ipcInterface();
    ipc->call("doTryConnect", Q_ARG(QString, appName), Q_ARG(QString, ipc::CooperRegisterName),
              Q_ARG(QString, ip), Q_ARG(QString, ""));
#else
    DLOG << "Compat mode not enabled, cannot perform compat login";
#endif
}

void NetworkUtil::doNextCombiRequest(const QString &ip, bool compat)
{
    DLOG << "Doing next combi request for IP:" << ip.toStdString() << "compat:" << compat;
    auto type = _nextCombi.first;
    auto comip = _nextCombi.second;
    if (type <= 0) {
        DLOG << "Next combi type is not positive, returning";
        return;
    }

    switch (type) {

    case APPLY_INFO: {
        DLOG << "Next combi type is APPLY_INFO";
        reqTargetInfo(comip, compat);
        break;
    }
    case APPLY_TRANS: {
        DLOG << "Next combi type is APPLY_TRANS";
        sendTransApply(comip, compat);
        break;
    }
    case APPLY_SHARE: {
        DLOG << "Next combi type is APPLY_SHARE";
        sendShareApply(comip, compat);
        break;
    }
    default:
        WLOG << "unkown next combi type: " << type;
        break;
    }
    //reset
    _nextCombi.first = 0;
    _nextCombi.second = "";
}

void NetworkUtil::tryTransApply(const QString &ip)
{
    _nextCombi.first = APPLY_TRANS;
    _nextCombi.second = ip;

    // session connect and then send rpc request
    int logind = d->sessionManager->sessionConnect(ip, d->servePort, COO_HARD_PIN);
    if (logind < 0) {
        DLOG << "try apply trans FAILED, try compat!";
        compatLogin(ip);
    } else if (logind > 0) {
        DLOG << "Already logged in, doing next combi request";
        // has been login, do next.
        doNextCombiRequest(ip);
    } else {
        DLOG << "Login status is 0, waiting for connection";
    }
}

void NetworkUtil::sendTransApply(const QString &ip, bool compat)
{
    DLOG << "Sending transfer apply for IP:" << ip.toStdString() << "compat:" << compat;
    auto deviceName = CooperationUtil::deviceInfo().value(AppSettings::DeviceNameKey).toString();
    if (compat) {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        auto appName = qAppName();
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doApplyTransfer", Q_ARG(QString, appName), Q_ARG(QString, ipc::CooperRegisterName), Q_ARG(QString, deviceName));
#else
        DLOG << "Compat mode not enabled, cannot send transfer apply with compat";
#endif
    } else {
        DLOG << "Sending transfer apply via session manager";
        // update the target address
        d->confirmTargetAddress = ip;

        // send transfer apply, and async handle in RPC recv
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        msg.nick = deviceName.toStdString();   // user define nice name
        msg.host = CooperationUtil::localIPAddress().toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        d->sessionManager->sendRpcRequest(ip, APPLY_TRANS, jsonMsg);
    }
}

void NetworkUtil::tryShareApply(const QString &ip, const QString &selfprint)
{
    _nextCombi.first = APPLY_SHARE;
    _nextCombi.second = ip;

    _selfFingerPrint = selfprint;

    int logind = d->sessionManager->sessionConnect(ip, d->servePort, COO_HARD_PIN);
    if (logind < 0) {
        DLOG << "try apply share FAILED, try compat!";
        compatLogin(ip);
    } else if (logind > 0) {
        DLOG << "Already logged in, doing next combi request";
        // has been login, do next.
        doNextCombiRequest(ip);
    } else {
        DLOG << "Login status is 0, waiting for connection";
    }
}

void NetworkUtil::sendShareApply(const QString &ip, bool compat)
{
    DLOG << "Sending share apply for IP:" << ip.toStdString() << "compat:" << compat;
    DeviceInfoPointer selfinfo = DiscoverController::selfInfo();
    if (compat) {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        QStringList dataInfo({ selfinfo->deviceName(),
                               selfinfo->ipAddress() });
        QString data = dataInfo.join(',');
        auto appName = qAppName();
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doApplyShare", Q_ARG(QString, appName), Q_ARG(QString, appName), Q_ARG(QString, ip), Q_ARG(QString, data));
#else
        DLOG << "Compat mode not enabled, cannot send share apply with compat";
#endif
    } else {
        DLOG << "Sending share apply via session manager";
        // update the target address
        d->confirmTargetAddress = ip;

        // session connect and then send rpc request
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        msg.nick = selfinfo->deviceName().toStdString();
        msg.host = CooperationUtil::localIPAddress().toStdString();
        msg.fingerprint = _selfFingerPrint.toStdString();   // send self fingerprint
        QString jsonMsg = msg.as_json().serialize().c_str();
        d->sessionManager->sendRpcRequest(ip, APPLY_SHARE, jsonMsg);
    }
}

void NetworkUtil::sendDisconnectShareEvents(const QString &ip)
{
    DLOG << "Sending disconnect share events for IP:" << ip.toStdString();
    DeviceInfoPointer selfinfo = DiscoverController::selfInfo();
    if (ip == d->confirmTargetAddress) {
        DLOG << "IP matches confirmTargetAddress, sending RPC request";
        // session connect and then send rpc request
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        msg.nick = selfinfo->deviceName().toStdString();
        msg.host = CooperationUtil::localIPAddress().toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        d->sessionManager->sendRpcRequest(ip, APPLY_SHARE_STOP, jsonMsg);
    } else {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doDisconnectShare", Q_ARG(QString, qAppName()), Q_ARG(QString, qAppName()), Q_ARG(QString, selfinfo->deviceName()));
#else
        DLOG << "Compat mode not enabled, cannot send disconnect share events";
#endif
    }
}

void NetworkUtil::replyTransRequest(bool agree, const QString &targetIp)
{
    DLOG << "Replying transfer request, agree:" << agree << "target IP:" << targetIp.toStdString();
    if (d->confirmTargetAddress == targetIp) {
        DLOG << "Confirm target address matches, sending RPC request";
        // send transfer reply, skip result
        ApplyMessage msg;
        msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
        msg.host = CooperationUtil::localIPAddress().toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();

        // _confirmTargetAddress
        d->sessionManager->sendRpcRequest(d->confirmTargetAddress, APPLY_TRANS_RESULT, jsonMsg);

        d->sessionManager->updateSaveFolder(d->storageFolder);
    } else {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        auto deviceName = CooperationUtil::deviceInfo().value(AppSettings::DeviceNameKey).toString();
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doReplyTransfer", Q_ARG(QString, ipc::CooperRegisterName), Q_ARG(QString, ipc::CooperRegisterName),
                  Q_ARG(QString, deviceName), Q_ARG(bool, agree));
#else
        DLOG << "Compat mode not enabled, cannot reply transfer request";
#endif
    }
}

void NetworkUtil::replyShareRequest(bool agree, const QString &selfprint, const QString &targetIp)
{
    DLOG << "Replying share request, agree:" << agree << "selfprint:" << selfprint.toStdString() << "target IP:" << targetIp.toStdString();
    if (d->confirmTargetAddress == targetIp) {
        DLOG << "Confirm target address matches, sending RPC request";
        // send transfer reply, skip result
        ApplyMessage msg;
        msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
        msg.host = CooperationUtil::localIPAddress().toStdString();
        msg.fingerprint = selfprint.toStdString();   // send self fingerprint
        QString jsonMsg = msg.as_json().serialize().c_str();

        // _confirmTargetAddress
        d->sessionManager->sendRpcRequest(d->confirmTargetAddress, APPLY_SHARE_RESULT, jsonMsg);
    } else {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        int reply = agree ? SHARE_CONNECT_COMFIRM : SHARE_CONNECT_REFUSE;
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doReplyShare", Q_ARG(QString, qAppName()), Q_ARG(QString, qAppName()), Q_ARG(int, reply));
#else
        DLOG << "Compat mode not enabled, cannot reply share request";
#endif
    }
}

void NetworkUtil::replyShareRequestBusy(const QString &targetIp)
{
    WLOG << "Replying share request with BUSY status to:" << targetIp.toStdString();

    // 直接发送忙碌拒绝消息
    ApplyMessage msg;
    msg.flag = REPLY_REJECT;
    msg.nick = "BUSY_COOPERATING"; // Special marker for busy status
    msg.host = CooperationUtil::localIPAddress().toStdString();
    msg.fingerprint = ""; // No fingerprint needed for busy response
    QString jsonMsg = msg.as_json().serialize().c_str();

    d->sessionManager->sendRpcRequest(targetIp, APPLY_SHARE_RESULT, jsonMsg);
    WLOG << "Sent BUSY_COOPERATING rejection to:" << targetIp.toStdString();
}

void NetworkUtil::cancelApply(const QString &type, const QString &targetIp)
{
    DLOG << "Canceling apply, type:" << type.toStdString() << "target IP:" << targetIp.toStdString();
    if (d->confirmTargetAddress == targetIp) {
        DLOG << "Confirm target address matches, sending RPC request";
        ApplyMessage msg;
        msg.nick = type.toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        d->sessionManager->sendRpcRequest(d->confirmTargetAddress, APPLY_CANCELED, jsonMsg);
    } else {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        // FIXME: cancel trans apply
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doCancelShareApply", Q_ARG(QString, qAppName()));
#else
        DLOG << "Compat mode not enabled, cannot cancel apply";
#endif
    }
}

void NetworkUtil::cancelTrans(const QString &targetIp)
{
    DLOG << "Canceling transfer for IP:" << targetIp.toStdString();
    if (d->confirmTargetAddress == targetIp) {
        DLOG << "Confirm target address matches, canceling sync file";
        d->sessionManager->cancelSyncFile(d->confirmTargetAddress);
    } else {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        // TRANS_CANCEL 1008; coopertion jobid: 1000
        bool res = false;
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doOperateJob", Q_RETURN_ARG(bool, res), Q_ARG(int, 1008), Q_ARG(int, 1000), Q_ARG(QString, qAppName()));
#else
        DLOG << "Compat mode not enabled, cannot cancel transfer";
#endif
    }
}

void NetworkUtil::doSendFiles(const QStringList &fileList, const QString &targetIp)
{
    DLOG << "Sending files to IP:" << targetIp.toStdString();
    if (d->confirmTargetAddress == targetIp) {
        DLOG << "Confirm target address matches, sending files via session manager";
        int ranport = deepin_cross::CommonUitls::getAvailablePort();
        d->sessionManager->sendFiles(d->confirmTargetAddress, ranport, fileList);
    } else {
#ifdef ENABLE_COMPAT
        DLOG << "Compat mode enabled, trying with old protocol by daemon";
        auto session = CompatWrapper::instance()->session();
        auto deviceName = CooperationUtil::deviceInfo().value(AppSettings::DeviceNameKey).toString();
        QString saveDir = (deviceName + "(%1)").arg(CooperationUtil::localIPAddress());
        // try again with old protocol by daemon
        auto ipc = CompatWrapper::instance()->ipcInterface();
        ipc->call("doTransferFile", Q_ARG(QString, session), Q_ARG(QString, ipc::CooperRegisterName),
                  Q_ARG(int, 1000), Q_ARG(QStringList, fileList), Q_ARG(bool, true),
                  Q_ARG(QString, saveDir));
#else
        DLOG << "Compat mode not enabled, cannot send files";
#endif
    }
}

bool NetworkUtil::isCurrentlyCooperating()
{
    // 检查非兼容模式状态
    auto server = ShareCooperationServiceManager::instance()->server();
    auto client = ShareCooperationServiceManager::instance()->client();
    bool serverRunning = server && server->isRunning();
    bool clientRunning = client && client->isRunning();
    bool nonCompatCooperating = serverRunning || clientRunning;


    // 检查兼容模式状态（通过IPC获取Comshare状态）
    bool compatCooperating = false;
#ifdef ENABLE_COMPAT
    auto ipc = CompatWrapper::instance()->ipcInterface();
    ipc->call("getCurrentCooperationStatus", Q_RETURN_ARG(bool, compatCooperating));
#endif

    bool isCooperating = nonCompatCooperating || compatCooperating;

    DLOG << "NetworkUtil cooperation status check - nonCompatCooperating:" << nonCompatCooperating
         << " compatCooperating:" << compatCooperating
         << " isCooperating:" << isCooperating;

    return isCooperating;
}

QString NetworkUtil::deviceInfoStr()
{
    auto infomap = CooperationUtil::deviceInfo();

    // 将QVariantMap转换为QJsonDocument转换为QString
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(infomap);
    QString jsonString = jsonDocument.toJson(QJsonDocument::Compact);

    return jsonString;
}

void NetworkUtil::stop()
{
    DLOG << "Stopping NetworkUtil";
    metaObject()->invokeMethod(TransferHelper::instance(),
                               "closeAllNotifications",
                               Qt::QueuedConnection);

    // Disconnect current target if exists
    if (!d->confirmTargetAddress.isEmpty()) {
        DLOG << "Disconnecting current target:" << d->confirmTargetAddress.toStdString();
        disconnectRemote(d->confirmTargetAddress);
        d->confirmTargetAddress.clear();
    }

#ifdef ENABLE_COMPAT
    compatAppExit();
#endif
}

#ifdef ENABLE_COMPAT
void NetworkUtil::compatSendStartShare(const QString &screenName)
{
    DLOG << "Compat send start share for screen:" << screenName.toStdString();
    auto ipc = CompatWrapper::instance()->ipcInterface();
    ipc->call("doStartShare", Q_ARG(QString, qAppName()), Q_ARG(QString, screenName));
}

void NetworkUtil::compatAppExit()
{
    DLOG << "Stopping NetworkUtil";
    auto ipc = CompatWrapper::instance()->ipcInterface();
    ipc->call("appExit");
}

void NetworkUtil::updateCooperationStatus(int status)
{
    DLOG << "NetworkUtil updating cooperation status to:" << status;
    auto ipc = CompatWrapper::instance()->ipcInterface();
    ipc->call("updateCooperationStatus", Q_ARG(int, status));
}
#endif
