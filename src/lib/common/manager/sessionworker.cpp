// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sessionworker.h"

#include "sessionmanager.h"

#include "common/log.h"

#include <QHostInfo>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QStorageInfo>

SessionWorker::SessionWorker(const std::shared_ptr<AsioService> &service, QObject *parent)
    : QObject(parent)
    , _service(service)
{
}

void SessionWorker::onReceivedMessage(const proto::OriginMessage &request, proto::OriginMessage *response)
{
    if (request.json_msg.empty()) {
        DLOG << "empty json message: ";
        return;
    }

    DLOG << "onReceivedMessage mask=" << request.mask << " json_msg: " << request.json_msg << std::endl;

    // Step 2: 解析响应数据
    picojson::value v;
    std::string err = picojson::parse(v, request.json_msg);
    if (!err.empty()) {
        DLOG << "Failed to parse JSON data: " << err;
        return;
    }

    // if these exten mssages are handled, return
    if (_extMsghandler) {
        std::string res_json;
        bool handled = _extMsghandler(request.mask, v, &res_json);
        if (handled) {
            response->id = request.id;
            response->mask = DO_SUCCESS;
            response->json_msg = res_json;
            return;
        }
    }

//    picojson::array files = v.get("files").get<picojson::array>();
    int type = request.mask;
    switch (type) {
    case REQ_LOGIN: {
        LoginMessage req, res;
        req.from_json(v);
        DLOG << "Login: " << req.name << " " << req.auth;
        std::hash<std::string> hasher;
        uint32_t pinHash = (uint32_t)hasher(_savedPin.toStdString());
        uint32_t pwdnumber = std::stoul(req.auth);
        if (pinHash == pwdnumber) {
            res.auth = "thatsgood";
            response->mask = LOGIN_SUCCESS;
        } else {
            // return empty auth token.
            res.auth = "";
            response->mask = LOGIN_DENIED;
        }

        res.name = QHostInfo::localHostName().toStdString();
        response->json_msg = res.as_json().serialize();
    }
    break;
    case REQ_FREE_SPACE: {
        FreeSpaceMessage req, res;
        req.from_json(v);

        int remainSpace = 0;//TransferHelper::getRemainSize(); // xx G
        res.free = remainSpace;
        response->json_msg = res.as_json().serialize();
    }
    break;
    case REQ_TRANS_DATAS: {
        TransDataMessage req, res;
        req.from_json(v);

        QString endpoint = QString::fromStdString(req.endpoint);
        QStringList nameList;
        for (auto name : req.names) {
            nameList.append(QString::fromStdString(name));
        }

        res.id = req.id;
        res.names = req.names;
        res.flag = true;
        res.size = 0;//TransferHelper::getRemainSize();
        response->json_msg = res.as_json().serialize();

        emit onTransData(endpoint, nameList);

        uint64_t total = req.size;
        if (total > 0) {
            QString oneName = nameList.join(";");
            emit onTransCount(oneName, total);
        }
    }
    break;
    case REQ_TRANS_CANCLE: {
        TransCancelMessage req, res;
        req.from_json(v);

        DLOG << "cancel name: " << req.name << " " << req.reason;

        res.id = req.id;
        res.name = req.name;
        res.reason = "";
        response->json_msg = res.as_json().serialize();

        emit onCancelJob(req.id);
    }
    break;
    case CAST_INFO: {
    }
    break;
    case INFO_TRANS_COUNT: {
        TransDataMessage req, res;
        req.from_json(v);

        QStringList nameList;
        for (auto name : req.names) {
            nameList.append(QString::fromStdString(name));
        }
        QString oneName = nameList.join(";");
        uint64_t total = req.size;

        res.id = req.id;
        res.names = req.names;
        res.flag = req.flag;
        res.size = total;
        response->json_msg = res.as_json().serialize();

        emit onTransCount(oneName, total);
    }
    default:
        DLOG << "unkown type: " << type;
        break;
    }

    response->id = request.id;
    response->mask = DO_SUCCESS;
}

bool SessionWorker::onStateChanged(int state, std::string &msg)
{
//    RPC_ERROR = -2,
//    RPC_DISCONNECTED = -1,
//    RPC_DISCONNECTING = 0,
//    RPC_CONNECTING = 1,
//    RPC_CONNECTED = 2,
    bool result = false;
    switch (state) {
    case RPC_CONNECTED: {
        _connectedAddress = QString(msg.c_str());
        DLOG << "connected remote: " << msg;
        result = true;
    }
    break;
    case RPC_DISCONNECTED: {
        if (msg.empty()) {
            DLOG << "first connect remote, retry...";
            return true;
        } else {
            DLOG << "disconnected remote: " << msg;
        }
    }
    break;
    case RPC_ERROR: {
        DLOG << "error remote code: " << msg;
        int code = std::stoi(msg);
        if (asio::error::host_unreachable == code) {
            DLOG << "ping failed: " << msg;
        }
    }
    break;
    default:
        DLOG << "other handling CONNECTING or DISCONNECTING: " << msg;
        break;
    }

    emit onConnectChanged(state, QString(msg.c_str()));

    return result;
}

