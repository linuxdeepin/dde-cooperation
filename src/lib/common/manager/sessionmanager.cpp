// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessionmanager.h"

#include "common/log.h"
#include "common/commonutils.h"
#include "sessionproto.h"
#include "sessionworker.h"
#include "transferworker.h"

SessionManager::SessionManager(QObject *parent) : QObject(parent)
{

    // Create a new Asio service
    asio_service = std::make_shared<AsioService>();

    // Start the Asio service
    asio_service->Start();

    // Create session and transfer worker
    _session_worker = std::make_shared<SessionWorker>(asio_service);
    _trans_worker = std::make_shared<TransferWorker>(asio_service);

    connect(_session_worker.get(), &SessionWorker::onConnectChanged, this, &SessionManager::notifyConnection);
    connect(_session_worker.get(), &SessionWorker::onTransData, this, &SessionManager::handleTransData);
}

SessionManager::~SessionManager()
{
    //FIXME: abort if stop them
    //_session_worker->stop();
    //_trans_worker->stop();

    if (!asio_service) {
        // Stop the Asio service
        asio_service->Stop();
    }
}

void SessionManager::setSessionExtCallback(ExtenMessageHandler cb)
{
    _session_worker->setExtMessageHandler(cb);
}

void SessionManager::updatePin(QString code)
{
    _session_worker->updatePincode(code);
}

void SessionManager::setStorageRoot(const QString &root)
{
    _trans_worker->updateSaveRoot(root);
}

void SessionManager::updateSaveFolder(const QString &folder)
{
    _save_dir = QString(folder);
}

void SessionManager::sessionListen(int port)
{
    bool success = _session_worker->startListen(port);
    if (!success) {
        ELOG << "Fail to start listen: " << port;
    }

//    emit notifyConnection(success, "");
}

bool SessionManager::sessionPing(QString ip, int port)
{
    LOG << "sessionPing: " << ip.toStdString();
    return _session_worker->clientPing(ip, port);
}

bool SessionManager::sessionConnect(QString ip, int port, QString password)
{
    LOG << "sessionConnect: " << ip.toStdString();
    if (_session_worker->isClientLogin(ip))
        return true;
    return _session_worker->connectRemote(ip, port, password);
}

void SessionManager::sessionDisconnect(QString ip)
{
    LOG << "session disconnect:" << ip.toStdString();
    _session_worker->disconnectRemote();
}

void SessionManager::sendFiles(QString &ip, int port, QStringList paths)
{
    std::vector<std::string> name_vector;
    std::string token;
    bool success = _trans_worker->tryStartSend(paths, port, &name_vector, &token);
    if (!success) {
        ELOG << "Fail to send size: " << paths.size() << " at:" << port;
        emit notifyDoResult(false, "");
        return;
    }

    QString localIp = deepin_cross::CommonUitls::getFirstIp().data();
    QString accesstoken = QString::fromStdString(token);
    QString endpoint = QString("%1:%2:%3").arg(localIp).arg(port).arg(accesstoken);
    WLOG << "endpoint: " << endpoint.toStdString();

    TransDataMessage req;
    req.id = std::to_string(_request_job_id);
    req.names = name_vector;
    req.endpoint = endpoint.toStdString();
    req.flag = true; // many folders
    req.size = -1; // unkown size

    QString jsonMsg = req.as_json().serialize().c_str();
    QString res = sendRpcRequest(ip, REQ_TRANS_DATAS, jsonMsg);
    if (res.isEmpty()) {
        ELOG << "send REQ_TRANS_DATAS failed.";
        emit notifyDoResult(false, "");
    } else {
        _send_task = true;
        _request_job_id++;
        emit notifyDoResult(true, "");
    }
}

void SessionManager::recvFiles(QString &ip, int port, QString &token, QStringList names)
{
    bool success = _trans_worker->tryStartReceive(names, ip, port, token, _save_dir);
    if (!success) {
        ELOG << "Fail to recv name size: " << names.size() << " at:" << ip.toStdString();
        emit notifyDoResult(false, "");
        return;
    }

    emit notifyDoResult(true, "");
}

void SessionManager::cancelSyncFile(QString &ip)
{
    DLOG << "cancelSyncFile is send?" << _send_task;
    _trans_worker->cancel(_send_task);

    TransCancelMessage req;
    req.id = _request_job_id;
    req.name = "all";
    req.reason = "";

    DLOG << "cancel name: " << req.name << " " << req.reason;

    QString jsonMsg = req.as_json().serialize().c_str();
    QString res = sendRpcRequest(ip, REQ_TRANS_CANCLE, jsonMsg);
    if (res.isEmpty()) {
        ELOG << "send REQ_TRANS_CANCLE failed.";
        emit notifyDoResult(false, "");
    } else {
        emit notifyDoResult(true, "");
    }
}

QString SessionManager::sendRpcRequest(const QString &ip, int type, const QString &reqJson)
{
    proto::OriginMessage request;
    request.mask = type;
    request.json_msg = reqJson.toStdString(); //req.as_json().serialize();

    DLOG << "sendRpcRequest " << request;

    auto response = _session_worker->sendRequest(ip, request);
    if (DO_SUCCESS == response.mask) {
        QString res = QString(response.json_msg.c_str());
        return res;
    } else {
        // return empty msg if failed.
        return "";
    }
}

void SessionManager::handleTransData(const QString endpoint, const QStringList nameVector)
{
    QStringList parts = endpoint.split(":");
    if (parts.length() == 3) {
        // 现在ip、port和token中分别包含了拆解后的内容
        recvFiles(parts[0], parts[1].toInt(), parts[2], nameVector);
    } else {
        // 错误处理，确保parts包含了3个元素
        WLOG << "endpoint format should be: ip:port:token";
    }
}
