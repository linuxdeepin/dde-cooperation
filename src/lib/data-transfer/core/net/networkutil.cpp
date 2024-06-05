// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkutil.h"
#include "networkutill_p.h"

#include "protoconstants.h"
#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"
#include "common/log.h"
#include "common/constant.h"

#include "helper/transferhepler.h"

#include <QApplication>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QTime>

#include <utils/transferutil.h>

NetworkUtilPrivate::NetworkUtilPrivate(NetworkUtil *qq)
    : q(qq)
{
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    LOG << "This is only transfer?" << onlyTransfer;

    ExtenMessageHandler msg_cb([this](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
        DLOG << "NetworkUtil >> " << mask << " msg_cb, json_msg: " << json_value << std::endl;
        switch (mask) {
        case SESSION_MESSAGE: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(TransferHelper::instance(),
                                          "handleMessage",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.nick.c_str())));
        }
            return true;
        }
        // unhandle message
        return false;
    });

    sessionManager = new SessionManager(this);
    sessionManager->setSessionExtCallback(msg_cb);

    sessionManager->sessionListen(DATA_SESSION_PORT);

    connect(sessionManager, &SessionManager::notifyConnection, this, &NetworkUtilPrivate::handleConnectStatus);
    connect(sessionManager, &SessionManager::notifyTransChanged, this, &NetworkUtilPrivate::handleTransChanged);
}

NetworkUtilPrivate::~NetworkUtilPrivate()
{
}

void NetworkUtilPrivate::handleConnectStatus(int result, QString reason)
{
    DLOG << " connect status: " << result << " " << reason.toStdString();
    if (result == 2)
        confirmTargetAddress = reason;

    if (result == -1 && confirmTargetAddress == reason)
        TransferHelper::instance()->emitDisconnected();

    if (result == LOGIN_SUCCESS) {
        QString unfinishJson;
        int remainSpace = TransferUtil::getRemainSize();
        bool unfinish = TransferUtil::isUnfinishedJob(unfinishJson, confirmTargetAddress);
        if (unfinish)
            TransferHelper::instance()->sendMessage("unfinish_json", unfinishJson);
        TransferHelper::instance()->sendMessage("remaining_space", QString::number(remainSpace));
        emit TransferHelper::instance()->connectSucceed();
        return;
    }
}

void NetworkUtilPrivate::handleTransChanged(int status, const QString &path, quint64 size)
{
    switch (status) {
    case TRANS_CANCELED:
        //cancelTransfer(path.compare("im_sender") == 0);
        break;
    case TRANS_EXCEPTION:
        //TODO: notify show exception UI
        break;
    case TRANS_COUNT_SIZE:
        // only update the total size while rpc notice
        transferInfo.totalSize = size;
        break;
    case TRANS_WHOLE_START:
        emit TransferHelper::instance()->transferring();
        break;
    case TRANS_WHOLE_FINISH:
#ifdef __linux__
        TransferHelper::instance()->setting(TransferUtil::DownLoadDir());
#endif
        break;
    case TRANS_INDEX_CHANGE:
        break;
    case TRANS_FILE_CHANGE:
        break;
    case TRANS_FILE_SPEED: {
        transferInfo.transferSize += size;
        transferInfo.maxTimeS += 1;   // 每1秒收到此信息
        updateTransProgress(transferInfo.transferSize, path);

    } break;
    case TRANS_FILE_DONE:
        break;
    }
}

void NetworkUtilPrivate::updateTransProgress(uint64_t current, const QString &path)
{
    QTime time(0, 0, 0);
    if (transferInfo.totalSize < 1) {
        // the total has not been set.
        emit TransferHelper::instance()->transferContent(tr("Transfering"), "", 0, 0);
        return;
    }

    // 计算整体进度和预估剩余时间
    double value = static_cast<double>(current) / transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);

    int remain_time;
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        progressValue = 100;
        remain_time = 0;
    } else {
        remain_time = (transferInfo.maxTimeS * 100 / progressValue - transferInfo.maxTimeS);
    }
    time = time.addSecs(remain_time);

    DLOG << "progressbar: " << progressValue << " remain_time=" << remain_time;
    DLOG << "totalSize: " << transferInfo.totalSize << " transferSize=" << current;

    emit TransferHelper::instance()->transferContent(tr("Transfering"), path, progressValue, remain_time);
}

NetworkUtil::NetworkUtil(QObject *parent)
    : QObject(parent),
      d(new NetworkUtilPrivate(this))
{
    updateStorageConfig();
}

NetworkUtil::~NetworkUtil()
{
}

NetworkUtil *NetworkUtil::instance()
{
    static NetworkUtil ins;
    return &ins;
}

void NetworkUtil::updateStorageConfig()
{
    d->sessionManager->updateSaveFolder("/Downloads");
}

void NetworkUtil::updatePassword(const QString &code)
{
    d->sessionManager->updatePin(code);
}

bool NetworkUtil::doConnect(const QString &ip, const QString &password)
{
    bool logind = d->sessionManager->sessionConnect(ip, DATA_SESSION_PORT, password);
    if (logind) {
        d->confirmTargetAddress = ip;
        return true;
    }
    return false;
}

void NetworkUtil::disConnect()
{
    if (!d->confirmTargetAddress.isEmpty())
        d->sessionManager->sessionDisconnect(d->confirmTargetAddress);
}

bool NetworkUtil::sendMessage(const QString &message)
{
    ApplyMessage msg;
    msg.nick = message.toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    QString res = d->sessionManager->sendRpcRequest(d->confirmTargetAddress, SESSION_MESSAGE, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send SESSION_MESSAGE failed.";
    }
    return true;
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
    d->sessionManager->sendFiles(d->confirmTargetAddress, DATA_WEB_PORT, fileList);
}