void SessionWorker::setExtMessageHandler(ExtenMessageHandler cb)
{
    _extMsghandler = std::move(cb);
}

void SessionWorker::stop()
{
    if (_server) {
        // Stop the server
        _server->Stop();
    }

    if (_client) {
        _client->DisconnectAndStop();
    }
}

bool SessionWorker::startListen(int port)
{
    if (!listen(port)) {
        ELOG << "Fail to start local listen:" << port;
        return false;
    }

    return true;
}

bool SessionWorker::clientPing(QString &address, int port)
{
    bool hasConnected = false;
    if (_client && _client->hasConnected(address.toStdString())) {
        hasConnected = _client->IsConnected();
    }

    if (hasConnected)
        return true;

    return connect(address, port);
}

bool SessionWorker::connectRemote(QString ip, int port, QString password)
{
    if (!connect(ip, port)) {
        ELOG << "Fail to connect remote:" << ip.toStdString();
        return false;
    }

    std::hash<std::string> hasher;
    uint32_t pinHash = (uint32_t)hasher(password.toStdString());

    LoginMessage req;
    req.name = qApp->applicationName().toStdString();
    req.auth = std::to_string(pinHash);
    proto::OriginMessage request;
    request.mask = REQ_LOGIN;
    request.json_msg = req.as_json().serialize();

    auto response = sendRequest(ip, request);
    if (LOGIN_SUCCESS == response.mask) {
        _login_hosts.insert(ip, true);
    } else {
        _login_hosts.insert(ip, false);
    }
    return (LOGIN_SUCCESS == response.mask);
}

void SessionWorker::disconnectRemote()
{
    if (!_client) return;

    _client->DisconnectAsync();
}

proto::OriginMessage SessionWorker::sendRequest(const QString &target, const proto::OriginMessage &request)
{
    if (_client && _client->hasConnected(target.toStdString())) {
        return _client->sendRequest(request);
    }

    if (_server && _server->hasConnected(target.toStdString())) {
        return _server->sendRequest(target.toStdString(), request);
    }

    WLOG << "Not found connected session for: " << target.toStdString();
    proto::OriginMessage nullres;
    return nullres;
}

void SessionWorker::updatePincode(QString code)
{
    _savedPin = code;
}

bool SessionWorker::isClientLogin(QString &ip)
{
    bool foundValue = false;
    bool hasConnected = false;
    auto it = _login_hosts.find(ip);
    if (it != _login_hosts.end()) {
        foundValue = it.value();
    }

    if (_client && _client->hasConnected(ip.toStdString())) {
        hasConnected = _client->IsConnected();
    }

    return foundValue && hasConnected;
}

bool SessionWorker::listen(int port)
{
    auto asioService = _service.lock();
    if (!asioService) {
        WLOG << "weak asio service";
        return false;
    }

    // Create a new proto protocol server
    if (!_server) {
        _server = std::make_shared<ProtoServer>(asioService, port);
        _server->SetupReuseAddress(true);
        _server->SetupReusePort(true);

        auto self(this->shared_from_this());
        _server->setCallbacks(self);
    }

    // Start the server
    return _server->Start();
}

bool SessionWorker::connect(QString &address, int port)
{
    auto asioService = _service.lock();
    if (!asioService) {
        WLOG << "weak asio service";
        return false;
    }

    if (!_client) {
        _client = std::make_shared<ProtoClient>(asioService, address.toStdString(), port);

        auto self(this->shared_from_this());
        _client->setCallbacks(self);
    } else {
        if (_connectedAddress.compare(address) == 0) {
            LOG << "This target has been conntectd: " << address.toStdString();
            return _client->IsConnected() ? true : _client->ConnectAsync();
        } else {
            // different target, create new connection.
            _client->DisconnectAndStop();
            _client = std::make_shared<ProtoClient>(asioService, address.toStdString(), port);

            auto self(this->shared_from_this());
            _client->setCallbacks(self);
        }
    }
    _client->ConnectAsync();

    return _client->IsConnected();
}


// 递归函数来获取目录下所有文件的大小，包括子目录
size_t SessionWorker::getDirectorySize(const std::string& path) {
    size_t totalSize = 0;

    CppCommon::Directory dir = CppCommon::Directory(path);
    std::vector<CppCommon::Path> subEntrys;
    try {
        subEntrys = dir.GetEntriesRecursive();
    } catch (const CppCommon::FileSystemException& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
        return totalSize;
    }

    // 遍历目录
    for (int i = 0; i < subEntrys.size(); ++i) {
        const auto entry = subEntrys[i];
        if (entry.IsDirectory()) {
            // 递归处理子目录
            totalSize += getDirectorySize(entry.string());
        } else if (entry.IsRegularFile()) {
            try {
                CppCommon::File temp(entry);
                // 获取文件大小并累加到总大小中
                totalSize += temp.size(); // mask as file flag.
            } catch (const CppCommon::FileSystemException& ex) {
                std::cout << "Error: " << ex.what() << std::endl;
                return totalSize;
            }
        } else {
            std::cout << "skip the link file: " << entry.string() << std::endl;
        }
    }

    return totalSize;
}
