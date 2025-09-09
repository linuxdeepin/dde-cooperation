// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessionmanager.h"

#include "common/log.h"
#include "common/commonutils.h"
#include "sessionproto.h"
#include "sessionworker.h"
#include "transferworker.h"
#include "filesizecounter.h"

#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>

SessionManager::SessionManager(QObject *parent) : QObject(parent)
{
    DLOG << "SessionManager";
    _trans_workers.clear();
    // Create session and transfer worker
    _session_worker = std::make_shared<SessionWorker>();
    connect(_session_worker.get(), &SessionWorker::onConnectChanged, this, &SessionManager::notifyConnection, Qt::QueuedConnection);
    connect(_session_worker.get(), &SessionWorker::onTransData, this, &SessionManager::handleTransData, Qt::QueuedConnection);
    connect(_session_worker.get(), &SessionWorker::onTransCount, this, &SessionManager::handleTransCount, Qt::QueuedConnection);
    connect(_session_worker.get(), &SessionWorker::onCancelJob, this, &SessionManager::handleCancelTrans, Qt::QueuedConnection);
    connect(_session_worker.get(), &SessionWorker::onRpcResult, this, &SessionManager::handleRpcResult, Qt::QueuedConnection);

    _file_counter = std::make_shared<FileSizeCounter>(this);
    connect(_file_counter.get(), &FileSizeCounter::onCountFinish, this, &SessionManager::handleFileCounted, Qt::QueuedConnection);
}

SessionManager::~SessionManager()
{
    DLOG << "~SessionManager";
    if (_file_counter) {
        DLOG << "Stopping file counter";
        _file_counter->stop();
        _file_counter.reset();
    }

    if (_session_worker) {
        DLOG << "Stopping session worker";
        _session_worker->stop();
        _session_worker.reset();
    }

    // release all transfer worker.
    DLOG << "Releasing all transfer workers";
    auto it = _trans_workers.begin();
    while (it != _trans_workers.end()) {
        it->second->stop();
        disconnect(it->second.get(), nullptr, nullptr, nullptr);

        it = _trans_workers.erase(it);
    }
    _trans_workers.clear();
}

void SessionManager::setSessionExtCallback(ExtenMessageHandler cb)
{
    DLOG << "setSessionExtCallback";
    _session_worker->setExtMessageHandler(cb);
}

void SessionManager::updatePin(QString code)
{
    DLOG << "updatePin: " << code.toStdString();
    _session_worker->updatePincode(code);
}

void SessionManager::setStorageRoot(const QString &root)
{
    DLOG << "setStorageRoot: " << root.toStdString();
    _save_root = QString(root);
}

void SessionManager::updateSaveFolder(const QString &folder)
{
    DLOG << "updateSaveFolder: " << folder.toStdString();
    if (_save_root.isEmpty()) {
        _save_root = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        DLOG << "Save root is empty, setting to default download location:" << _save_root.toStdString();
    }
    _save_dir = _save_root + QDir::separator();
    if (!folder.isEmpty()) {
        _save_dir += folder + QDir::separator();
        DLOG << "Save folder updated:" << _save_dir.toStdString();
    }
}

void SessionManager::updateLoginStatus(QString &ip, bool logined)
{
    DLOG << "updateLoginStatus ip: " << ip.toStdString() << " logined: " << logined;
    _session_worker->updateLogin(ip, logined);
}

void SessionManager::sessionListen(int port)
{
    DLOG << "sessionListen: " << port;
    bool success = _session_worker->startListen(port);
    if (!success) {
        ELOG << "Fail to start listen: " << port;
    }

//    emit notifyConnection(success, "");
}

bool SessionManager::sessionPing(QString ip, int port)
{
    LOG << "sessionPing: " << ip.toStdString();
    return _session_worker->netTouch(ip, port);
}

