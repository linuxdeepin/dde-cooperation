#include "transferhepler.h"
#include "transferworker.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"

#include <co/rpc.h>
#include <co/co.h>
#include <co/all.h>

#include <QTimer>
#include <QDebug>
#include <QCoreApplication>

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
    _file_ids.clear();

    _backendOK = TransferWoker::instance()->pingBackend(appName.toStdString());
    if (_backendOK) {
        saveSession(TransferWoker::instance()->getSessionId());
    }

    //log
    qInstallMessageHandler(logHandler);
}

TransferHandle::~TransferHandle()
{
    _this_destruct = true;
}

void TransferHandle::logHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
        DLOG << msg.toStdString();
        break;
    case QtInfoMsg:
        LOG << msg.toStdString();
        break;
    case QtWarningMsg:
        WLOG << msg.toStdString();
        break;
    default:
        DLOG << msg.toStdString();
        break;
    }
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
            case PING: {
                ipc::PingFrontParam param;
                param.from_json(json_obj);

                bool result = false;
                fastring my_ver(FRONTEND_PROTO_VERSION);
                if (my_ver.compare(param.version) == 0 &&
                        (param.session.compare(_sessionid) == 0 ||
                         param.session.compare("backendServerOnline") == 0 )) {
                    result = true;
                } else {
                    DLOG << param.version << " =version not match= " << my_ver;
                }

                BridgeJsonData res;
                res.type = PING;
                res.json = result ? param.session : "";   // 成功则返回session，否则为空

                _frontendIpcService->bridgeResult()->operator<<(res);
                break;
            }
            case MISC_MSG: {
                QString json(bridge.json.c_str());
                handleMiscMessage(json);
                break;
            }
            case FRONT_PEER_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                // example to parse string to NodePeerInfo object
                NodePeerInfo peerobj;
                peerobj.from_json(param.msg);

                qInfo() << param.result << " peer : " << param.msg.c_str();

                break;
            }
            case FRONT_CONNECT_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString mesg(param.msg.c_str());
                handleConnectStatus(param.result, mesg);
                break;
            }
            case FRONT_TRANS_STATUS_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString mesg(param.msg.c_str());   // job path

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
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(_frontendIpcService);
    rpc::Server().add_service(frontendimp).start("0.0.0.0", UNI_IPC_FRONTEND_PORT, "/frontend", "", "");
}

void TransferHandle::saveSession(fastring sessionid)
{
    _sessionid = sessionid;
}

void TransferHandle::handleConnectStatus(int result, QString msg)
{
    qInfo() << "connect status: " << result << " msg:" << msg;
    if (result > 0) {
        emit TransferHelper::instance()->connectSucceed();

        //#ifndef WIN32
        //        TransferHelper::instance()->isUnfinishedJob(msg);
        //#endif
    }
}

void TransferHandle::handleTransJobStatus(int id, int result, QString path)
{
    auto it = _job_maps.find(id);
    qInfo() << "handleTransJobStatus " << result << " saved:" << path;

    switch (result) {
    case JOB_TRANS_FAILED:
        // remove job from maps
        if (it != _job_maps.end()) {
            _job_maps.erase(it);
        }
        qInfo() << "Send job failed: (" << id << ") " << path;
        break;
    case JOB_TRANS_DOING:
        _job_maps.insert(id, path);
        emit TransferHelper::instance()->transferring();
        break;
    case JOB_TRANS_FINISHED:
        // remove job from maps
        if (it != _job_maps.end()) {
            _job_maps.erase(it);
        }
#ifdef WIN32
        emit TransferHelper::instance()->transferSucceed(true);
#else
        TransferHelper::instance()->setting(path);
#endif
        break;
    default:
        break;
    }
}

