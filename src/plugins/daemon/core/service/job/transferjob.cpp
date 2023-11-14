// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferjob.h"
#include "co/log.h"
#include "co/fs.h"
#include "co/path.h"
#include "co/co.h"
#include "common/constant.h"
#include "service/fsadapter.h"
#include "utils/config.h"

TransferJob::TransferJob(QObject *parent)
    : QObject(parent)
{
}

void TransferJob::initRpc(fastring target, uint16 port)
{
    if (nullptr == _rpcBinder) {
        _rpcBinder = new RemoteServiceBinder(this);
        _rpcBinder->createExecutor(_tar_app_name.c_str(), target.c_str(), port);
    }
}

void TransferJob::initJob(fastring appname, fastring targetappname, int id, fastring path, bool sub, fastring savedir, bool write)
{
    _app_name = appname;
    _tar_app_name = targetappname;
    _jobid = id;
    _path = path;
    _sub = sub;
    _savedir = savedir;
    _writejob = write;
    _inited = true;
}

void TransferJob::start()
{
    _stoped = false;
    _finished = false;
    _jobCanceled = false;
    if (_writejob) {
        DLOG << "start write job: " << _savedir;
        handleJobStatus(JOB_TRANS_DOING);
    } else {
        //并行读取文件数据
        DLOG << "doTransfileJob path to save:" << _savedir;
        int res = _rpcBinder->doTransfileJob(_app_name.c_str(), _jobid, _savedir.c_str(), false, _sub, _writejob);
        if (res < 0) {
            ELOG << "binder doTransfileJob failed: " << res << " jobpath: " << _path;
            _stoped = true;
            handleJobStatus(JOB_TRANS_FAILED);
        }

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
}

void TransferJob::stop()
{
    DLOG << "(" << _jobid << ") stop now!";
    _stoped = true;
}

void TransferJob::waitFinish()
{
    DLOG << "(" << _jobid << ") wait write finish!";
    _waitfinish = true;
}

bool TransferJob::finished()
{
    return _finished;
}

bool TransferJob::isRunning()
{
    return !_stoped;
}

bool TransferJob::isWriteJob()
{
    return _writejob;
}

fastring TransferJob::getAppName()
{
    return _app_name;
}

void TransferJob::cancel()
{
    _file_info_maps.clear();
    _jobCanceled = true;
    if (_writejob) {
        // FIXME: the receive can not send rpc
        //        UNIGO([this] {
        //            FileTransJobCancel *cancel = new FileTransJobCancel();
        //            FileTransUpdate update;

        //            cancel->set_job_id(_jobid);
        //            cancel->set_path(_path.c_str());

        //            update.set_allocated_cancel(cancel);
        //            int res = _rpcBinder->doUpdateTrans(_tar_app_name.c_str(), update);
        //            if (res <= 0) {
        //                ELOG << "update failed: " << _path << " result=" << result;
        //            }
        //        });
    }
    this->stop();
}

void TransferJob::pushQueque(const QSharedPointer<FSDataBlock> block)
{
    if (_jobCanceled) {
        DLOG << "This job has mark cancel, stop handle data.";
        return;
    }
    co::mutex_guard g(_queque_mutex);
    _block_queue.enqueue(block);
}

void TransferJob::insertFileInfo(FileInfo &info)
{
    int fileid = info.file_id;
    auto it = _file_info_maps.find(fileid);
    if (it == _file_info_maps.end()) {
        DLOG << "insertFileInfo new file INFO: " << fileid << info.name;
        // skip 0B file
        if (info.total_size > 0) {
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
    _fileid++;
    fastring subdir = getSubdir(path.c_str(), root.c_str());
    int res =
            _rpcBinder->doSendFileInfo(_tar_app_name.c_str(), _jobid, _fileid, subdir.c_str(), path.c_str());
    if (res <= 0) {
        ELOG << "error file info : " << path;
        return;
    }
    if (fs::isdir(path.c_str())) {
        readPath(path, root);
    } else {
        readFile(path, _fileid, subdir.c_str());
    }
}

void TransferJob::readPath(fastring path, fastring root)
{
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

    size_t block_size = BLOCK_SIZE;
    int64 file_size = fs::fsize(filepath);

    if (file_size <= 0) {
        // error file or 0B file.
        return;
    }
    // record the file info
    {
        FileInfo info;
        info.file_id = fileid;
        info.name = subname;
        info.total_size = file_size;
        info.current_size = 0;
        info.time_spended = -1;
        _file_info_maps.insert(std::make_pair(fileid, info));
        handleTransStatus(FILE_TRANS_IDLE, info);
        //        LOG << "======this file (" << subname << "fileid" << fileid << ") start   _file_info_maps";
    }
    UNIGO([this, filepath, fileid, subname, file_size, block_size]() {
        int64 read_size = 0;
        uint32 block_id = 0;

        fs::file fd(filepath, 'r');
        if (!fd) {
            ELOG << "open file failed: " << filepath;
            return;
        }

        size_t block_len = block_size * sizeof(char);
        char *buf = reinterpret_cast<char *>(malloc(block_len));
        do {
            memset(buf, 0, block_len);
            size_t resize = fd.read(buf, block_size);
            if (resize <= 0) {
                LOG << "read file ERROR or END, resize = " << resize;
                break;
            }

            QSharedPointer<FSDataBlock> block(new FSDataBlock);

            block->job_id = _jobid;
            block->file_id = fileid;
            block->filename = subname;
            block->blk_id = block_id;
            block->compressed = false;
            // copy binrary data
            fastring bufdata(buf, resize);
            block->data = bufdata;

            pushQueque(block);

            read_size += resize;
            block_id++;
        } while (read_size < file_size);

        free(buf);
        fd.close();
    });
}

void TransferJob::handleBlockQueque()
{
    // 定时获取状态并通知，检测是否结束
    UNIGO([this]() {
        bool exit = false;
        bool next_exit = false;
        int _max_count = 5;   // 接收文件最长时间 x秒，认为异常（网络断开或对端退出）
        do {
            exit = _jobCanceled;
            co::sleep(1000);   // 每秒检测发送一次状态
            bool empty = this->syncHandleStatus();   //检测一次
            if (empty) {
                if (_waitfinish) {
                    if (next_exit) {
                        // 已经被标记等待结束，这里定时更新，说明一定时间内无数据，直接结束
                        handleJobStatus(JOB_TRANS_FINISHED);
                        break;
                    } else {
                        next_exit = true;   //标记等待下一次状态检测退出
                        continue;
                    }
                }
                if (_writejob) {
                    // 写作业，计算等待时间
                    _max_count--;
                    if (_max_count < 0) {
                        DLOG << " wait block data timeout NOW!";
                        handleJobStatus(JOB_TRANS_FINISHED);
                        exit = true;
                    }
                } else {
                    DLOG << "all file finished, send job finish.";
                    handleUpdate(FINIASH, _path.c_str(), "");
                    _waitfinish = true;
                }
            } else {
                // reset timeout
                _max_count = 5;
            }
        } while (!exit);

        this->stop();
    });

    bool exception = false;
    while (!_stoped) {
        auto block = popQueue();
        if (block.isNull()) {
            co::sleep(10);
            continue;
        }

        int32 job_id = block->job_id;
        int32 file_id = block->file_id;
        uint64 blk_id = block->blk_id;
        fastring name = path::join(DaemonConfig::instance()->getStorageDir(), block->filename);
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
            FileTransBlock file_block;
            file_block.set_job_id(job_id);
            file_block.set_file_id(file_id);
            file_block.set_filename(block->filename.c_str());
            file_block.set_blk_id(static_cast<uint>(blk_id));
            file_block.set_data(buffer.c_str(), len);
            file_block.set_compressed(comp);
            // DLOG << "(" << job_id << ") send block " << block->filename << " size: " << len;

            int send = _rpcBinder->doSendFileBlock(_tar_app_name.c_str(), file_block);
            if (send <= 0) {
                ELOG << "rpc disconnect, break : " << name;
                exception = true;
                break;
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
    } else {
        LOG << "trans job finished: " << _jobid;
        _finished = true;
    }
}

void TransferJob::handleUpdate(FileTransRe result, const char *path, const char *emsg)
{
    UNIGO([this, result, path, emsg] {
        FileTransJobReport *report = new FileTransJobReport();
        FileTransUpdate update;

        report->set_job_id(_jobid);
        report->set_path(path);
        report->set_result(result);
        report->set_error(emsg);

        update.set_allocated_report(report);
        int res = _rpcBinder->doUpdateTrans(_tar_app_name.c_str(), update);
        if (res <= 0) {
            ELOG << "update failed: " << _path << " result=" << result;
        }
    });
}

bool TransferJob::syncHandleStatus()
{
    // update the file info
    for (auto &pair : _file_info_maps) {
        //DLOG << "syncHandleStatus()" << pair.second.name;
        if (pair.second.time_spended >= 0) {
            pair.second.time_spended += 1;
            bool end = pair.second.current_size >= pair.second.total_size;
            handleTransStatus(end ? FILE_TRANS_END : FILE_TRANS_SPEED, pair.second);
            if (end) {
                DLOG << "should notify file finish: " << pair.second.name;
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
    QString savepath(_savedir.c_str());

    emit notifyJobResult(appname, _jobid, status, savepath);
}

void TransferJob::handleTransStatus(int status, FileInfo &info)
{
    co::Json filejson = info.as_json();
    //update the file relative to abs path
    fastring savedpath = path::join(DaemonConfig::instance()->getStorageDir(), info.name);
    filejson.remove("name");
    filejson.add_member("name", savedpath);
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
