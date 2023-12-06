// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferjob.h"
#include "co/log.h"
#include "co/fs.h"
#include "co/path.h"
#include "co/co.h"
#include "common/constant.h"
#include "common/commonstruct.h"
#include "service/fsadapter.h"
#include "service/rpc/sendrpcservice.h"
#include "service/ipc/sendipcservice.h"
#include "utils/config.h"
#include "service/comshare.h"

#include <QPointer>

TransferJob::TransferJob(QObject *parent)
    : QObject(parent)
{
    _status = NONE;
}

TransferJob::~TransferJob()
{
    _status = STOPED;
}

bool TransferJob::initRpc(fastring target, uint16 port)
{
    _tar_ip = target;
    _tar_port = port;
    // ip是空，并且是发送那就出现错误了
    if (target.empty() && !_writejob) {
        ELOG << "TransferJob initRpc ip is empty and is push job!! app = " << _app_name;
        return false;
    }
    _remote.reset(new RemoteServiceSender(_app_name.c_str(), _tar_ip.c_str(), _tar_port, true));
    if (!_writejob) {
        FileTransJob req_job;
        req_job.job_id = _jobid;
        req_job.save_path = _savedir.c_str();
        req_job.include_hidden = false;
        req_job.path = _path;
        req_job.sub = _sub;
        req_job.write = (!_writejob);
        req_job.app_who = _tar_app_name;
        req_job.targetAppname = _app_name;
        // 必须等待对方回复了才执行后面的流程
        auto res = _remote->doSendProtoMsg(IN_TRANSJOB, req_job.as_json().str().c_str(), QByteArray());
        if (res.errorType < INVOKE_OK) {
            SendStatus st;
            st.type = res.errorType;
            st.msg = res.as_json().str();
            co::Json req = st.as_json();
            req.add_member("api", "Frontend.notifySendStatus");
            SendIpcService::instance()->handleSendToAllClient(req.str().c_str());
            this->_init_success = false;
            return false;
        }
    }
    return true;
}

// 先调用initrpc, 在调用initjob
void TransferJob::initJob(fastring appname, fastring targetappname, int id, fastring path, bool sub, fastring savedir, bool write)
{
    _app_name = appname;
    _tar_app_name = targetappname;
    _jobid = id;
    _path = path;
    _sub = sub;
    _savedir = savedir;
    _writejob = write;
    _status = INIT;
    if (_writejob) {
        fastring fullpath = path::join(
                DaemonConfig::instance()->getStorageDir(_app_name), _savedir);
        FSAdapter::newFileByFullPath(fullpath.c_str(), true);
    }
}

bool TransferJob::createFile(const QString &filename, const bool isDir)
{
    fastring path = path::join(DaemonConfig::instance()->getStorageDir(_app_name), _savedir);
    fastring fullpath = path::join(path, filename.toStdString().c_str());
    return FSAdapter::newFileByFullPath(fullpath.c_str(), isDir);
}

void TransferJob::start()
{
    atomic_store(&_status, STARTED);
    if (_writejob) {
        DLOG << "start write job: " << _savedir;
        handleJobStatus(JOB_TRANS_DOING);
    } else {
        //并行读取文件数据
        DLOG << "doTransfileJob path to save:" << _savedir;
        co::Json pathJson;
        pathJson.parse_from(_path);
        DLOG << "read job start path: " << pathJson;
        for (uint32 i = 0; i < pathJson.array_size(); i++) {
            fastring jobpath = pathJson[i].as_string();
            std::pair<fastring, fastring> pairs = path::split(jobpath);
            fastring rootpath = pairs.first.c_str();
            scanPath(rootpath, jobpath);
        }
        DLOG << "read job init end " << _file_info_maps.size();
    }

    // 开始循环处理数据块
    handleBlockQueque();

    // 自己完成了
    co::sleep(100);
    emit notifyJobFinished(_jobid);
}

void TransferJob::stop()
{
    DLOG << "(" << _jobid << ") stop now!";
    atomic_store(&_status, STOPED);
}

void TransferJob::waitFinish()
{
    DLOG << "(" << _jobid << ") wait write finish!";
    atomic_store(&_status, WAIT_DONE);
}

bool TransferJob::ended()
{
    return unlikely(_status == STOPED);
}

bool TransferJob::isRunning()
{
    return unlikely(_status == STARTED);
}