int SessionManager::sessionConnect(QString ip, int port, QString password)
{
    LOG << "sessionConnect: " << ip.toStdString();
    if (_session_worker->isClientLogin(ip)) {
        DLOG << "Client is already logged in:" << ip.toStdString();
        return 1;
    }
    if (!_session_worker->netTouch(ip, port)) {
        // ELOG << "Fail to connect remote:" << ip.toStdString();
        return -1;
    }

    // base64 the pwd
    QByteArray pinHash = password.toUtf8().toBase64();
    std::string pinString(pinHash.constData(), pinHash.size());

    LoginMessage req;
    req.name = deepin_cross::CommonUitls::getFirstIp();
    req.auth = pinString;

    QString jsonMsg = req.as_json().serialize().c_str();
    sendRpcRequest(ip, REQ_LOGIN, jsonMsg);

    return 0;
}

void SessionManager::sessionDisconnect(QString ip)
{
    LOG << "session disconnect:" << ip.toStdString();
    _session_worker->disconnectRemote();
}

std::shared_ptr<TransferWorker> SessionManager::createTransWorker(const QString &jobid)
{
    
    // Create a new TransferWorker
    auto newWorker = std::make_shared<TransferWorker>(jobid);
    // auto newWorker = QSharedPointer<TransferWorker>::create(this);
    connect(newWorker.get(), &TransferWorker::notifyChanged, this, &SessionManager::notifyTransChanged, Qt::QueuedConnection);
    connect(newWorker.get(), &TransferWorker::onException, this, &SessionManager::handleTransException, Qt::QueuedConnection);
    connect(newWorker.get(), &TransferWorker::onFinished, this, &SessionManager::handleTransFinish, Qt::QueuedConnection);

    return newWorker;
}

void SessionManager::releaseTransWorker(const QString &jobid)
{
    DLOG << "releaseTransWorker jobid: " << jobid.toStdString();
    auto it = _trans_workers.find(jobid);
    if (it != _trans_workers.end()) {
        DLOG << "Releasing transfer worker for job ID:" << jobid.toStdString();
        it->second->stop();
        disconnect(it->second.get(), nullptr, nullptr, nullptr);

        _trans_workers.erase(it);
    } else {
        WLOG << "Worker not found for job id: " << jobid.toStdString();
    }
}

void SessionManager::sendFiles(QString &ip, int port, QStringList paths)
{
    LOG << "sendFiles: " << ip.toStdString();
    std::vector<std::string> name_vector;
    std::string token;

    auto worker = createTransWorker(ip);
    bool success = worker->tryStartSend(paths, port, &name_vector, &token);
    if (!success) {
        ELOG << "Fail to send size: " << paths.size() << " at:" << port;
        return;
    }
    _trans_workers[ip] = worker;

    QString localIp = deepin_cross::CommonUitls::getFirstIp().data();
    QString accesstoken = QString::fromStdString(token);
    QString endpoint = QString("%1:%2:%3").arg(localIp).arg(port).arg(accesstoken);
    int64_t total = _file_counter->countFiles(ip, paths);
    bool needCount = total == 0;
    DLOG << "Sending files, needCount:" << needCount << "total size:" << total;

    TransDataMessage req;
    req.id = ip.toStdString();
    req.names = name_vector;
    req.endpoint = endpoint.toStdString();
    req.flag = needCount; // many folders
    req.size = total; // unkown size

    QString jsonMsg = req.as_json().serialize().c_str();
    sendRpcRequest(ip, REQ_TRANS_DATAS, jsonMsg);

    if (total > 0) {
        DLOG << "Total size is known, handling transfer count";
        QString oneName = paths.join(";");
        handleTransCount(oneName, total);
    }
}

void SessionManager::recvFiles(QString &ip, int port, QString &token, QStringList names)
{
    
    auto worker = createTransWorker(ip);
    bool success = worker->tryStartReceive(names, ip, port, token, _save_dir);
    if (!success) {
        ELOG << "Fail to recv name size: " << names.size() << " at:" << ip.toStdString();
        return;
    }

    _trans_workers[ip] = worker;
}

