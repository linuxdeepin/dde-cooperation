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
#include "common/constant.h"
#include "ipc/fs.h"

using namespace deamon_core;

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
        if (DaemonConfig::instance()->needConfirm()) {
            // wait for user's confirmation
            co::Timer t;
            // t.restart();
            sleep::ms(20);
            int64 ms = t.ms();
        }

        fastring pass = Util::decodeBase64(request->auth().c_str());
        LOG << "pass= " << pass << " getPin=" << DaemonConfig::instance()->getPin();
        //FIXME: getPin is empty
        if (DaemonConfig::instance()->getPin().compare(pass) == 0) {
            response->set_error("Invalid auth code");
            response->set_token("");
        } else {
            DaemonConfig::instance()->saveSession(request->session_id());

            //TODO: generate auth token
            char *auth_token = "thatsgood";
            DaemonConfig::instance()->setTargetName(request->name().c_str());   // save the login name

            PeerInfo *info = new PeerInfo();
            info->set_version(version);
            info->set_hostname(Util::getHostname());
            info->set_privacy_mode(false);
            info->set_platform("UOS");
            info->set_username(Util::getCurrentUsername());

            response->set_allocated_peer_info(info);

            response->set_token(auth_token);
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
    fastring name = Util::parseFileName(request->path().c_str());
    if (request->push()) {
        // create write job and push into write_jobs
    }

    response->set_id(job_id);
    response->set_name(name.c_str());
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
    int32 id = request->id();

    FileEntry entry = request->entry();

    fastring filename = entry.name();
    size_t filesize = entry.size();
    FileType type = entry.type();

    bool exist = FSAdapter::newFile(filename.c_str(), type == DIR);

    response->set_id(id);
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
    LOG << "req= " << request->ShortDebugString().c_str();

    int32 id = request->id();
    uint32 blk_id = request->blk_id();
    fastring name = request->filename();
    fastring buffer = request->data();
    bool comp = request->compressed();
    size_t len = buffer.size();
    size_t offset = (blk_id > 0) ? ((blk_id - 1) * BLOCK_SIZE) : 0;

    bool good = FSAdapter::writeBlock(name.c_str(), offset, buffer.data(), len);

    response->set_id(id);
    response->set_name(name.c_str());
    response->set_result(good ? OK : IO_ERROR);

    LOG << "res= " << response->ShortDebugString().c_str();

    if (done) {
        done->Run();
    }
}

class ZRpcClientExecutor
{
public:
    ZRpcClientExecutor(const char *targetip, uint16 port)
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

void RemoteServiceBinder::startRpcListen()
{
#ifdef __linux__
    // TODO: load key from bin
    char key[] = "/usr/share/mobile-assistant-daemon/certificates/desktop.key";
    char crt[] = "/usr/share/mobile-assistant-daemon/certificates/desktop.crt";
#else
    char key[] = "certificates/desktop.key";
    char crt[] = "certificates/desktop.crt";
#endif

    zrpc_ns::ZRpcServer *server = new zrpc_ns::ZRpcServer(UNI_RPC_PORT_BASE, key, crt);
    server->registerService<RemoteServiceImpl>();

    LOG << "RPC server run...";
    // run in other co is OK
    if (server->start()) {
        ELOG << "RPC server start failed.";
    }
}

void RemoteServiceBinder::createExecutor(const char *targetip, int16_t port)
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
        emit loginResult(false, rpc_controller->ErrorText().c_str());
        return;
    }

    DLOG << "response body: " << rpc_res.ShortDebugString();
    fastring token = rpc_res.token();

    if (rpc_res.has_peer_info() && !token.empty()) {
        // TODO: save the target peer info into target's map
        PeerInfo target_info = rpc_res.peer_info();
        // login successful
        DaemonConfig::instance()->saveAuthed(token);

        return emit loginResult(true, "");
    }
    fastring err = rpc_res.error();
    emit loginResult(false, err.c_str());
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

