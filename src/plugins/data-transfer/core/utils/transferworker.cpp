#include "transferhepler.h"
#include "transferworker.h"

#include "common/constant.h"
#include "common/commonstruct.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/backend.h"
#include "ipc/proto/chan.h"

#include <co/rpc.h>
#include <co/co.h>
#include <co/all.h>

#include <QTimer>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>

#pragma execution_character_set("utf-8")
TransferHandle::TransferHandle()
    : QObject()
{
    _this_destruct = false;
    localIPCStart();

    QString appName = QCoreApplication::applicationName();

    _backendOK = false;
    _request_job_id = appName.length();   // default start at appName's lenght
    _job_maps.clear();

    _backendOK = TransferWoker::instance()->pingBackend(appName.toStdString());
    if (_backendOK) {
        saveSession(TransferWoker::instance()->getSessionId());
    }

    QTimer *timer = new QTimer();

    // 连接定时器的 timeout 信号到槽函数
    ipcPing = 3;
    connect(timer, &QTimer::timeout, [this, appName]() {
        ipcPing--;
        if (ipcPing <= 0) {
            _backendOK = TransferWoker::instance()->pingBackend(appName.toStdString());
            if (_backendOK) {
                ipcPing = 3;
                saveSession(TransferWoker::instance()->getSessionId());
            } else {
                //后端离线，跳转提示界面.
                // FIXME: 只有从等待传输界面开始才能跳转
                TransferHelper::instance()->emitDisconnected();
            }
        }
    });
    timer->start(1000);
}

TransferHandle::~TransferHandle()
{
    _this_destruct = true;
}

