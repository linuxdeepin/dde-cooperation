#include "transferhepler.h"
#include "transferworker.h"

#include "common/constant.h"
#include "common/commonutils.h"

#include "protoconstants.h"

#include "common/logger.h"


#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QHostInfo>
#include <QStorageInfo>
#include <QStandardPaths>

TransferStatus::TransferStatus()
    : QObject()
{

}

TransferStatus::~TransferStatus()
{
}

// return true -> cancel
bool TransferStatus::onProgress(const std::string &path, uint64_t current, uint64_t total)
{
//    LOG_IF(FLG_log_detail) << "progressbar: " << progressbar << " remain_time=" << remain_time;
//    LOG_IF(FLG_log_detail) << "all_total_size: " << _file_stats.all_total_size << " all_current_size=" << _file_stats.all_current_size;
//    emit TransferHelper::instance()->transferContent(tr("Transfering"), filepath, progressbar, remain_time);
    LOG << "path: " << path << " current=" << current << " total=" << total;

    return false;
}

void TransferStatus::onWebChanged(int state, std::string msg)
{

}


//-------------------------------------

TransferHandle::TransferHandle()
    : QObject()
{
    _this_destruct = false;

    if (!_statusRecorder) {
        _statusRecorder = std::make_shared<TransferStatus>();
    }

    connect(this, &TransferHandle::notifyTransData, this, &TransferHandle::handleDataDownload, Qt::QueuedConnection);
    connect(this, &TransferHandle::notifyCancelWeb, this, &TransferHandle::handleWebCancel, Qt::QueuedConnection);

    connect(qApp, &QCoreApplication::aboutToQuit, [this]() {
        DLOG << "App exit, exit ipc server";
        cancelTransferJob();   //退出，取消job
        disconnectRemote();
    });
}

TransferHandle::~TransferHandle()
{
    _this_destruct = true;

    if (!_server) {
        // Stop the server
        _server->Stop();
    }

    if (!_service) {
        // Stop the Asio service
        _service->Stop();
    }
}

void TransferHandle::onReceivedMessage(const proto::OriginMessage &request, proto::OriginMessage *response)
{
    if (request.json_msg.empty()) {
        DLOG << "empty json message: ";
        return;
    }

    // Step 2: 解析响应数据
    picojson::value v;
    std::string err = picojson::parse(v, request.json_msg);
    if (!err.empty()) {
        DLOG << "Failed to parse JSON data: " << err;
        return;
    }

//    picojson::array files = v.get("files").get<picojson::array>();
    int type = request.mask;
    switch (type) {
    case REQ_LOGIN: {
        LoginMessage req, res;
        req.from_json(v);
        DLOG << "Login: " << req.name << " " << req.auth;
        std::hash<std::string> hasher;
        uint32_t pinHash = (uint32_t)hasher(_pinCode.toStdString());
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

//        bool isdir = req.flag;
//        DLOG << "trans data: " << req.token << " " << isdir;
//        DLOG << "Names: ";
//        for (const auto &name : req.names) {
//            DLOG << name << " ";
//        }

        res.id = req.id;
        res.names = req.names;
        res.flag = true;
        res.size = 0;//TransferHelper::getRemainSize();
        response->json_msg = res.as_json().serialize();

        emit notifyTransData(req.names);
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

        emit notifyCancelWeb(req.id);
    }
    break;
    case CAST_INFO: {
    }
    break;
    default:
        DLOG << "unkown type: " << type;
        break;
    }

    response->id = request.id;
    response->mask = 1;
}

void TransferHandle::onStateChanged(int state, std::string msg)
{
//    RPC_ERROR = -2,
//    RPC_DISCONNECTED = -1,
//    RPC_DISCONNECTING = 0,
//    RPC_CONNECTING = 1,
//    RPC_CONNECTED = 2,
    switch (state) {
    case RPC_CONNECTED: {
        _connectedAddress = QString(msg.c_str());
        DLOG << "connected remote: " << msg;
        // update save dir
        _saveDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + QDir::separator() + _connectedAddress;
    }
    break;
    case RPC_DISCONNECTED: {
        DLOG << "disconnected remote: " << msg;
    }
    break;
    case RPC_ERROR: {
        DLOG << "error remote: " << msg;
    }
    break;
    default:
        DLOG << "other handling CONNECTING or DISCONNECTING: " << msg;
        break;
    }
}

void TransferHandle::init()
{
    // Create a new Asio service
    _service = std::make_shared<AsioService>();

    // Start the Asio service
    _service->Start();

    // Create a new proto protocol server
    if (!_server) {
        _server = std::make_shared<ProtoServer>(_service, SESSION_TCP_PORT);
        _server->SetupReuseAddress(true);
        _server->SetupReusePort(true);

        auto self(this->shared_from_this());
        _server->setCallbacks(self);
    }

    // Start the server
    _server->Start();
}

