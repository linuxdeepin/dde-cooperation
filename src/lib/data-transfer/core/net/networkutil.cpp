// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkutil.h"
#include "networkutill_p.h"

#include "protoconstants.h"
#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"
#include "common/log.h"

#include "helper/transferhepler.h"

#include <QApplication>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>

NetworkUtilPrivate::NetworkUtilPrivate(NetworkUtil *qq)
    : q(qq)
{
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    LOG << "This is only transfer?" << onlyTransfer;

    ExtenMessageHandler msg_cb([this](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
        DLOG << "NetworkUtil >> " << mask << " msg_cb, json_msg: " << json_value << std::endl;
        switch (mask) {
        case SESSION_LOGIN: {
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
        case SESSION_TRANS: {
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
        case SESSION_STOP: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(TransferHelper::instance(),
                                          "handleCancelCooperApply",
                                          Qt::QueuedConnection);
        }
            return true;
        case SESSION_MESSAGE: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(TransferHelper::instance(),
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
    sessionManager->updatePin(DATA_HARD_PIN);

    sessionManager->sessionListen(DATA_SESSION_PORT);

    // connect(sessionManager, &SessionManager::notifyConnection, this, &NetworkUtilPrivate::handleConnectStatus);
    connect(sessionManager, &SessionManager::notifyTransChanged, this, &NetworkUtilPrivate::handleTransChanged);
}

NetworkUtilPrivate::~NetworkUtilPrivate()
{
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
}

void NetworkUtil::updatePassword(const QString &code)
{
    d->sessionManager->updatePin(code);
}

bool NetworkUtil::doConnect(const QString &ip, const QString &password)
{
    // session connect by async, handle status in callback
    d->sessionManager->sessionPing(ip, DATA_SESSION_PORT);

    bool logind = d->sessionManager->sessionConnect(ip, DATA_SESSION_PORT, password);
    if (logind)
        return true;
    return false;
}

void NetworkUtil::sendTransApply(const QString &ip)
{
    // session connect and then send rpc request
    bool logind = d->sessionManager->sessionConnect(ip, DATA_SESSION_PORT, DATA_HARD_PIN);

    metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                               Qt::QueuedConnection,
                               Q_ARG(int, logind ? 1 : 0),
                               Q_ARG(QString, ip),
                               Q_ARG(bool, true));

    if (logind) {
        // update the target address
        d->confirmTargetAddress = ip;

        // send transfer apply, and async handle in RPC recv
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        QString jsonMsg = msg.as_json().serialize().c_str();
        QString res = d->sessionManager->sendRpcRequest(ip, 1, jsonMsg);
        if (res.isEmpty()) {
            // transfer request send exception, it perhaps network error
            WLOG << "Send APPLY_TRANS failed.";
        }
    }
}

void NetworkUtil::replyTransRequest(bool agree)
{
    // send transfer reply, skip result
    ApplyMessage msg;
    msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
    QString jsonMsg = msg.as_json().serialize().c_str();

    // _confirmTargetAddress
    QString res = d->sessionManager->sendRpcRequest(d->confirmTargetAddress, 1, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send APPLY_TRANS_RESULT failed.";
    }

    d->sessionManager->updateSaveFolder(d->storageFolder);
}

void NetworkUtil::cancelTrans()
{
    d->sessionManager->cancelSyncFile(d->confirmTargetAddress);
}

void NetworkUtil::doSendFiles(const QStringList &fileList)
{
    if (d->confirmTargetAddress.isEmpty()) {
        WLOG << "No confirm address!!!";
        return;
    }
    d->sessionManager->sendFiles(d->confirmTargetAddress, 1, fileList);
}