void TransferHandle::handleFileTransStatus(QString statusstr)
{
    // FileStatus
    //    qInfo() << "handleFileTransStatus: " << statusstr;
    co::Json status_json;
    status_json.parse_from(statusstr.toStdString());
    ipc::FileStatus param;
    param.from_json(status_json);

    QString filepath(param.name.c_str());

    switch (param.status) {
    case FILE_TRANS_IDLE: {
        // 这个文件未被统计
        _file_stats.all_total_size += param.total;
        _file_stats.all_current_size += param.current;
        _file_ids.insert(param.file_id, param.current);

        qInfo() << "file receive IDLE: " << filepath;
        break;
    }
    case FILE_TRANS_SPEED: {
        if (_file_ids.contains(param.file_id)) {
            // 已经记录过，只更新数据
            int64_t increment = param.current - _file_ids[param.file_id];
            //        qInfo() << "_file_ids " << param.file_id << " increment: " << increment;
            _file_stats.all_current_size += increment;   //增量值
            _file_ids[param.file_id] = param.current;

            if (param.current >= param.total) {
                // 此文件已完成，从文件统计中删除
                _file_ids.remove(param.file_id);
            }
        }
        float speed = param.current / 1024 / param.second;
        if (speed > 1024) {
            qInfo() << filepath << "SPEED: " << speed / 1024 << "MB/s";
        } else {
            qInfo() << filepath << "SPEED: " << speed << "KB/s";
        }
        break;
    }
    case FILE_TRANS_END: {
        // 此文件已完成，从文件统计中删除
        int64_t increment = param.current - _file_ids[param.file_id];
        _file_stats.all_current_size += increment;   //增量值
        _file_ids.remove(param.file_id);

        qInfo() << "file receive END: " << filepath;
        //TransferHelper::instance()->addFinshedFiles(filepath);
        break;
    }
    default:
        qInfo() << "unhandle status: " << param.status;
        break;
    }

    if (param.second > _file_stats.max_time_sec) {
        _file_stats.max_time_sec = param.second;
    }

    // 全部file_id的all_total_size, all_current/all_total_size
    double value = static_cast<double>(_file_stats.all_current_size) / _file_stats.all_total_size;
    int progressbar = static_cast<int>(value * 100);
    int remain_time;
    if (progressbar <= 0) {
        remain_time = -1;
    } else if (progressbar > 100) {
        progressbar = 100;
        remain_time = 0;
    } else {
        remain_time = _file_stats.max_time_sec * 100 / progressbar - _file_stats.max_time_sec;
    }

    //    qInfo() << "progressbar: " << progressbar << " remain_time=" << remain_time;
    //    qInfo() << "all_total_size: " << _file_stats.all_total_size << " all_current_size=" << _file_stats.all_current_size;

    emit TransferHelper::instance()->transferContent("正在传输", filepath, progressbar, remain_time);
}

void TransferHandle::handleMiscMessage(QString jsonmsg)
{
    qInfo() << "misc message arrived:" << jsonmsg;
}

void TransferHandle::tryConnect(QString ip, QString password)
{
    if (!_backendOK) return;

    TransferWoker::instance()->tryConnect(ip.toStdString(), password.toStdString());
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
    if (!_backendOK) return false;

    co::wait_group g_wg;
    g_wg.add(1);
    bool ok;
    int jobid = _job_maps.firstKey();
    UNIGO([&ok, g_wg, jobid]() {
        ok = TransferWoker::instance()->cancelTransferJob(jobid);
        g_wg.done();
    });
    g_wg.wait();
    return ok;
}

void TransferHandle::sendFiles(QStringList paths)
{
    if (!_backendOK) return;

    //清空上次任务的所有文件统计
    _file_ids.clear();
    int current_id = _request_job_id;
    TransferWoker::instance()->sendFiles(current_id, paths);
    current_id++;
}

TransferWoker::TransferWoker()
{
    // initialize the proto client
    coClient = std::shared_ptr<rpc::Client>(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false));
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

    req = {
        { "session", _session_id },
        { "job_id", jobid },
        { "is_remote", true }
    };
    req.add_member("api", "Backend.cancelTransJob");   //BackendImpl::cancelTransJob
    coClient->call(req, res);
    qInfo() << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
    return res.get("result").as_bool();
}

void TransferWoker::tryConnect(const std::string &ip, const std::string &password)
{
    co::Json req, res;
    fastring target_ip(ip);
    fastring pin_code(password);
    QString appName = QCoreApplication::applicationName();

    req = {
        { "session", appName.toStdString() },
        { "host", target_ip },
        { "password", pin_code },
    };
    req.add_member("api", "Backend.tryConnect");   //BackendImpl::tryConnect
    call(req, res);
}

fastring TransferWoker::getSessionId()
{
    return _session_id;
}

void TransferWoker::sendFiles(int reqid, QStringList filepaths)
{
    co::Json req, res, paths;

    for (QString path : filepaths) {
        paths.push_back(path.toStdString());
    }

    //TransFilesParam
    req = {
        { "session", _session_id },
        { "id", reqid },
        { "paths", paths },
        { "sub", true },
        { "savedir", "" },
    };

    req.add_member("api", "Backend.tryTransFiles");   //BackendImpl::tryTransFiles

    call(req, res);
}

void TransferWoker::call(const json::Json &req, json::Json &res)
{
    coClient.reset(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false));
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