void TransferHandle::connectRemote(QString &address)
{
    if (!_client) {
        _client = std::make_shared<ProtoClient>(_service, address.toStdString(), SESSION_TCP_PORT);
        _client->ConnectAsync();
    } else {
        _client->IsConnected() ? _client->ReconnectAsync() : _client->ConnectAsync();
    }
}

bool TransferHandle::startFileWeb()
{
    // Create a new file http server
    if (!_file_server) {
        _file_server = std::make_shared<FileServer>(_service, WEB_TCP_PORT);
//        std::shared_ptr<TransferStatus> recorder = std::make_shared<TransferStatus>();;
        if (_statusRecorder) {
            _file_server->setCallback(_statusRecorder);
        }
    }

    return _file_server->start();
}

bool TransferHandle::handleDataDownload(const std::vector<std::string> nameVector)
{
    // Create a new file http client
    if (!_file_client) {
        _file_client = std::make_shared<FileClient>(_service, _connectedAddress.toStdString(), WEB_TCP_PORT); //service, address, port
        if (_statusRecorder) {
            _file_client->setCallback(_statusRecorder);
        }
    }

    _file_client->cancel(false);
    _file_client->setConfig(_accessToken.toStdString(), _saveDir.toStdString());
    DLOG << "Download Names: ";
    for (const auto name : nameVector) {
        DLOG << name;
        _file_client->downloadFolder(name);
    }
    return true;
}

void TransferHandle::handleWebCancel(const std::string jobid)
{
    if (_file_client && _file_client->downloading()) {
        _file_client->cancel();
    }

    if (_file_server) {
        _file_server->clearBind();
        _file_server->clearToken();
        _file_server->stop();
    }
}

void TransferHandle::handleConnectStatus(int result, QString msg)
{
    LOG << "connect status: " << result << " msg:" << msg.toStdString();
    if (result > 0) {
        emit TransferHelper::instance()->connectSucceed();
#ifdef __linux__
//        json::Json message;
//        QString unfinishJson;
//        QString ip = msg.split(" ").first();
//        TransferHelper::instance()->setConnectIP(ip);
//        int remainSpace = TransferHelper::getRemainSize();
//        bool unfinish = TransferHelper::instance()->isUnfinishedJob(unfinishJson);
//        if (unfinish) {
//            message.add_member("unfinish_json", unfinishJson.toStdString());
//        }
//        message.add_member("remaining_space", remainSpace);
//        sendMessage(message);
#endif
    } else {
        emit TransferHelper::instance()->connectFailed();
    }
}

void TransferHandle::handleTransJobStatus(int id, int result, QString path)
{
    auto it = _job_maps.find(id);
    DLOG << "handleTransJobStatus " << result << " saved:" << path.toStdString();

    switch (result) {
    case JOB_TRANS_FAILED:
        // remove job from maps
        if (it != _job_maps.end()) {
            _job_maps.erase(it);
        }
        LOG << "Send job failed: (" << id << ") " << path.toStdString();
        emit TransferHelper::instance()->interruption();
        TransferHelper::instance()->emitDisconnected();
        break;
    case JOB_TRANS_DOING:
        _job_maps.insert(id, path);
        emit TransferHelper::instance()->transferring();
#ifdef __linux__
        QFile::remove(path + "/" + "transfer.json");
#endif
        break;
    case JOB_TRANS_FINISHED:
        // remove job from maps
        if (it != _job_maps.end()) {
            _job_maps.erase(it);
        }
#ifdef __linux__
        TransferHelper::instance()->setting(path);
#endif
        break;
    case JOB_TRANS_CANCELED:
        _job_maps.remove(id);
        emit TransferHelper::instance()->interruption();
        TransferHelper::instance()->emitDisconnected();
        break;
    default:
        break;
    }
}

