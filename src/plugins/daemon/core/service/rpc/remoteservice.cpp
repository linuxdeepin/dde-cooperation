// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <google/protobuf/service.h>
#include <sstream>
#include <atomic>

#include "remoteservice.h"
#include "co/log.h"
#include "co/co.h"
#include "co/time.h"
#include "co/json.h"
#include "co/mem.h"
#include "co/fs.h"

#include "common/constant.h"
#include "common/comonstruct.h"
#include "version.h"
#include "utils/utils.h"
#include "utils/config.h"
#include "ipc/proto/chan.h"
#include "../comshare.h"
#include "../fsadapter.h"

static QReadWriteLock _executor_lock;
// <ip, executor>
static QMap<QString, QSharedPointer<ZRpcClientExecutor>> _executor_ps;

void RemoteServiceImpl::proto_msg(google::protobuf::RpcController *controller,
                                  const ProtoData *request, ProtoData *response,
                                  google::protobuf::Closure *done)
{
    Q_UNUSED(controller);
    LOG << " req= type = " << request->type() << " msg  =  " << request->msg();
    IncomeData in;
    in.type = static_cast<IncomeType>(request->type());
    in.json = request->msg();
    std::string buffer = request->data();
    in.buf = buffer;
    _income_chan << in;

    OutData out;
    _outgo_chan >> out;
    response->set_type(in.type);
    response->set_msg(out.json.c_str());

    LOG << " res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

RemoteServiceSender::RemoteServiceSender(const QString &appname, const QString &ip, const uint16 port, QObject *parent)
    : QObject(parent)
    , _app_name(appname)
    , _target_ip(ip)
    , _target_port(port)
{
}

RemoteServiceSender::~RemoteServiceSender()
{
}

SendResult RemoteServiceSender::doSendProtoMsg(const uint32 type, const QString &msg, const QByteArray &data)
{
    ELOG << "send to remote = " << type << " = " << msg.toStdString();
    auto _executor_p = createExecutor();
    SendResult res;
    res.protocolType = type;
    if (_executor_p.isNull()) {
        res.errorType = PARAM_ERROR;
        res.data = "doSendProtoMsg ERROR: no executor";
        ELOG << "doSendProtoMsg ERROR: no executor";
        return res;
    }

    RemoteService_Stub stub(_executor_p->chan());
    zrpc_ns::ZRpcController *rpc_controller = _executor_p->control();

    ProtoData req, rpc_res;
    req.set_type(type);
    req.set_msg(msg.toStdString());
    fastring dt(data.toStdString().c_str(), static_cast<size_t>(data.size()));
    req.set_data(dt.c_str(), dt.size());
    int retryCount = 0;

retryed:
    stub.proto_msg(rpc_controller, &req, &rpc_res, nullptr);

    if (rpc_controller->ErrorCode() != 0) {
        retryCount++;
        if (retryCount <= 3)
            goto retryed;

        res.errorType = INVOKE_FAIL;
        ELOG << "Failed to call server, error code: " << rpc_controller->ErrorCode()
            << ", error info: " << rpc_controller->ErrorText();
        res.data = "Failed to call server, error code: "
                + QString::number(rpc_controller->ErrorCode()).toStdString()
                + ", error info: " + rpc_controller->ErrorText();

        return res;
    }

    res.errorType = INVOKE_OK;
    res.data = rpc_res.msg();
    DLOG << "response body: " << rpc_res.ShortDebugString() << "\n" << res.as_json();
    return res;
}

void RemoteServiceSender::clearExecutor(const QString &appname)
{
    QWriteLocker lk(&_executor_lock);
    _executor_ps.remove(appname);
}

void RemoteServiceSender::remoteIP(const QString &session, QString *ip, uint16 *port)
{
    QReadLocker lk(&_executor_lock);
    auto _exector = _executor_ps.value(session);
    if (_exector.isNull()) {
        ELOG << "current session = " << session.toStdString() << " has no  executor!!";
        return;
    }
    if (ip)
        *ip = _exector->targetIP();
    if (port)
        *port = _exector->targetPort();
}

void RemoteServiceSender::setIpInfo(const QString &ip, const uint16 port)
{
    if (ip == _target_ip && port == _target_port)
        return;

    _target_ip = ip;
    _target_port = port;
}

void RemoteServiceSender::setTargetAppName(const QString &targetApp)
{
    _tar_app_name = targetApp;
}

QSharedPointer<ZRpcClientExecutor> RemoteServiceSender::createExecutor()
{
    ELOG << "app name : " << _app_name.toStdString() << ", = ip "
         << _target_ip.toStdString() << " : port =  " << _target_port;
    QWriteLocker lk(&_executor_lock);
    if (_target_ip.isEmpty()) {
        ELOG << "Invalide IP address, _target_ip is empty!!!!";
        return nullptr;
    }
    auto _exec = _executor_ps.value(_target_ip);
    if (!_exec.isNull())
        return _exec;
    _exec.reset(new ZRpcClientExecutor(_target_ip.toStdString().c_str(),
                                       _target_port));
    _executor_ps.insert(_target_ip, _exec);
    return _exec;
}

RemoteServiceBinder::RemoteServiceBinder(QObject *parent) : QObject (parent)
{

}

void RemoteServiceBinder::startRpcListen(const char *keypath, const char *crtpath,
                                         const std::function<void (int, const fastring &, const uint16)> &call)
{
    char key[1024];
    char crt[1024];
    strcpy(key, keypath);
    strcpy(crt, crtpath);
    zrpc_ns::ZRpcServer *server = new zrpc_ns::ZRpcServer(UNI_RPC_PORT_BASE, key, crt);
    server->registerService<RemoteServiceImpl>();
    if (call) {
        callback = call;
        server->setCallBackFunc(callback);
    }
    server->start();
}