void TransferHandle::localIPCStart()
{
    if (_frontendIpcService) return;

    _frontendIpcService = new FrontendService(this);

    UNIGO([this]() {
        while (!_this_destruct) {
            BridgeJsonData bridge;
            _frontendIpcService->bridgeChan()->operator>>(bridge);   //300ms超时
            if (!_frontendIpcService->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

            //            LOG << "TransferHandle get bridge json: " << bridge.type << " json:" << bridge.json;
            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                ELOG << "parse error from: " << bridge.json;
                continue;
            }
            switch (bridge.type) {
            case IPC_PING: {
                ipcPing = 3;
                ipc::PingFrontParam param;
                param.from_json(json_obj);

                bool result = false;
                fastring my_ver(FRONTEND_PROTO_VERSION);
                if (my_ver.compare(param.version) == 0 && (param.session.compare(_sessionid) == 0 || param.session.compare("backendServerOnline") == 0)) {
                    result = true;
                } else {
                    DLOG << param.version << " =version not match= " << my_ver;
                }

                BridgeJsonData res;
                res.type = IPC_PING;
                res.json = result ? param.session : "";   // 成功则返回session，否则为空

                _frontendIpcService->bridgeResult()->operator<<(res);
                break;
            }
            case MISC_MSG: {
                QString json(json_obj.get("msg").as_c_str());
                handleMiscMessage(json);
                break;
            }
            case FRONT_PEER_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                // example to parse string to NodePeerInfo object
                NodePeerInfo peerobj;
                peerobj.from_json(param.msg);

                //LOG << param.result << " peer : " << param.msg.c_str();

                break;
            }
            case FRONT_CONNECT_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString mesg(param.msg.c_str());
                LOG << param.result << " FRONT_CONNECT_CB : " << param.msg.c_str();
                handleConnectStatus(param.result, mesg);
                break;
            }
            case FRONT_TRANS_STATUS_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString mesg(param.msg.c_str());   // job path

                mesg = mesg.replace("::not enough", "");
                handleTransJobStatus(param.id, param.result, mesg);
                break;
            }
            case FRONT_FS_PULL_CB: {
                break;
            }
            case FRONT_FS_ACTION_CB: {
                break;
            }
            case FRONT_NOTIFY_FILE_STATUS: {
                QString objstr(bridge.json.c_str());
                handleFileTransStatus(objstr);
                break;
            }
            case FRONT_SEND_STATUS: {
                SendStatus param;
                param.from_json(json_obj);
                if (REMOTE_CLIENT_OFFLINE == param.status) {
                    cancelTransferJob();   //离线，取消job
                    TransferHelper::instance()->emitDisconnected();
                }
                break;
            }
            case FRONT_DISCONNECT_CB: {
                TransferHelper::instance()->emitDisconnected();
                break;
            }
            case FRONT_SERVER_ONLINE: {
                QString appName = QCoreApplication::applicationName();
                _backendOK = TransferWoker::instance()->pingBackend(appName.toStdString());
                if (_backendOK) {
                    saveSession(TransferWoker::instance()->getSessionId());
                }
                break;
            }
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(_frontendIpcService);
    _rpcServer = new rpc::Server();
    _rpcServer->add_service(frontendimp);
    _rpcServer->start("0.0.0.0", UNI_IPC_FRONTEND_PORT, "/frontend", "", "");

    connect(qApp, &QCoreApplication::aboutToQuit, [this]() {
        DLOG << "App exit, exit ipc server";
        cancelTransferJob();   //退出，取消job
        emit TransferHelper::instance()->interruption();
        disconnectRemote();
        //FIXME: it always abort if invoke exit
    });
}

void TransferHandle::saveSession(fastring sessionid)
{
    _sessionid = sessionid;
}

void TransferHandle::handleConnectStatus(int result, QString msg)
{
    LOG << "connect status: " << result << " msg:" << msg.toStdString();
    if (result > 0) {
        emit TransferHelper::instance()->connectSucceed();
#ifndef WIN32
        json::Json message;
        QString unfinishJson;
        QString ip = msg.split(" ").first();
        TransferHelper::instance()->setConnectIP(ip);
        int remainSpace = TransferHelper::getRemainSize();
        bool unfinish = TransferHelper::instance()->isUnfinishedJob(unfinishJson);
        if (unfinish) {
            message.add_member("unfinish_json", unfinishJson.toStdString());
        }
        message.add_member("remaining_space", remainSpace);
        sendMessage(message);
#endif
    } else {
        emit TransferHelper::instance()->connectFailed();
    }
}

void TransferHandle::handleTransJobStatus(int id, int result, QString path)
{
    auto it = _job_maps.find(id);
    LOG_IF(FLG_log_detail) << "handleTransJobStatus " << result << " saved:" << path.toStdString();

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
#ifndef WIN32
        QFile::remove(path + "/" + "transfer.json");
#endif
        break;
    case JOB_TRANS_FINISHED:
        // remove job from maps
        if (it != _job_maps.end()) {
            _job_maps.erase(it);
        }
#ifdef WIN32
        emit TransferHelper::instance()->transferFinished();
#else
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
    co::Json status_json;
    status_json.parse_from(statusstr.toStdString());
    ipc::FileStatus param;
    param.from_json(status_json);

    QString filepath(param.name.c_str());

    _file_stats.all_total_size = param.total;

    switch (param.status) {
    case FILE_TRANS_IDLE: {
        _file_stats.all_total_size = param.total;
        _file_stats.all_current_size = param.current;
        LOG_IF(FLG_log_detail) << "file receive IDLE: " << filepath.toStdString() << " total: " << param.total;
        break;
    }
    case FILE_TRANS_SPEED: {
        int64_t increment = (param.current - _file_stats.all_current_size) * 1000;   // 转换为毫秒速度
        int64_t time_spend = param.millisec - _file_stats.cast_time_ms;
        if (time_spend > 0) {
            float speed = increment / 1024 / time_spend;
            if (speed > 1024) {
                LOG << filepath.toStdString() << " SPEED: " << speed / 1024 << " MB/s";
            } else {
                LOG << filepath.toStdString() << " SPEED: " << speed << " KB/s";
            }
        }
        break;
    }
    case FILE_TRANS_END: {
        LOG_IF(FLG_log_detail) << "file receive END: " << filepath.toStdString();
#ifndef WIN32
        TransferHelper::instance()->addFinshedFiles(filepath, param.total);
#endif

        break;
    }
    default:
        LOG << "unhandle status: " << param.status;
        break;
    }
    _file_stats.all_current_size = param.current;
    _file_stats.cast_time_ms = param.millisec;

    // 计算整体进度和预估剩余时间
    double value = static_cast<double>(_file_stats.all_current_size) / _file_stats.all_total_size;
    int progressbar = static_cast<int>(value * 100);
    int64 remain_time;
    if (progressbar <= 0) {
        remain_time = -1;
    } else if (progressbar > 100) {
        progressbar = 100;
        remain_time = 0;
    } else {
        // 转为秒
        remain_time = (_file_stats.cast_time_ms * 100 / progressbar - _file_stats.cast_time_ms) / 1000;
    }

    LOG_IF(FLG_log_detail) << "progressbar: " << progressbar << " remain_time=" << remain_time;
    LOG_IF(FLG_log_detail) << "all_total_size: " << _file_stats.all_total_size << " all_current_size=" << _file_stats.all_current_size;

    emit TransferHelper::instance()->transferContent(tr("Transfering"), filepath, progressbar, remain_time);
}

void TransferHandle::handleMiscMessage(QString jsonmsg)
{
    //LOG << "misc message arrived:" << jsonmsg.toStdString();
    co::Json miscJson;
    if (!miscJson.parse_from(jsonmsg.toStdString())) {
        DLOG << "error json format string!";
        return;
    }

    // 前次迁移未完成信息
    if (miscJson.has_member("unfinish_json")) {
        QString undoneJsonstr = miscJson.get("unfinish_json").as_c_str();
        // 弹出前次迁移未完成对话框
        emit TransferHelper::instance()->unfinishedJob(undoneJsonstr);
    }

    if (miscJson.has_member("remaining_space")) {
        int remainSpace = miscJson.get("remaining_space").as_int();
        LOG << "remaining_space " << remainSpace << "G";
        emit TransferHelper::instance()->remoteRemainSpace(remainSpace);
    }
}

void TransferHandle::tryConnect(QString ip, QString password)
{
    if (!_backendOK) return;

    TransferWoker::instance()->tryConnect(ip.toStdString(), password.toStdString());
}

void TransferHandle::disconnectRemote()
{
    if (!_backendOK) return;

    TransferWoker::instance()->disconnectRemote();
}

QString TransferHandle::getConnectPassWord()
{
    if (!_backendOK) return "";

    QString password;
    TransferWoker::instance()->setEmptyPassWord();
    password = TransferWoker::instance()->getConnectPassWord();
    return password;
}

bool TransferHandle::cancelTransferJob()
{
    if (!_backendOK || _job_maps.isEmpty()) return false;

    int jobid = _job_maps.firstKey();
    _job_maps.remove(jobid);
    return TransferWoker::instance()->cancelTransferJob(jobid);
}

void TransferHandle::sendFiles(QStringList paths)
{
    if (!_backendOK) return;

    TransferWoker::instance()->sendFiles(_request_job_id, paths);
    _job_maps.insert(_request_job_id, paths.first());
    _request_job_id++;
}

void TransferHandle::sendMessage(json::Json &message)
{
    if (!_backendOK) return;

    TransferWoker::instance()->sendMessage(message);
}

TransferWoker::TransferWoker()
{
    // initialize the proto client
    coClient = std::shared_ptr<rpc::Client>(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_DATA_TRAN_PORT, false));
}

