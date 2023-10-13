#include "transferhepler.h"
#include "transferworker.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"

#include <co/rpc.h>
#include <co/co.h>
#include <co/all.h>

#include <QTimer>
#include <QDebug>

TransferHandle::TransferHandle()
    : QObject()
{
    _frontendIpcService = new FrontendService(this);

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(_frontendIpcService);
    rpc::Server().add_service(frontendimp)
                 .start("0.0.0.0", UNI_IPC_FRONTEND_PORT, "/frontend", "", "");

    connect(_frontendIpcService, &FrontendService::sigSession, this, &TransferHandle::saveSession, Qt::QueuedConnection);
    connect(_frontendIpcService, &FrontendService::sigConnectStatus, this, &TransferHandle::handleConnectStatus, Qt::QueuedConnection);
    connect(_frontendIpcService, &FrontendService::sigTransJobtatus, this, &TransferHandle::handleTransJobStatus, Qt::QueuedConnection);
    connect(_frontendIpcService, &FrontendService::sigFileTransStatus, this, &TransferHandle::handleFileTransStatus, Qt::QueuedConnection);

    _backendOK = false;

    go([this]() {
        _backendOK = TransferWoker::instance()->pingBackend();
    });
}

TransferHandle::~TransferHandle()
{
}

void TransferHandle::saveSession(QString sessionid)
{
    _sessionid = sessionid;
}

void TransferHandle::handleConnectStatus(int result, QString msg)
{
    qInfo() << "connect status: " << result << " msg:" << msg;
    if (result > 0) {
        emit TransferHelper::instance()->connectSucceed();
    }
}

void TransferHandle::handleTransJobStatus(int id, int result, QString msg)
{
    switch (result)
    {
    case -1:
        qInfo() << "Send job failed: (" << id << ") " << msg;
        break;
    case 0:
        emit TransferHelper::instance()->transferring();
        break;
    case 1:
        emit TransferHelper::instance()->transferSucceed();
        break;
    default:
        break;
    }
}

void TransferHandle::handleFileTransStatus(QString statusstr)
{
    qInfo() << "handleFileTransStatus: " << statusstr;
//    FileStatus param;
//    param.from_json(statusstr.toStdString());

}

void TransferHandle::tryConnect(QString ip, QString password)
{
    if (!_backendOK) return;

    go([ip, password]() {
        TransferWoker::instance()->tryConnect(ip.toStdString(), password.toStdString());
    });
}

QString TransferHandle::getConnectPassWord()
{
    if (!_backendOK) return "";

    co::wait_group g_wg;
    g_wg.add(1);
    QString password;
    go([&password, g_wg]() {
        password = TransferWoker::instance()->getConnectPassWord();
        g_wg.done();
    });
    g_wg.wait();
    return password;
}

void TransferHandle::sendFiles(QStringList paths)
{
    if (!_backendOK) return;

    go([paths]() {
        TransferWoker::instance()->sendFiles(paths);
    });
}

TransferWoker::TransferWoker() {
    _gPool = new co::Pool(
        []() { return (void*) new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false); },
        [](void* p) { delete (rpc::Client*) p; }
    );
}

TransferWoker::~TransferWoker() {
    delete _gPool;
}

bool TransferWoker::pingBackend()
{
    co::pool_guard<rpc::Client> c(_gPool);
    co::Json req, res;
    //PingBackParam
    req = {
        { "who", "data-transfer" },
        { "version", UNI_IPC_PROTO },
        { "cb_port", UNI_IPC_FRONTEND_PORT },
    };

    req.add_member("api", "Backend.ping"); //BackendImpl::ping

    c->call(req, res);
    _session_id = res.get("msg").as_string(); // save the return session.

    //CallResult
    return res.get("result").as_bool();
}

QString TransferWoker::getConnectPassWord()
{
    co::pool_guard<rpc::Client> c(_gPool);
    co::Json req, res;

    req.add_member("api", "Backend.getPassword"); //BackendImpl::getPassword

    c->call(req, res);
//    c->close();

    return res.get("password").as_string().c_str();
}

void TransferWoker::tryConnect(const std::string &ip, const std::string &password)
{
    co::pool_guard<rpc::Client> c(_gPool);
    co::Json req, res;
    fastring target_ip(ip);
    fastring pin_code(password);

    req = {
        { "session", _session_id },
        { "host", target_ip },
        { "password", pin_code },
    };
    req.add_member("api", "Backend.tryConnect"); //BackendImpl::tryConnect
    c->call(req, res);
//    c->close();
}

void TransferWoker::sendFiles(QStringList filepaths)
{
    co::pool_guard<rpc::Client> c(_gPool);
    co::Json req, res, paths;

    for (QString path : filepaths) {
        paths.push_back(path.toStdString());
    }

    //TransFilesParam
    req = {
        { "session", _session_id },
        { "id", -1 }, // TODO: set trans job id
        { "paths", paths },
        { "sub", true },
        { "savedir", "" },
    };

    req.add_member("api", "Backend.tryTransFiles"); //BackendImpl::tryTransFiles

    c->call(req, res);
//    c->close();
}