void RemoteServiceBinder::doFileAction(int type, const char *actionjson)
{
    if (nullptr == _executor_p) {
        ELOG << "doLogin ERROR: no executor";
        return;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    FileAction rpc_req;
    FileResponse rpc_res;
    co::Json jsonObject = json::parse(actionjson);
    if (jsonObject == nullptr) {
        emit fileActionResult(false, -1);
        return;
    }
    LOG << "type: " << type << " jsonObject: " << jsonObject;

    switch (type) {
        // Just tell target to create read_job or write_job for file transfer.
    case TRANS_SEND:
    case TRANS_RECV: {
        ipc::FilesTrans trans;
        trans.from_json(jsonObject);
        std::string path = trans.paths[0].c_str();

        FileTransferRequest transRequest;
        transRequest.set_id(trans.id);
        transRequest.set_path(path);
        transRequest.set_file_num(trans.file_num);
        transRequest.set_include_hidden(false);
        if (type == TRANS_SEND) {
            // notify the target ready to receive.
            transRequest.set_ask(FileTransferRequest_Direction_Receive);
        } else {
            // notify the target send to some file.
            transRequest.set_ask(FileTransferRequest_Direction_Send);
        }

        rpc_req.set_allocated_transfer(&transRequest);
        break;
    }
    default:
        LOG << "Unsupported transfer type: " << type;
        break;
    }

    stub.fsaction(rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call server, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        emit fileActionResult(false, -1);
        return;
    }

    DLOG << "response body: " << rpc_res.ShortDebugString();
    uint32 id = 0;
    if (rpc_res.has_dir()) {
        FileDirectory dir = rpc_res.dir();
    } else if (rpc_res.has_block()) {
    } else if (rpc_res.has_error()) {
    } else if (rpc_res.has_digest()) {
    } else if (rpc_res.has_done()) {
    } else if (rpc_res.has_confirm()) {
        id = rpc_res.confirm().id();
        emit fileActionResult(id > 0, id);
    } else {
        ELOG << "unsupport response data in this method.";
    }

    emit fileActionResult(true, id);
}

int RemoteServiceBinder::doFileFlow(int type, const char *flowjson, const void *bindata, int binlen)
{
    int result = 0;
    if (nullptr == _executor_p) {
        ELOG << "doLogin ERROR: no executor";
        return PARAM_ERROR;
    }

    RemoteService_Stub stub(((ZRpcClientExecutor *)_executor_p)->chan());
    zrpc_ns::ZRpcController *rpc_controller = ((ZRpcClientExecutor *)_executor_p)->control();

    int32 req_id = 0;
    fastring req_blk_md5 = "";
    FileResponse rpc_req;
    FileResponse rpc_res;
    co::Json jsonObject = json::parse(flowjson);
    if (jsonObject == nullptr) {
        emit fileActionResult(false, 0);
        return PARAM_ERROR;
    }
    LOG << "type: " << type << " jsonObject: " << jsonObject;

    switch (type) {
        // Just tell target to create read_job or write_job for file transfer.
    case TRANS_BLOCK: {
        if (bindata == nullptr || binlen <= 0) {
            ELOG << "bindata is nullptr!!!";
            return PARAM_ERROR;
        }
        ipc::FileTransBlock block;
        block.from_json(jsonObject);
        req_id = block.id;

        FileTransferBlock transBlock;
        transBlock.set_id(req_id);
        transBlock.set_file_num(block.file_num);
        transBlock.set_data(bindata, binlen);
        transBlock.set_compressed(block.compressed);
        transBlock.set_blk_id(block.blk_id);

        rpc_req.set_allocated_block(&transBlock);

        break;
    }
    case TRANS_DIGEST: {
        ipc::FileTransDigest digest;
        digest.from_json(jsonObject);
        req_id = digest.id;

        FileTransferDigest transDigest;
        transDigest.set_id(req_id);
        transDigest.set_blk_id(digest.blk_id);
        transDigest.set_blk_md5(digest.blk_md5.c_str());

        rpc_req.set_allocated_digest(&transDigest);

        break;
    }
    case TRANS_ERROR: {
        ipc::FileTransError error;
        error.from_json(jsonObject);
        req_id = error.id;

        FileTransferError transError;
        transError.set_id(req_id);
        transError.set_error(error.error.c_str());
        transError.set_file_num(error.file_num);

        rpc_req.set_allocated_error(&transError);
        break;
    }
    case TRANS_DONE: {
        ipc::FileTransDone godone;
        godone.from_json(jsonObject);
        req_id = godone.id;

        FileTransferDone transDone;
        transDone.set_id(req_id);
        transDone.set_file_num(godone.file_num);

        rpc_req.set_allocated_done(&transDone);
        break;
    }
    default:
        LOG << "Unsupported transfer type: " << type;
        break;
    }

    stub.fsflow(rpc_controller, &rpc_req, &rpc_res, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call server, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        emit fileActionResult(false, -1);
        return INVOKE_FAIL;
    }

    DLOG << "response body: " << rpc_res.ShortDebugString();
    uint32 id = 0;
    if (rpc_res.has_digest()) {
        id = rpc_res.digest().id();

    } else if (rpc_res.has_error()) {
        id = rpc_res.error().id();

    } else if (rpc_res.has_done()) {
        id = rpc_res.done().id();
    } else {
        ELOG << "unsupport response data in this method.";
    }

    return INVOKE_OK;
}

int RemoteServiceBinder::doPushfileJob(int id, const char *filepath)
{
    int result = 0;
    if (nullptr == _executor_p || nullptr == filepath) {
        ELOG << "doPushfileJob ERROR: no executor";
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
        ELOG << "doPushfileJob getFileEntry ERROR!";
        return PARAM_ERROR;
    }

    req_create.set_id(id);
    req_create.set_allocated_entry(entry);

    stub.filetrans_create(rpc_controller, &req_create, &res_create, NULL);

    if (rpc_controller->ErrorCode() != 0) {
        ELOG << "Failed to call filetrans_create, error code: " << rpc_controller->ErrorCode()
             << ", error info: " << rpc_controller->ErrorText();
        return INVOKE_FAIL;
    }

    fs::file fd(filepath, 'r');
    if (!fd) {
        ELOG << "open file failed: " << filepath;
        return INVOKE_FAIL;
    }

    // Step2: send file block in loop
    FileTransBlock req_block;
    FileTransResponse res_block;

    int try_max = 3;
    size_t block_size = BLOCK_SIZE;
    size_t file_size = entry->size();
    size_t read_size = 0;
    uint32 block_id = 0;
    // void *buffer = co::alloc(block_size);
    fastring buf;
    buf.reserve(block_size);
    do {
        if (buf.capacity() == 0) buf.reserve(block_size);
        buf.resize(block_size);
        size_t resize = fd.read((char *)buf.data(), block_size);
        if (resize <= 0) {
            LOG << "read file ERROR or END, resize = " << resize;
            break;
        }
        // req_block.Clear();

        req_block.set_id(id);
        req_block.set_filename(entry->name());
        req_block.set_blk_id(block_id);
        req_block.set_data(buf.data());
        req_block.set_compressed(false);
    retry:
        stub.filetrans_block(rpc_controller, &req_block, &res_block, NULL);

        if (rpc_controller->ErrorCode() != 0) {
            ELOG << "Failed to call filetrans_block, error code: " << rpc_controller->ErrorCode()
                 << ", error info: " << rpc_controller->ErrorText();
            if (try_max > 0) {
                try_max--;
                goto retry;
            }
            fd.close();
            return INVOKE_FAIL;
        }

        FileTransRe result_res = res_block.result();
        if (IO_ERROR == result_res) {
            ELOG << "The target return ERROR, maybe it's IO";
            fd.close();
            emit fileTransResult(false, filepath, id);
            return INVOKE_OK;
        } else if (FINIASH == result_res) {
            emit fileTransResult(true, filepath, id);
        }

        read_size += resize;
        block_id++;

    } while (read_size < file_size);

end:
    fd.close();
    return INVOKE_DONE;
}