void SessionManager::cancelSyncFile(const QString &ip, const QString &reason)
{
    DLOG << "cancelSyncFile to: " << ip.toStdString();

    if (reason.contains("net_error")) {
        // no need to cancel sync file if net_error, or it will cause ui block.
        WLOG << "net_error, no need to cancel sync file";
    } else {
        DLOG << "Sending cancel RPC request";
        // first: send cancel rpc, target jobid is self ip addr.
        TransCancelMessage req;
        req.id = deepin_cross::CommonUitls::getFirstIp();
        req.name = "all";
        req.reason = reason.toStdString();

        QString jsonMsg = req.as_json().serialize().c_str();
        sendRpcRequest(ip, REQ_TRANS_CANCLE, jsonMsg);
    }

    // then: stop local worker
    DLOG << "Stopping local worker";
    handleCancelTrans(ip, reason);
}

void SessionManager::sendRpcRequest(const QString &ip, int type, const QString &reqJson)
{
    DLOG << "sendRpcRequest to: " << ip.toStdString();
    proto::OriginMessage request;
    request.mask = type;
    request.json_msg = reqJson.toStdString();
#ifdef QT_DEBUG
    DLOG << "sendRpcRequest " << request;
#endif
    WLOG << "sendRpcRequest " << request;
    _session_worker->sendAsyncRequest(ip, request);
}

void SessionManager::handleTransData(const QString endpoint, const QStringList nameVector)
{
    DLOG << "handleTransData from: " << endpoint.toStdString();
    QStringList parts = endpoint.split(":");
    if (parts.length() == 3) {
        DLOG << "Handling transfer data for endpoint:" << endpoint.toStdString();
        // 现在ip、port和token中分别包含了拆解后的内容
        recvFiles(parts[0], parts[1].toInt(), parts[2], nameVector);
    } else {
        // 错误处理，确保parts包含了3个元素
        WLOG << "endpoint format should be: ip:port:token";
    }
}

void SessionManager::handleTransCount(const QString names, quint64 size)
{
    DLOG << "handleTransCount names: " << names.toStdString();
    // TRANS_COUNT_SIZE = 50
    emit notifyTransChanged(50, names, size);
}

void SessionManager::handleCancelTrans(const QString jobid, const QString reason)
{
    DLOG << "handleCancelTrans jobid: " << jobid.toStdString();
    // stop & release the worker
    releaseTransWorker(jobid);

    if (!reason.isEmpty()) {
        DLOG << "Handling transfer cancellation with reason:" << reason.toStdString();
        // TRANS_EXCEPTION = 49
        emit notifyTransChanged(49, reason, 0);
    } else {
        DLOG << "Handling transfer cancellation by user";
        // TRANS_CANCELED = 48 by user
        emit notifyTransChanged(48, "", 0);
    }
}

void SessionManager::handleFileCounted(const QString ip, const QStringList paths, quint64 totalSize)
{
    DLOG << "handleFileCounted ip: " << ip.toStdString();
    if (ip.isEmpty()) {
        WLOG << "empty target address for file counted.";
        return;
    }
    DLOG << "Handling file counted for ip:" << ip.toStdString() << "total size:" << totalSize;
    std::vector<std::string> nameVector;
    foreach (const QString &path, paths) {
        nameVector.push_back(path.toStdString());
    }

    TransDataMessage req;
    req.id = ip.toStdString();
    req.names = nameVector;
    req.endpoint = "::";
    req.flag = false; // no need count
    req.size = totalSize;

    QString jsonMsg = req.as_json().serialize().c_str();
    sendRpcRequest(ip, INFO_TRANS_COUNT, jsonMsg);

    // notify local
    DLOG << "Notifying local about file count";
    QString oneName = paths.join(";");
    handleTransCount(oneName, totalSize);
}


void SessionManager::handleRpcResult(int32_t type, const QString &response)
{
#ifdef QT_DEBUG
    DLOG << "RPC Result type=" << type << " response: " << response.toStdString();
#endif
    // notify the result to upper caller
    emit notifyAsyncRpcResult(type, response);
}

void SessionManager::handleTransException(const QString jobid, const QString reason)
{
    WLOG << jobid.toStdString() << " transfer occur exception: " << reason.toStdString();

    cancelSyncFile(jobid, reason);
}

void SessionManager::handleTransFinish(const QString jobid)
{
#ifdef QT_DEBUG
    DLOG << jobid.toStdString() << " transfer finished!";
#endif
    // stop & release the worker
    releaseTransWorker(jobid);
}