TransferWoker::~TransferWoker()
{
    coClient->close();
}

bool TransferWoker::pingBackend(const std::string &who)
{
    co::Json req, res;
    //PingBackParam
    req = {
        { "who", who },
        { "version", UNI_IPC_PROTO },
        { "cb_port", UNI_IPC_FRONTEND_PORT },
    };

    req.add_member("api", "Backend.ping");   //BackendImpl::ping

    call(req, res);
    _session_id = res.get("msg").as_string();   // save the return session.

    //CallResult
    return res.get("result").as_bool() && !_session_id.empty();
}

void TransferWoker::setEmptyPassWord()
{
    // set empty password, it will refresh password by random
    co::Json req, res;
    req = {
        { "password", "" },
    };

    req.add_member("api", "Backend.setPassword");   //BackendImpl::setPassword

    call(req, res);
}

QString TransferWoker::getConnectPassWord()
{
    co::Json req, res;

    req.add_member("api", "Backend.getPassword");   //BackendImpl::getPassword

    call(req, res);

    return res.get("password").as_string().c_str();
}

bool TransferWoker::cancelTransferJob(int jobid)
{
    co::Json req, res;

    ipc::TransJobParam jobParam;
    jobParam.session = _session_id;
    jobParam.job_id = jobid;
    jobParam.appname = qApp->applicationName().toStdString();

    req = jobParam.as_json();
    req.add_member("api", "Backend.cancelTransJob");   //BackendImpl::cancelTransJob
    call(req, res);
    LOG << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
    return res.get("result").as_bool();
}

