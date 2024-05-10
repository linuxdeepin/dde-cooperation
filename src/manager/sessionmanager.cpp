// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessionmanager.h"

#include "common/logger.h"
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
}

SessionManager::~SessionManager()
{
    _session_worker->stop();
    _trans_worker->stop();

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
    return _session_worker->connect(ip, port);
}

bool SessionManager::sessionConnect(QString ip, int port, QString password)
{
    return _session_worker->connectRemote(ip, port, password);
}

void SessionManager::sessionDisconnect(QString ip)
{
    LOG << "session disconnect:" << ip.toStdString();
    _session_worker->disconnectRemote();
}

void SessionManager::sendFiles(int port, QStringList paths)
{
    std::vector<std::string> name_vector;
    bool success = _trans_worker->tryStartSend(paths, port, &name_vector);
    if (!success) {
        ELOG << "Fail to send size: " << paths.size() << " at:" << port;
        emit notifyDoResult(false, "");
        return;
    }

    TransDataMessage req;
    req.id = std::to_string(_request_job_id);
    req.names = name_vector;
    req.token = "gen-temp-token";
    req.flag = true; // many folders
    req.size = -1; // unkown size
    proto::OriginMessage request;
    request.json_msg = req.as_json().serialize();

    auto response = _session_worker->sendRequest(request);
    if (DO_SUCCESS == response.mask) {
        emit notifyDoResult(true, "");
        _send_task = true;
        _request_job_id++;
    } else {
        // emit error!
        emit notifyDoResult(false, "");
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

void SessionManager::cancelSyncFile()
{
    DLOG << "cancelSyncFile is send?" << _send_task;
    _trans_worker->cancel(_send_task);

    TransCancelMessage req;
    req.id = _request_job_id;
    req.name = "all";
    req.reason = "";
    proto::OriginMessage request;
    request.json_msg = req.as_json().serialize();

    DLOG << "cancel name: " << req.name << " " << req.reason;

    auto response = _session_worker->sendRequest(request);
    if (DO_SUCCESS == response.mask) {
        emit notifyDoResult(true, "");
    } else {
        // emit error!
        emit notifyDoResult(false, "");
    }
}

QString SessionManager::sendRpcRequest(int type, const QString &reqJson)
{
    proto::OriginMessage request;
    request.mask = type;
    request.json_msg = reqJson.toStdString(); //req.as_json().serialize();

    auto response = _session_worker->sendRequest(request);
    if (DO_SUCCESS == response.mask) {
        QString res = QString(response.json_msg.c_str());
        return res;
    } else {
        // return empty msg if failed.
        return "";
    }
}