bool TransferJob::isWriteJob()
{
    return _writejob;
}

fastring TransferJob::getAppName()
{
    return _app_name;
}

void TransferJob::cancel(bool notify)
{
    _file_info_maps.clear();
    if (notify) {
        atomic_cas(&_status, STARTED, CANCELING);   // 如果运行，则赋值取消
    } else {
        handleJobStatus(JOB_TRANS_CANCELED);   // 通知前端应用作业已取消
        atomic_store(&_status, STOPED);
    }
}

void TransferJob::pushQueque(const QSharedPointer<FSDataBlock> block)
{
    if (_status == CANCELING) {
        DLOG << "This job has mark cancel, stop handle data.";
        return;
    }
    co::mutex_guard g(_queque_mutex);
    _block_queue.enqueue(block);
}

void TransferJob::insertFileInfo(FileInfo &info)
{
    // reset
    atomic_store(&_queue_empty_times, 0);

    int fileid = info.file_id;
    auto it = _file_info_maps.find(fileid);
    if (it == _file_info_maps.end()) {
        DLOG_IF(FLG_log_detail) << "insertFileInfo new file INFO: " << fileid << info.name;
        // skip 0B file
        if (info.total_size > 0) {
            co::mutex_guard g(_map_mutex);
            _file_info_maps.insert(std::make_pair(fileid, info));
            handleTransStatus(FILE_TRANS_IDLE, info);
        }
    }
}

fastring TransferJob::getSubdir(const char *path, const char *root)
{
    fastring indir = "";
    std::pair<fastring, fastring> pairs = path::split(fastring(path));
    fastring filedir = pairs.first;
    indir = filedir.size() > strlen(root) ? filedir.remove_prefix(root) : "";
    fastring subdir = path::join(indir.c_str(), "");
    return subdir;
}

void TransferJob::scanPath(fastring root, fastring path)
{
#ifdef linux
    // 链接文件不拷贝
    if (fs::isSymlink(path.c_str()))
        return;
#endif
    _fileid++;
    fastring subdir = getSubdir(path.c_str(), root.c_str());
    FileTransCreate info;
    info.job_id = _jobid;
    info.file_id = _fileid;
    info.sub_dir = subdir.c_str();
    FileEntry *entry = new FileEntry();
    if (FSAdapter::getFileEntry(path.c_str(), &entry) < 0) {
        ELOG << "get file entry error !!!!";
        cancel();
        return;
    }
    info.entry = *entry;
    info.entry.appName = _app_name;
    info.entry.rcvappName = _tar_app_name;
    auto res = _remote->doSendProtoMsg(FS_INFO, info.as_json().str().c_str(), QByteArray());
    if (res.errorType < INVOKE_OK) {
        SendStatus st;
        st.type = res.errorType;
        st.msg = res.as_json().str();
        co::Json req = st.as_json();
        req.add_member("api", "Frontend.notifySendStatus");
        SendIpcService::instance()->handleSendToAllClient(req.str().c_str());
        cancel();
    }
    if (_status >= STOPED)
        return;
    if (fs::isdir(path.c_str())) {
        readPath(path, root);
    } else {
        readFile(path, _fileid, subdir.c_str());
    }
}

void TransferJob::readPath(fastring path, fastring root)
{
    if (_status >= STOPED)
        return;

    fastring dirpath = path::join(path, "");
    fs::dir d(dirpath);
    auto v = d.all();   // 读取所有子项
    for (const fastring &file : v) {
        fastring file_path = path::join(d.path(), file.c_str());
        scanPath(root, file_path);
    }
}

bool TransferJob::readFile(fastring filepath, int fileid, fastring subdir)
{
    if (_status >= STOPED)
        return false;

    std::pair<fastring, fastring> pairs = path::split(fastring(filepath));

    fastring filename = pairs.second;

    fastring subname = path::join(subdir, filename.c_str());
    readFileBlock(filepath, fileid, subname);
    return true;
}

