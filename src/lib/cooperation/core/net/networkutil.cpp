// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkutil.h"
#include "networkutill_p.h"

#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"

#include "cooconstrants.h"
#include "discover/discovercontroller.h"
#include "helper/transferhelper.h"
#include "helper/sharehelper.h"
#include "utils/cooperationutil.h"

#include <QJsonDocument>
#include <QDebug>
#include <QDir>

using namespace cooperation_core;
NetworkUtilPrivate::NetworkUtilPrivate(NetworkUtil *qq)
    : q(qq)
{
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    LOG << "This is only transfer?" << onlyTransfer;

    ExtenMessageHandler msg_cb([this](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
#ifdef QT_DEBUG
        DLOG << "NetworkUtil >> " << mask << " msg_cb, json_msg: " << json_value << std::endl;
#endif
        switch (mask) {
        case APPLY_INFO: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;

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
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            *res_msg = res.as_json().serialize();
            confirmTargetAddress = QString::fromStdString(req.host);
            storageFolder = QString::fromStdString(req.nick + "(" + req.host + ")");
            q->metaObject()->invokeMethod(TransferHelper::instance(),
                                          "notifyTransferRequest",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.nick.c_str())));
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
            QString info = QString(req.host.c_str()) + "," + req.nick.c_str();
            *res_msg = res.as_json().serialize();
            confirmTargetAddress = QString::fromStdString(req.host);
            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "notifyConnectRequest",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, info));
        }
            return true;
        case APPLY_SHARE_RESULT: {
            ApplyMessage req, res;
            req.from_json(json_value);
            bool agree = (req.flag == REPLY_ACCEPT);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "handleConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, agree ? SHARE_CONNECT_COMFIRM : SHARE_CONNECT_REFUSE));
        }
            return true;
        case APPLY_SHARE_STOP: {
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
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            if (req.nick == "share")
                q->metaObject()->invokeMethod(ShareHelper::instance(),
                                              "handleCancelCooperApply",
                                              Qt::QueuedConnection);
            else if (req.nick == "transfer")
                q->metaObject()->invokeMethod(TransferHelper::instance(),
                                              "handleCancelTransferApply",
                                              Qt::QueuedConnection);
        }
            return true;
        }

        // unhandle message
        return false;
    });

    sessionManager = new SessionManager(this);
    sessionManager->setSessionExtCallback(msg_cb);
    sessionManager->updatePin(COO_HARD_PIN);

    sessionManager->sessionListen(COO_SESSION_PORT);

    connect(sessionManager, &SessionManager::notifyConnection, this, &NetworkUtilPrivate::handleConnectStatus);
    connect(sessionManager, &SessionManager::notifyTransChanged, this, &NetworkUtilPrivate::handleTransChanged);
    connect(sessionManager, &SessionManager::notifyAsyncRpcResult, this, &NetworkUtilPrivate::handleAsyncRpcResult);
}

NetworkUtilPrivate::~NetworkUtilPrivate()
{
}

void NetworkUtilPrivate::handleConnectStatus(int result, QString reason)
{
    DLOG << " connect status: " << result << " " << reason.toStdString();
    if (result == 113 || result == 110) {
        // host unreachable or timeout
        metaObject()->invokeMethod(DiscoverController::instance(),
                                   "addSearchDeivce",
                                   Qt::QueuedConnection,
                                   Q_ARG(QString, ""));
        return;
    }
    if (result == -2) {
        // error hanppend
        DLOG << "connect error, reason = " << reason.toStdString();
    } else if (result == -1) {
        // disconnected
        q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, 0),
                                      Q_ARG(QString, reason),
                                      Q_ARG(bool, false));
    } else if (result == 2) {
        // connected
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
    // handle failed result
    if (response.isEmpty()) {
        WLOG << "FAIL to RPC: " << type;

        if (APPLY_SHARE == type) {
            q->metaObject()->invokeMethod(ShareHelper::instance(),
                                          "handleConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, SHARE_CONNECT_UNABLE));
        } else if (APPLY_TRANS == type) {
            q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, 0),
                                          Q_ARG(QString, confirmTargetAddress),
                                          Q_ARG(bool, true));
        }

        return;
    }

    // handle success result
    if (APPLY_INFO == type) {
        picojson::value json_value;
        std::string err = picojson::parse(json_value, response.toStdString());
        if (!err.empty()) {
            DLOG << "Failed to parse JSON data: " << err;
            return;
        }

        ApplyMessage reply;
        reply.from_json(json_value);
        // update this device info to discovery list
        q->metaObject()->invokeMethod(DiscoverController::instance(),
                                      "addSearchDeivce",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, QString(reply.nick.c_str())));
    }
}

NetworkUtil::NetworkUtil(QObject *parent)
    : QObject(parent),
      d(new NetworkUtilPrivate(this))
{
}

NetworkUtil::~NetworkUtil()
{
}

NetworkUtil *NetworkUtil::instance()
{
    static NetworkUtil ins;
    return &ins;
}

void NetworkUtil::updateStorageConfig(const QString &value)
{
    d->sessionManager->setStorageRoot(value);
    d->storageRoot = value;
}

QString NetworkUtil::getStorageFolder() const
{
    return d->storageRoot + QDir::separator() + d->storageFolder;
}