void TransferWoker::tryConnect(const std::string &ip, const std::string &password)
{
    co::Json req, res;
    fastring target_ip(ip);
    fastring pin_code(password);
    fastring app_name(qApp->applicationName().toStdString());

    ipc::ConnectParam conParam;
    conParam.appName = app_name;
    conParam.host = target_ip;
    conParam.password = pin_code;
    conParam.targetAppname = app_name;

    req = conParam.as_json();
    req.add_member("api", "Backend.tryConnect");
    call(req, res);
}

void TransferWoker::disconnectRemote()
{
    co::Json req, res;
    fastring app_name(qApp->applicationName().toStdString());

    ShareDisConnect info;
    info.tarAppname = app_name;

    req = info.as_json();
    req.add_member("api", "Backend.disconnectCb");   //BackendImpl::disConnect
    LOG << "disConnect" << req.str().c_str();
    call(req, res);
}

fastring TransferWoker::getSessionId()
{
    return _session_id;
}

void TransferWoker::sendFiles(int reqid, QStringList filepaths)
{
    co::Json req, res;

    co::vector<fastring> fileVector;
    for (QString path : filepaths) {
        fileVector.push_back(path.toStdString());
    }
    fastring app_name(qApp->applicationName().toStdString());

    ipc::TransFilesParam transParam;
    transParam.session = _session_id;
    transParam.targetSession = app_name;
    transParam.id = reqid;
    transParam.paths = fileVector;
    transParam.sub = true;
    transParam.savedir = "";

    req = transParam.as_json();
    req.add_member("api", "Backend.tryTransFiles");   //BackendImpl::tryTransFiles

    call(req, res);
}

void TransferWoker::sendMessage(json::Json &message)
{
    co::Json req, res;
    fastring app_name(qApp->applicationName().toStdString());

    MiscJsonCall miscParam;
    miscParam.app = app_name;
    miscParam.json = message.str().c_str();
    req = miscParam.as_json();
    req.add_member("api", "Backend.miscMessage");   //BackendImpl::miscMessage

    LOG << "sendMessage" << req.str().c_str();
    call(req, res);
}

void TransferWoker::call(const json::Json &req, json::Json &res)
{
    coClient.reset(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_DATA_TRAN_PORT, false));
#if defined(WIN32)
    co::wait_group wg;
    wg.add(1);
    UNIGO([this, &req, &res, wg]() {
        coClient->call(req, res);
        wg.done();
    });
    wg.wait();
#else
    coClient->call(req, res);
#endif
    coClient->close();
}
