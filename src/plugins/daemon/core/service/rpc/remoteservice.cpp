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
#include "zrpc.h"

#include "common/constant.h"
#include "version.h"
#include "utils/utils.h"
#include "utils/config.h"
#include "ipc/proto/chan.h"
#include "../comshare.h"
#include "../fsadapter.h"

void RemoteServiceImpl::login(::google::protobuf::RpcController *controller,
                              const ::LoginRequest *request,
                              ::LoginResponse *response,
                              ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();

    std::string version = request->version();
    if (version.compare(UNIAPI_VERSION) != 0) {
        // Notification not match version
        response->set_error("Invalid version");
        response->set_token("");
    } else {
        bool authOK = false;

        fastring pwd = request->auth();
        if (pwd.empty()) {
            // TODO: 无认证，用户确认
            if (DaemonConfig::instance()->needConfirm()) {
                IncomeData in;

                LoginConfirm confirm;
                confirm.user_name = request->my_name();
                confirm.session_id = request->session_id();
//                confirm.host_ip = ?

                in.type = IN_LOGIN;
                in.json = confirm.as_json().str();
                _income_chan << in;

                // wait for user's confirmation
//                co::Timer t;
//                // t.restart();
//                sleep::ms(20);
//                int64 ms = t.ms();
                OutData out;
                _outgo_chan >> out;
            }
        } else {
            fastring pass = Util::decodeBase64(pwd.c_str());
//            LOG << "pass= " << pass << " getPin=" << DaemonConfig::instance()->getPin();
            authOK = DaemonConfig::instance()->getPin().compare(pass) == 0;
        }

        if (!authOK) {
            response->set_error("Invalid auth code");
            response->set_token("");
        } else {
            DaemonConfig::instance()->saveRemoteSession(request->session_id());

            //TODO: generate auth token
            fastring auth_token = "thatsgood";
            DaemonConfig::instance()->setTargetName(request->name().c_str());   // save the login name
            fastring plattsr;
            if (WINDOWS == Util::getOSType()) {
                plattsr = "Windows";
            } else {
                //TODO: other OS
                plattsr = "UOS";
            }

            PeerInfo *info = new PeerInfo();
            info->set_version(version);
            info->set_hostname(Util::getHostname());
            info->set_privacy_mode(false);
            info->set_platform(plattsr.c_str());
            info->set_username(Util::getUsername());

            response->set_allocated_peer_info(info);

            response->set_token(auth_token.c_str());
        }
    }

    DaemonConfig::instance()->setStatus(connected);
    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::query_peerinfo(::google::protobuf::RpcController *controller,
                                       const ::PeerInfo *request,
                                       ::PeerInfo *response,
                                       ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::misc(::google::protobuf::RpcController *controller,
                             const ::Misc *request,
                             ::Misc *response,
                             ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::fsaction(::google::protobuf::RpcController *controller,
                                 const ::FileAction *request,
                                 ::FileResponse *response,
                                 ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::fsflow(::google::protobuf::RpcController *controller,
                               const ::FileResponse *request,
                               ::FileResponse *response,
                               ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::filetrans_job(::google::protobuf::RpcController *controller,
                                      const ::FileTransJob *request,
                                      ::FileTransResponse *response,
                                      ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();
    int32 job_id = request->job_id();
    IncomeData in;

    FSJob job;
    job.job_id = job_id;
    job.path = request->path();
    job.hidden = request->include_hidden();
    job.sub = request->recursive();
    job.write = request->push();
    job.who = request->app_who();

    in.type = IN_TRANSJOB;
    in.json = job.as_json().str();
    _income_chan << in;

    response->set_id(job_id);
    response->set_name(request->path().c_str());
    response->set_result(OK);

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::filetrans_create(::google::protobuf::RpcController *controller,
                                         const ::FileTransCreate *request,
                                         ::FileTransResponse *response,
                                         ::google::protobuf::Closure *done)
{
    LOG << "req= " << request->ShortDebugString().c_str();
    int32 fileid = request->file_id();

    FileEntry entry = request->entry();

    FileType type = entry.type();
    fastring filename;
    if (request->sub_dir().empty()) {
        filename = entry.name();
    } else {
        filename = request->sub_dir() + "/" + entry.name();
    }

    if (type == FILE_B) {
        FileInfo info;
        info.job_id = request->job_id();
        info.file_id = fileid;
        info.name = filename;
        info.total_size = entry.size();
        info.current_size = 0;
        info.time_spended = -1;

        IncomeData in;
        in.type = FS_INFO;
        in.json = info.as_json().str();
        _income_chan << in;
    }

    bool exist = FSAdapter::newFile(filename.c_str(), type == DIR);

    response->set_id(fileid);
    response->set_name(filename.c_str());
    response->set_result(exist ? OK : IO_ERROR);

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::filetrans_block(::google::protobuf::RpcController *controller,
                                        const ::FileTransBlock *request,
                                        ::FileTransResponse *response,
                                        ::google::protobuf::Closure *done)
{
    // LOG << "req= " << request->ShortDebugString().c_str();

    int32 job_id = request->job_id();
    int32 file_id = request->file_id();
    uint32 blk_id = request->blk_id();
    fastring name = request->filename();
    std::string buffer = request->data();
    bool comp = request->compressed();

    IncomeData in;

    FSDataBlock block;
    block.job_id = job_id;
    block.file_id = file_id;
    block.filename = name;
    block.blk_id = blk_id;
    block.compressed = comp;
    // do not set buffer at here, it will be cut by json.
    // block.data = buffer;

    in.type = FS_DATA;
    in.json = block.as_json().str();
    in.buf = buffer;
    _income_chan << in;

    // LOG << "filetrans_block name= " << name << " data len=" << len << " blk_id=" << blk_id;

    response->set_id(file_id);
    response->set_name(name.c_str());
    response->set_result(_income_chan.done() ? OK : IO_ERROR);

    // LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

void RemoteServiceImpl::filetrans_update(::google::protobuf::RpcController* controller,
                     const ::FileTransUpdate* request,
                     ::FileTransResponse* response,
                     ::google::protobuf::Closure* done)
{
    LOG << "req= " << request->ShortDebugString().c_str();
    if (request->has_report()) {
        FileTransJobReport report = request->report();

        FSReport info;
        info.job_id = report.job_id();
        info.path = report.path();
        info.result = report.result();
        info.error = report.error();

        IncomeData in;
        in.type = FS_REPORT;
        in.json = info.as_json().str();
        _income_chan << in;

        response->set_id(report.job_id());
        response->set_name(report.path());
    } else if (request->has_cancel()) {
        FileTransJobCancel cancel = request->cancel();

        FSJobCancel info;
        info.job_id = cancel.job_id();
        info.path = cancel.path();

        IncomeData in;
        in.type = TRANS_CANCEL;
        in.json = info.as_json().str();
        _income_chan << in;

        response->set_id(cancel.job_id());
        response->set_name(cancel.path());
    }

    response->set_result(OK);

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

class ZRpcClientExecutor
{
public:
    ZRpcClientExecutor(const char *targetip, uint16_t port)
    {
        _client = new zrpc_ns::ZRpcClient(targetip, port, true);
    }

    ~ZRpcClientExecutor() = default;

    zrpc_ns::ZRpcChannel *chan() { return _client->getChannel(); }

    zrpc_ns::ZRpcController *control() { return _client->getControler(); }

private:
    zrpc_ns::ZRpcClient *_client { nullptr };
};

RemoteServiceBinder::RemoteServiceBinder(QObject *parent)
    : QObject(parent)
{
}

RemoteServiceBinder::~RemoteServiceBinder()
{
}

void RemoteServiceBinder::startRpcListen(const char *keypath, const char *crtpath)
{
    char key[1024];
    char crt[1024];
    strcpy(key, keypath);
    strcpy(crt, crtpath);
    zrpc_ns::ZRpcServer *server = new zrpc_ns::ZRpcServer(UNI_RPC_PORT_BASE, key, crt);
    server->registerService<RemoteServiceImpl>();

    LOG << "RPC server run...";
    // run in other co is OK
    if (!server->start()) {
        ELOG << "RPC server start failed.";
    }
}

void RemoteServiceBinder::createExecutor(const char *targetip, uint16_t port)
{
    _executor_p = co::make<ZRpcClientExecutor>(targetip, port);
}

void RemoteServiceBinder::doLogin(const char *username, const char *pincode)
{
    if (nullptr == _executor_p) {
        ELOG << "doLogin ERROR: no executor";
        return;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    LoginRequest rpc_req;
    LoginResponse rpc_res;

    // 使用base64加密auth
    rpc_req.set_name(username);
    rpc_req.set_auth(Util::encodeBase64(pincode));

    std::string uuid = Util::genUUID();
    rpc_req.set_my_uid(uuid);
    rpc_req.set_my_name(Util::getHostname());

    OptionMessage option;
    option.set_lock_after_session_end(OptionMessage_BoolOption_NotSet);

    rpc_req.set_session_id(hash64(uuid));   // gen from the uuid
    rpc_req.set_version(UNIAPI_VERSION);

    stub.login(rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call server, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        emit loginResult(false, QString(username));
        return;
    }

    DLOG << "response body: " << rpc_res.ShortDebugString();
    fastring token = rpc_res.token();

    if (rpc_res.has_peer_info() && !token.empty()) {
        // TODO: save the target peer info into target's map
        PeerInfo target_info = rpc_res.peer_info();
        // login successful
        DaemonConfig::instance()->saveAuthed(token);

        return emit loginResult(true, QString(username));
    }

    emit loginResult(false, QString(username));
}

void RemoteServiceBinder::doQuery()
{
    if (nullptr == _executor_p) {
        ELOG << "doLogin ERROR: no executor";
        return;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    PeerInfo rpc_req;
    PeerInfo rpc_res;

    stub.query_peerinfo(rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call server, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        emit queryResult(false, rpc_controller->ErrorText().c_str());
        return;
    }

    DLOG << "response body: " << rpc_res.ShortDebugString();

    emit queryResult(true, "");
}

void RemoteServiceBinder::doMisc()
{
    if (nullptr == _executor_p) {
        ELOG << "doLogin ERROR: no executor";
        return;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    Misc rpc_req;
    Misc rpc_res;

    stub.misc(rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call server, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        emit miscResult(false, rpc_controller->ErrorText().c_str());
        return;
    }

    DLOG << "response body: " << rpc_res.ShortDebugString();

    emit miscResult(true, "");
}

int RemoteServiceBinder::doTransfileJob(int id, const char *path, bool hidden, bool recursive, bool recv)
{
    if (nullptr == _executor_p || nullptr == path) {
        ELOG << "doTransfileJob ERROR: no executor";
        return PARAM_ERROR;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    FileTransJob req_job;
    FileTransResponse res_job;

    req_job.set_job_id(id);
    req_job.set_path(path);
    req_job.set_include_hidden(hidden);
    req_job.set_recursive(recursive);
    req_job.set_push(!recv);

    stub.filetrans_job(rpc_controller, &req_job, &res_job, nullptr);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call filetrans_job, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        return INVOKE_FAIL;
    }

    FileTransRe result_res = res_job.result();
    if (OK == result_res) {
        return INVOKE_OK;
    }

    return INVOKE_FAIL;
}

int RemoteServiceBinder::doSendFileInfo(int jobid, int fileid, const char *subdir, const char *filepath)
{
    if (nullptr == _executor_p || nullptr == filepath) {
        ELOG << "doSendFileInfo ERROR: no executor";
        return PARAM_ERROR;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    // Step1: create file
    FileTransCreate req_create;
    FileTransResponse res_create;

    // read file info
    FileEntry *entry = new FileEntry();
    if (FSAdapter::getFileEntry(filepath, &entry) < 0) {
        ELOG << "doSendFileInfo getFileEntry ERROR!";
        return PARAM_ERROR;
    }

    req_create.set_job_id(jobid);
    req_create.set_file_id(fileid);
    req_create.set_sub_dir(subdir);
    req_create.set_allocated_entry(entry);

    stub.filetrans_create(rpc_controller, &req_create, &res_create, nullptr);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call filetrans_create, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        return INVOKE_FAIL;
    }

    return INVOKE_DONE;
}

int RemoteServiceBinder::doSendFileBlock(FileTransBlock fileblock)
{
    if (nullptr == _executor_p) {
        ELOG << "doSendFileBlock ERROR: no executor";
        return PARAM_ERROR;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    FileTransResponse res_block;
    int try_max = 3;
retry:
    stub.filetrans_block(rpc_controller, &fileblock, &res_block, nullptr);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call filetrans_block, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        if (try_max > 0) {
            try_max--;
            goto retry;
        }
        return INVOKE_FAIL;
    }

    // DLOG << "send block response body: " << res_block.ShortDebugString();

    FileTransRe result_res = res_block.result();
    if (IO_ERROR == result_res) {
        ELOG << "The target return ERROR, maybe it's IO";
        return INVOKE_OK;
    }

    return INVOKE_DONE;
}

int RemoteServiceBinder::doUpdateTrans(FileTransUpdate update)
{
    if (nullptr == _executor_p) {
        ELOG << "doUpdateTrans ERROR: no executor";
        return PARAM_ERROR;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    FileTransResponse res;
    stub.filetrans_update(rpc_controller, &update, &res, nullptr);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call filetrans_update, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        return INVOKE_FAIL;
    }

    FileTransRe result_res = res.result();
    if (IO_ERROR == result_res) {
        ELOG << "The target return ERROR, maybe it's IO";
        return INVOKE_OK;
    }

    return INVOKE_DONE;
}