QString NetworkUtil::getConfirmTargetAddress() const
{
    return d->confirmTargetAddress;
}

void NetworkUtil::searchDevice(const QString &ip)
{
    DLOG << "searching " << ip.toStdString();

    // session connect and then send rpc request
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);
    if (logind) {
        reqTargetInfo(ip);
    }
}

void NetworkUtil::pingTarget(const QString &ip)
{
    // session connect by async, handle status in callback
    d->sessionManager->sessionPing(ip, COO_SESSION_PORT);
}

void NetworkUtil::reqTargetInfo(const QString &ip)
{
    // send info apply, and sync handle
    ApplyMessage msg;
    msg.flag = ASK_QUIET;
    msg.host = ip.toStdString();
    msg.nick = deviceInfoStr().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    d->sessionManager->sendRpcRequest(ip, APPLY_INFO, jsonMsg);
    // handle callback result in handleAsyncRpcResult
}

void NetworkUtil::sendTransApply(const QString &ip)
{
    // session connect and then send rpc request
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);

    metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                               Qt::QueuedConnection,
                               Q_ARG(int, logind ? 1 : 0),
                               Q_ARG(QString, ip),
                               Q_ARG(bool, true));

    if (logind) {
        // update the target address
        d->confirmTargetAddress = ip;

        auto deviceName = CooperationUtil::deviceInfo().value(AppSettings::DeviceNameKey).toString();

        // send transfer apply, and async handle in RPC recv
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        msg.nick = deviceName.toStdString();   // user define nice name
        msg.host = CooperationUtil::localIPAddress().toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        d->sessionManager->sendRpcRequest(ip, APPLY_TRANS, jsonMsg);
    }
}

void NetworkUtil::sendShareEvents(const QString &ip)
{
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);
    if (!logind) {
        WLOG << "fail to login: " << ip.toStdString() << " pls try again.";
        metaObject()->invokeMethod(ShareHelper::instance(),
                                   "handleConnectResult",
                                   Qt::QueuedConnection,
                                   Q_ARG(int, SHARE_CONNECT_UNABLE));
        return;
    }

    // update the target address
    d->confirmTargetAddress = ip;

    DeviceInfoPointer selfinfo = DiscoverController::selfInfo();
    // session connect and then send rpc request
    ApplyMessage msg;
    msg.flag = ASK_NEEDCONFIRM;
    msg.nick = selfinfo->deviceName().toStdString();
    msg.host = CooperationUtil::localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    d->sessionManager->sendRpcRequest(ip, APPLY_SHARE, jsonMsg);
}

void NetworkUtil::sendDisconnectShareEvents(const QString &ip)
{
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);
    if (!logind) {
        WLOG << "fail to login: " << ip.toStdString() << " pls try again.";
        return;
    }

    DeviceInfoPointer selfinfo = DiscoverController::selfInfo();
    // session connect and then send rpc request
    ApplyMessage msg;
    msg.flag = ASK_NEEDCONFIRM;
    msg.nick = selfinfo->deviceName().toStdString();
    msg.host = CooperationUtil::localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    d->sessionManager->sendRpcRequest(ip, APPLY_SHARE_STOP, jsonMsg);
}

void NetworkUtil::replyTransRequest(bool agree)
{
    // send transfer reply, skip result
    ApplyMessage msg;
    msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
    msg.host = CooperationUtil::localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();

    // _confirmTargetAddress
    d->sessionManager->sendRpcRequest(d->confirmTargetAddress, APPLY_TRANS_RESULT, jsonMsg);

    d->sessionManager->updateSaveFolder(d->storageFolder);
}

void NetworkUtil::replyShareRequest(bool agree)
{
    // send transfer reply, skip result
    ApplyMessage msg;
    msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
    msg.host = CooperationUtil::localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();

    // _confirmTargetAddress
    d->sessionManager->sendRpcRequest(d->confirmTargetAddress, APPLY_SHARE_RESULT, jsonMsg);
}

void NetworkUtil::cancelApply(const QString &type)
{
    if (d->confirmTargetAddress.isEmpty()) {
        WLOG << "No confirm address!!!";
        return;
    }
    ApplyMessage msg;
    msg.nick = type.toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    d->sessionManager->sendRpcRequest(d->confirmTargetAddress, APPLY_CANCELED, jsonMsg);
}

void NetworkUtil::cancelTrans()
{
    if (d->confirmTargetAddress.isEmpty()) {
        WLOG << "No confirm address!!!";
        return;
    }
    d->sessionManager->cancelSyncFile(d->confirmTargetAddress);
}

void NetworkUtil::doSendFiles(const QStringList &fileList)
{
    if (d->confirmTargetAddress.isEmpty()) {
        WLOG << "No confirm address!!!";
        return;
    }
    d->sessionManager->sendFiles(d->confirmTargetAddress, COO_WEB_PORT, fileList);
}

QString NetworkUtil::deviceInfoStr()
{
    auto infomap = CooperationUtil::deviceInfo();
    infomap.insert("IPAddress", CooperationUtil::localIPAddress());

    // 将QVariantMap转换为QJsonDocument转换为QString
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(infomap);
    QString jsonString = jsonDocument.toJson(QJsonDocument::Compact);

    return jsonString;
}
