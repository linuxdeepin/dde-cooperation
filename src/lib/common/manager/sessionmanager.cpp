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
    connect(_session_worker.get(), &SessionWorker::onTransCount, this, &SessionManager::handleTransCount);

    connect(_trans_worker.get(), &TransferWorker::notifyChanged, this, &SessionManager::notifyTransChanged);


    _file_counter = std::make_shared<FileSizeCounter>(this);
    connect(_file_counter.get(), &FileSizeCounter::onCountFinish, this, &SessionManager::handleFileCounted);
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

void SessionManager::calculateTotalSize(const QStringList &paths, std::function<void(qint64)> onFinish)
{
    qint64 totalSize = 0;

    std::function<void(const QString &)> calculateSizeRecursively = [&](const QString &currentPath) {
        QDir dir(currentPath);
        if (!dir.exists()) {
            WLOG << "directory not exist: " << currentPath.toStdString();
            return;
        }

        QFileInfoList fileList = dir.entryInfoList(QDir::Files);
        for (const QFileInfo &fileInfo : fileList) {
            totalSize += fileInfo.size();
        }

        QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &subDirInfo : dirList) {
            calculateSizeRecursively(subDirInfo.filePath());
        }
    };

    for (const QString &path : paths) {
        calculateSizeRecursively(path);
    }

    onFinish(totalSize);
}

void SessionManager::calculateTotalSizeWithTimeout(const QStringList &paths, std::function<void(qint64)> onFinish)
{
    qint64 totalSize = 0;

    // 创建定时器，超时时间为 200 毫秒
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(200);

    // 定义超时处理函数
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        timer.stop();
        onFinish(0);  // 如果超时，则返回大小为 0
    });

    // 正常计算文件大小的逻辑
    std::function<void(const QString &)> calculateSizeRecursively = [&](const QString &currentPath) {
        QFileInfo fileInfo(currentPath);
        if (fileInfo.isFile()) {
            totalSize += fileInfo.size();
            return;
        }

        QDir dir(currentPath);
        if (!dir.exists()) {
            WLOG << "directory not exist: " << currentPath.toStdString();
            return;
        }

        QFileInfoList fileList = dir.entryInfoList(QDir::Files);
        for (const QFileInfo &fileInfo : fileList) {
            totalSize += fileInfo.size();
        }

        QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &subDirInfo : dirList) {
            calculateSizeRecursively(subDirInfo.filePath());
        }
    };

    for (const QString &path : paths) {
        calculateSizeRecursively(path);
    }

    // 运行完成后停止定时器，并返回计算结果
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        onFinish(totalSize);
    });
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
    int64_t total = _file_counter->countFiles(ip, paths);
    bool needCount = total == 0;

    TransDataMessage req;
    req.id = std::to_string(_request_job_id);
    req.names = name_vector;
    req.endpoint = endpoint.toStdString();
    req.flag = needCount; // many folders
    req.size = total; // unkown size

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

    if (total > 0) {
        QString oneName = paths.join(";");
        handleTransCount(oneName, total);
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

void SessionManager::handleTransCount(const QString names, quint64 size)
{
    // TRANS_COUNT_SIZE = 50
    emit notifyTransChanged(50, names, size);
}

void SessionManager::handleFileCounted(const QString ip, const QStringList paths, quint64 totalSize)
{
    if (ip.isEmpty()) {
        WLOG << "empty target address for file counted.";
        return;
    }
    std::vector<std::string> nameVector;
    for (auto path : paths) {
        QFileInfo fileInfo(path);
        std::string name = fileInfo.fileName().toStdString();
        nameVector.push_back(name);
    }

    TransDataMessage req;
    req.id = std::to_string(_request_job_id);
    req.names = nameVector;
    req.endpoint = "::";
    req.flag = false; // no need count
    req.size = totalSize;

    QString jsonMsg = req.as_json().serialize().c_str();
    QString res = sendRpcRequest(ip, INFO_TRANS_COUNT, jsonMsg);
    if (res.isEmpty()) {
        ELOG << "send INFO_TRANS_COUNT failed.";
    }

    // notify local
    QString oneName = paths.join(";");
    handleTransCount(oneName, totalSize);
}
