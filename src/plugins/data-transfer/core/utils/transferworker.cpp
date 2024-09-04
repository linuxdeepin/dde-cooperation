#include "transferhepler.h"
#include "transferworker.h"

#include "common/constant.h"
#include "common/commonstruct.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/backend.h"
#include "ipc/proto/chan.h"
#include "ipc/bridge.h"

#include <co/rpc.h>
#include <co/co.h>
#include <co/all.h>

#include <CuteIPCInterface.h>

#include <QTimer>
#include <QCoreApplication>
#include <QFile>

#pragma execution_character_set("utf-8")
TransferHandle::TransferHandle()
    : QObject()
{
    QString appName = QCoreApplication::applicationName();

    _backendOK = false;
    _request_job_id = appName.length();   // default start at appName's lenght
    _job_maps.clear();

    auto ipcInterface = TransferWoker::instance()->ipc();

    _backendOK = ipcInterface->connectToServer("cooperation-daemon");
    if (_backendOK) {
        // bind SIGNAL to SLOT
        ipcInterface->remoteConnect(SIGNAL(dataTransferSignal(int, QString)), this, SLOT(backendMessageSlot(int, QString)));

        QString sessionId;
        ipcInterface->call("bindSignal", Q_RETURN_ARG(QString, sessionId), Q_ARG(QString, appName), Q_ARG(QString, "dataTransferSignal"));

        DLOG << "ping return ID:" << sessionId.toStdString();
        TransferWoker::instance()->setSessionId(sessionId);
    } else {
        WLOG << "can not connect to: cooperation-daemon";
        TransferHelper::instance()->emitDisconnected();
    }

    connect(qApp, &QCoreApplication::aboutToQuit, [this]() {
        DLOG << "App exit, exit ipc server";
        cancelTransferJob();   //退出，取消job
        disconnectRemote();
        //FIXME: it always abort if invoke exit
    });
}

TransferHandle::~TransferHandle()
{
}

void TransferHandle::backendMessageSlot(int type, const QString& msg)
{
    co::Json json_obj = json::parse(msg.toStdString());
    if (json_obj.is_null()) {
        WLOG << "parse error from: " << msg.toStdString();
        return;
    }

    switch (type) {
    case IPC_PING: {
        //FIXME: ping?
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
        handleFileTransStatus(msg);
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

        break;
    }
    default:
        break;
    }

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
#ifndef WIN32
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

    if (miscJson.has_member("unfinish_json")) {
        // 前次迁移未完成信息
        QString undoneJsonstr = miscJson.get("unfinish_json").as_c_str();
        emit TransferHelper::instance()->unfinishedJob(undoneJsonstr);
    }

    if (miscJson.has_member("remaining_space")) {
        int remainSpace = miscJson.get("remaining_space").as_int();
        LOG << "remaining_space " << remainSpace << "G";
        emit TransferHelper::instance()->remoteRemainSpace(remainSpace);
    }

    if (miscJson.has_member("add_result")) {
        QString result = miscJson.get("add_result").as_c_str();
        LOG << "add_result" << result.data();
        for (QString str : result.split(";")) {
            auto res = str.split(" ");
            if (res.size() != 3)
                continue;
            emit TransferHelper::instance()->addResult(res.at(0), res.at(1) == "true", res.at(2));
        }
        emit TransferHelper::instance()->transferFinished();
    }

    if (miscJson.has_member("change_page")) {
        QString result = miscJson.get("change_page").as_c_str();
        LOG << "change_page" << result.data();
        if (!result.endsWith("_cb"))
            TransferHelper::instance()->sendMessage("change_page", result + "_cb");
        if (result.startsWith("startTransfer")) {
#ifdef linux
            emit TransferHelper::instance()->changeWidget(PageName::waitwidget);
#else
            emit TransferHelper::instance()->changeWidget(PageName::selectmainwidget);
#endif
            emit TransferHelper::instance()->clearWidget();
        }
    }

    if (miscJson.has_member("transfer_content")) {
        QString result = miscJson.get("transfer_content").as_c_str();
        for (QString str : result.split(";")) {
            auto res = str.split(" ");
            if (res.size() != 4)
                continue;
            emit TransferHelper::instance()->transferContent(res.at(0), res.at(1), res.at(2).toInt(), res.at(3).toInt());
        }
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
    emit TransferHelper::instance()->interruption();
    return TransferWoker::instance()->cancelTransferJob(jobid);
}

bool TransferHandle::isTransferring()
{
    return !_job_maps.isEmpty();
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
    ipcInterface = new CuteIPCInterface();
}

TransferWoker::~TransferWoker()
{
}

CuteIPCInterface *TransferWoker::ipc()
{
    return ipcInterface;
}

void TransferWoker::setEmptyPassWord()
{
    // set empty password, it will refresh password by random
    ipcInterface->call("setAuthPassword", Q_ARG(QString, ""));
}

QString TransferWoker::getConnectPassWord()
{
    QString password;
    ipcInterface->call("getAuthPassword", Q_RETURN_ARG(QString, password));
    return password;
}

bool TransferWoker::cancelTransferJob(int jobid)
{
    // TRANS_CANCEL 1008
    bool res =  false;
    ipcInterface->call("doOperateJob", Q_RETURN_ARG(bool, res), Q_ARG(int, 1008), Q_ARG(int, jobid), Q_ARG(QString, qApp->applicationName()));
    LOG << "cancelTransferJob result=" << res;
    return res;
}

void TransferWoker::tryConnect(const std::string &ip, const std::string &password)
{
    QString appname = qApp->applicationName();
    ipcInterface->call("doTryConnect", Q_ARG(QString, appname), Q_ARG(QString, appname),
                       Q_ARG(QString, QString::fromStdString(ip)), Q_ARG(QString, QString::fromStdString(password)));
}

void TransferWoker::disconnectRemote()
{
    ipcInterface->call("doDisShareCallback", Q_ARG(QString, qApp->applicationName()));
}

void TransferWoker::setSessionId(QString &seesionid)
{
    _session_id = seesionid;
}

void TransferWoker::sendFiles(int reqid, QStringList filepaths)
{
    QString target = qApp->applicationName();

    ipcInterface->call("doTransferFile", Q_ARG(QString, _session_id), Q_ARG(QString, target),
                       Q_ARG(int, reqid), Q_ARG(QStringList, filepaths), Q_ARG(bool, true),
                       Q_ARG(QString, ""));
}

void TransferWoker::sendMessage(json::Json &message)
{
    QString appname = qApp->applicationName();
    MiscJsonCall miscParam;
    miscParam.app = appname.toStdString();
    miscParam.json = message.str().c_str();

    QString jsonData = miscParam.as_json().str().c_str();

    ipcInterface->call("sendMiscMessage", Q_ARG(QString, appname), Q_ARG(QString, jsonData));
}