void TransferJob::readFileBlock(fastring filepath, int fileid, const fastring subname)
{
    if (filepath.empty() || fileid < 0 || !fs::exists(filepath)) {
        ELOG << "readFileBlock file is invaild" << filepath;
        return;
    }

    if (_status >= STOPED)
        return;

    size_t block_size = BLOCK_SIZE;
    int64 file_size = fs::fsize(filepath);

    if (file_size <= 0) {
        // error file or 0B file.
        return;
    }
    // record the file info
    {
        FileInfo info;
        info.job_id = _jobid;
        info.file_id = fileid;
        info.name = filepath;
        info.total_size = file_size;
        info.current_size = 0;
        info.time_spended = -1;

        insertFileInfo(info);
        //        LOG << "======this file (" << subname << "fileid" << fileid << ") start   _file_info_maps";
    }
    QPointer<TransferJob> self = this;
    UNIGO([self, filepath, fileid, subname, file_size, block_size]() {
        int64 read_size = 0;
        uint32 block_id = 0;

        fs::file fd(filepath, 'r');
        if (!fd) {
            ELOG << "open file failed: " << filepath;
            return;
        }

        size_t block_len = block_size * sizeof(char);
        char *buf = reinterpret_cast<char *>(malloc(block_len));
        size_t resize = 0;
        do {
            // 最多300个数据块
            if (self && self->queueCount() > 300)
                co::sleep(10);
            if (self.isNull() || self->_status >= STOPED)
                break;

            memset(buf, 0, block_len);
            resize = fd.read(buf, block_size);
            if (resize <= 0) {
                LOG_IF(FLG_log_detail) << "read file ERROR or END, resize = " << resize;
                break;
            }

            QSharedPointer<FSDataBlock> block(new FSDataBlock);
            if (self)
                block->job_id = self->_jobid;
            block->file_id = fileid;
            block->filename = subname;
            block->blk_id = block_id;
            block->compressed = false;
            // copy binrary data
            fastring bufdata(buf, resize);
            block->data = bufdata;
            if (self)
                self->pushQueque(block);

            read_size += resize;
            block_id++;
        } while (read_size < file_size || resize > 0);

        free(buf);
        fd.close();
    });
}

void TransferJob::handleBlockQueque()
{
    atomic_store(&_queue_empty_times, 0);
    // 定时获取状态并通知，检测是否结束
    QPointer<TransferJob> self = this;
    UNIGO([self]() {
        bool exit = false;
        bool next_exit = false;
        do {
            co::sleep(1000);   // 每秒检测发送一次状态
            if (self.isNull()) {
                DLOG << "job has been distructed, break!";
                break;
            }
            exit = (self->_status == CANCELING) || (self->_status >= STOPED);
            if (exit) {
                DLOG << "job has stop, break!";
                break;
            }

            bool empty = self->syncHandleStatus();   //检测一次所有文件是否完成
            if (empty) {
                // 文件信息已全部为空，可能结束，也可能还有文件没有开始
                if (self->_status == WAIT_DONE) {
                    // 对端发送结束，等待完成
                    if (next_exit) {
                        // 已经被标记等待结束，这里定时更新，说明一定时间内无数据，直接结束
                        self->handleJobStatus(JOB_TRANS_FINISHED);
                        break;
                    } else {
                        next_exit = true;   //标记等待下一次状态检测退出
                        continue;
                    }
                } else if (!self->_writejob) {
                    DLOG_IF(FLG_log_detail) << "all file finished, send job finish.";
                    // 通知对端文件已发完，并等待下一循环退出
                    self->handleUpdate(FINIASH, self->_path.c_str(), "");
                    atomic_store(&(self->_status), WAIT_DONE);
                }
            }

            if (self->_queue_empty_times > 3000) {
                // 每10ms增1，连续30s无数据
                DLOG << " wait block data timeout NOW!";
                self->handleJobStatus(JOB_TRANS_FAILED);
                break;
            }
        } while (!exit);

        if (!self.isNull() && self->_status != STOPED)
            self->stop();
    });

    bool exception = false;
    while (_status != STOPED) {
        if (_status == CANCELING) {
            break;
        }
        auto block = popQueue();
        if (block.isNull()) {
            atomic_inc(&_queue_empty_times);
            co::sleep(10);
            continue;
        } else {
            // reset
            atomic_store(&_queue_empty_times, 0);
        }

        int32 job_id = block->job_id;
        int32 file_id = block->file_id;
        uint64 blk_id = block->blk_id;
        fastring path = path::join(DaemonConfig::instance()->getStorageDir(_app_name), _savedir);
        fastring name = path::join(path, block->filename);
        fastring buffer = block->data;
        size_t len = buffer.size();
        bool comp = block->compressed;

        if (_writejob) {
            int64 offset = static_cast<int64>(blk_id * BLOCK_SIZE);
            // ELOG << "file : " << name << " write : " << len;
            bool good = FSAdapter::writeBlock(name.c_str(), offset, buffer.c_str(), len);
            if (!good) {
                ELOG << "file : " << name << " write BLOCK error";
                exception = true;
                // FIXME: the client can not callback.
                // handleUpdate(IO_ERROR, block.filename.c_str(), "failed to write", binder);
            }
        } else {
            co::sleep(25);
            FileTransBlock file_block;
            file_block.job_id = (job_id);
            file_block.file_id = (file_id);
            file_block.filename = (block->filename.c_str());
            file_block.blk_id = (static_cast<uint>(blk_id));
            file_block.compressed = (comp);
            QByteArray data(buffer.c_str(), static_cast<int>(len));
            // DLOG << "(" << job_id << ") send block " << block->filename << " size: " << len;
            auto res = _remote->doSendProtoMsg(FS_DATA, file_block.as_json().str().c_str(), data);
            if (res.errorType < INVOKE_OK) {
                SendStatus st;
                st.type = res.errorType;
                st.msg = res.as_json().str();
                co::Json req = st.as_json();
                req.add_member("api", "Frontend.notifySendStatus");
                SendIpcService::instance()->handleSendToAllClient(req.str().c_str());
                cancel();
                exception = true;
            }
        }

        if (!exception) {
            // update the file info
            auto it = _file_info_maps.find(file_id);
            if (it != _file_info_maps.end()) {
                it->second.current_size += len;
                if (blk_id == 0) {
                    // first block data, file begin, start record time
                    it->second.time_spended = 0;
                }
            }
        }
    };
    if (exception) {
        DLOG << "trans job exception hanpend: " << _jobid;
        handleJobStatus(JOB_TRANS_FAILED);
    }

    LOG << "trans job end: " << _jobid;
    atomic_store(&_status, STOPED);
}