void TransferHandle::handleFileTransStatus(QString statusstr)
{
    // FileStatus
    //    LOG << "handleFileTransStatus: " << statusstr;
//    co::Json status_json;
//    status_json.parse_from(statusstr.toStdString());
//    ipc::FileStatus param;
//    param.from_json(status_json);

//    QString filepath(param.name.c_str());

//    _file_stats.all_total_size = param.total;

//    switch (param.status) {
//    case FILE_TRANS_IDLE: {
//        _file_stats.all_total_size = param.total;
//        _file_stats.all_current_size = param.current;
//        LOG_IF(FLG_log_detail) << "file receive IDLE: " << filepath.toStdString() << " total: " << param.total;
//        break;
//    }
//    case FILE_TRANS_SPEED: {
//        int64_t increment = (param.current - _file_stats.all_current_size) * 1000;   // 转换为毫秒速度
//        int64_t time_spend = param.millisec - _file_stats.cast_time_ms;
//        if (time_spend > 0) {
//            float speed = increment / 1024 / time_spend;
//            if (speed > 1024) {
//                LOG << filepath.toStdString() << " SPEED: " << speed / 1024 << " MB/s";
//            } else {
//                LOG << filepath.toStdString() << " SPEED: " << speed << " KB/s";
//            }
//        }
//        break;
//    }
//    case FILE_TRANS_END: {
//        LOG_IF(FLG_log_detail) << "file receive END: " << filepath.toStdString();
//#ifdef __linux__
//        TransferHelper::instance()->addFinshedFiles(filepath, param.total);
//#endif

//        break;
//    }
//    default:
//        LOG << "unhandle status: " << param.status;
//        break;
//    }
//    _file_stats.all_current_size = param.current;
//    _file_stats.cast_time_ms = param.millisec;

//    // 计算整体进度和预估剩余时间
//    double value = static_cast<double>(_file_stats.all_current_size) / _file_stats.all_total_size;
//    int progressbar = static_cast<int>(value * 100);
//    int64 remain_time;
//    if (progressbar <= 0) {
//        remain_time = -1;
//    } else if (progressbar > 100) {
//        progressbar = 100;
//        remain_time = 0;
//    } else {
//        // 转为秒
//        remain_time = (_file_stats.cast_time_ms * 100 / progressbar - _file_stats.cast_time_ms) / 1000;
//    }

//    LOG_IF(FLG_log_detail) << "progressbar: " << progressbar << " remain_time=" << remain_time;
//    LOG_IF(FLG_log_detail) << "all_total_size: " << _file_stats.all_total_size << " all_current_size=" << _file_stats.all_current_size;
//    emit TransferHelper::instance()->transferContent(tr("Transfering"), filepath, progressbar, remain_time);
}

bool TransferHandle::tryConnect(QString ip, QString password)
{
    connectRemote(ip);
    if (!_client) return true;

    std::hash<std::string> hasher;
    uint32_t pinHash = (uint32_t)hasher(password.toStdString());

    LoginMessage req, res;
    req.name = qApp->applicationName().toStdString();
    req.auth = std::to_string(pinHash);
    proto::OriginMessage request;
    request.json_msg = req.as_json().serialize();

    auto response = _client->request(request).get();
    if (LOGIN_SUCCESS == response.mask) {
        emit TransferHelper::instance()->connectSucceed();
        return true;
    }
    return false;
}

void TransferHandle::disconnectRemote()
{
    if (!_client) return;

    _client->DisconnectAsync();
}

QString TransferHandle::getConnectPassWord()
{
    _pinCode = deepin_cross::CommonUitls::generateRandomPassword();
    DLOG << "getConnectPassWord :" << _pinCode.toStdString();
    return _pinCode;
}

bool TransferHandle::cancelTransferJob()
{
    if(_job_maps.isEmpty()) return true;

    TransCancelMessage req;
    int jobid = _job_maps.firstKey();
    req.id = std::to_string(jobid);
    req.name = _job_maps.value(jobid).toStdString();
    req.reason = "User canceled";
    proto::OriginMessage request;
    request.json_msg = req.as_json().serialize();

    auto response = _client->request(request).get();
    if (DO_SUCCESS == response.mask) {
        emit TransferHelper::instance()->interruption();
        return true;
    }
    return false;
}

bool TransferHandle::isTransferring()
{
    return !_job_maps.isEmpty();
}

void TransferHandle::sendFiles(QStringList paths)
{
    // first try run web, or prompt error
    if (!startFileWeb()) {
        ELOG << "try start web sever failed!!!";
        return;
    }

    _file_server->clearBind();
    std::vector<std::string> name_vector;
    for (auto path : paths) {
        QFileInfo fileInfo(path);
        QString name = fileInfo.fileName();
        name_vector.push_back(name.toStdString());
        _file_server->webBind(name.toStdString(), path.toStdString());
    }

    TransDataMessage req;
    req.id = std::to_string(_request_job_id);
    req.names = name_vector;
    req.token = "gen-temp-token";
    req.flag = true; // many folders
    req.size = -1; // unkown size
    proto::OriginMessage request;
    request.json_msg = req.as_json().serialize();

    auto response = _client->request(request).get();
    if (DO_SUCCESS == response.mask) {
        _request_job_id++;
    } else {
        // emit error!
        _file_server->stop();
    }
}