void TransferJob::handleUpdate(FileTransRe result, const char *path, const char *emsg)
{
    FileTransJobReport report;
    report.job_id = (_jobid);
    report.path = (path);
    report.result = (result);
    report.error = (emsg);
    auto res = _remote->doSendProtoMsg(FS_REPORT,
                                       report.as_json().str().c_str(), QByteArray());
}

bool TransferJob::syncHandleStatus()
{
    QPointer<TransferJob> self = this;
    // update the file info
    for (auto &pair : _file_info_maps) {
        if (self.isNull())
            return true;

        //DLOG << "syncHandleStatus()" << pair.second.name;
        if (pair.second.time_spended >= 0) {
            pair.second.time_spended += 1;
            bool end = pair.second.current_size >= pair.second.total_size;
            handleTransStatus(end ? FILE_TRANS_END : FILE_TRANS_SPEED, pair.second);
            if (end) {
                DLOG_IF(FLG_log_detail) << "should notify file finish: " << pair.second.name << " jobid=" << pair.second.job_id;
                co::mutex_guard g(_map_mutex);
                _file_info_maps.erase(pair.first);
                if (!_writejob) {
                    //TODO: check sum
                    // handleUpdate(OK, block.filename.c_str(), "");
                }
            }
        }
    }
    return _file_info_maps.empty();
}

void TransferJob::handleJobStatus(int status)
{
    QString appname(_app_name.c_str());
    fastring fullpath = path::join(
            DaemonConfig::instance()->getStorageDir(_app_name), _savedir);
    QString savepath(fullpath.c_str());

    emit notifyJobResult(appname, _jobid, status, savepath);
}

void TransferJob::handleTransStatus(int status, FileInfo &info)
{
    co::Json filejson = info.as_json();
    QString appname(_app_name.c_str());
    QString fileinfo(filejson.str().c_str());

    // FileInfo > FileStatus in handle func
    emit notifyFileTransStatus(appname, status, fileinfo);
}

QSharedPointer<FSDataBlock> TransferJob::popQueue()
{
    co::mutex_guard g(_queque_mutex);
    if (_block_queue.empty())
        return nullptr;
    return _block_queue.dequeue();
}

int TransferJob::queueCount() const
{
    co::mutex_guard g(_queque_mutex);
    return _block_queue.count();
}
