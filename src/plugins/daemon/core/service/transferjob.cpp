// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferjob.h"
#include "co/log.h"
#include "co/fs.h"
#include "co/path.h"
#include "co/co.h"
#include "common/constant.h"
#include "fsadapter.h"
#include "utils/config.h"

TransferJob::TransferJob(QObject *parent) : QObject(parent)
{
    _block_queue.clear();
    _file_info_maps.clear();
}

void TransferJob::initRpc(fastring target, uint16 port)
{
    if (nullptr == _rpcBinder) {
        _rpcBinder = new RemoteServiceBinder(this);
        _rpcBinder->createExecutor(target.c_str(), port);
    }
}

void TransferJob::initJob(int id, fastring path, bool sub, fastring savedir)
{
    _jobid = id;
    _path = path;
    _sub = sub;
    if (!savedir.empty()) {
        _savedir = savedir;
        _writejob = true;
    } else {
        _writejob = false;
    }

    _inited = true;
}

void TransferJob::start()
{
    _stoped = false;
    _finished = false;

    const char *jobpath = _path.c_str();
    if (_writejob) {
        DLOG << "start write job: " << _savedir;
    } else {
        //并行读取文件数据
        fastring path; // file save path
        if (fs::isdir(jobpath)) {
            path = path::base(jobpath);
        } else {
            path = ""; // 文件没有指定保存目录，默认目标机设置的保存目录
        }
        int res = _rpcBinder->doTransfileJob(_jobid, path.c_str(), false, true, _writejob);
        if (res < 0) {
            ELOG << "binder doTransfileJob failed: " << res << " jobpath: " << jobpath;
            _stoped = true;
            emit notifyJobResult(QString(jobpath), false, 0);
        }

        readPath(jobpath, _jobid);
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

void TransferJob::pushQueque(FSDataBlock &block)
{
    co::mutex_guard g(_queque_mutex);
    _block_queue.push_back(block);
}

void TransferJob::insertFileInfo(FileInfo &info)
{
    int fileid = info.file_id;
    auto it = _file_info_maps.find(fileid);
    if (it == _file_info_maps.end()) {
        DLOG << "insertFileInfo new file INFO: " << fileid;
        // skip 0B file
        if (info.total_size > 0) {
            _file_info_maps.insert(std::make_pair(fileid, info));
        }
    }
}

fastring TransferJob::getSubdir(const char *path)
{
    fastring topdir = "";
    std::pair<fastring, fastring> pairs = path::split(path);
    fastring filedir = pairs.first;

    if (fs::isdir(_path)) {
        // 传输文件夹，文件夹为保存子目录。
        topdir = path::base(_path);
    }
    fastring indir = str::trim(filedir, _path.c_str(), 'l');
    fastring subdir = path::join(topdir.c_str(), indir.c_str());

    return subdir;
}

void TransferJob::readPath(const char *path, int id)
{
    int file_id = id;
    if (fs::isdir(path)) {
        fs::dir d(path);
        auto v = d.all(); // 读取所有子项
        for (const fastring &file : v) {
            file_id++;
            fastring file_path = path::join(d.path(), file.c_str());
            if (fs::isdir(file_path) && _sub) {
                // create sub dir name
                fastring subdir = getSubdir(file_path.c_str());
                int res = _rpcBinder->doSendFileInfo(_jobid, file_id, subdir.c_str(), file_path.c_str());
                if (res <= 0) {
                    ELOG << "fail to create dir info : " << path;
                }

                readPath(file_path.c_str(), file_id);
            } else {
                if (!readFile(file_path.c_str(), file_id)) {
                    continue;
                }
            }

            if (_stoped) {
                // job has been canceled
                QString jobpath(_path.c_str());
                emit notifyJobResult(jobpath, false, 0);
                break;
            }
        }
    } else {
        readFile(path, file_id);
    }
}

bool TransferJob::readFile(const char *filepath, int fileid)
{
    std::pair<fastring, fastring> pairs = path::split(filepath);
    fastring filename = pairs.second;

    fastring subdir = getSubdir(filepath);
    int res = _rpcBinder->doSendFileInfo(_jobid, fileid, subdir.c_str(), filepath);
    if (res <= 0) {
        ELOG << "error file info : " << filepath;
        return false;
    }

    fastring subname = path::join(subdir.c_str(), filename.c_str());
    readFileBlock(filepath, fileid, subname);
    return true;
}

void TransferJob::readFileBlock(const char *filepath, int fileid, const fastring subname)
{
    if (nullptr == filepath || fileid < 0) {
        return;
    }
    go ([this, filepath, fileid, subname]() {
        size_t block_size = BLOCK_SIZE;
        int64 file_size = fs::fsize(filepath);
        int64 read_size = 0;
        uint32 block_id = 0;

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
        }

        fs::file fd(filepath, 'r');
        if (!fd) {
            ELOG << "open file failed: " << filepath;
            return;
        }

         LOG << "======this file (" << subname <<") size=" << file_size;
        size_t block_len = block_size * sizeof(char);
        char* buf = reinterpret_cast<char*>(malloc(block_len));
        do {
            memset(buf, 0, block_len);
            size_t resize = fd.read(buf, block_size);
            if (resize <= 0) {
                LOG << "read file ERROR or END, resize = " << resize;
                break;
            }

            FSDataBlock block;

            block.job_id = _jobid;
            block.file_id = fileid;
            block.filename = subname;
            block.blk_id = block_id;
            block.compressed = false;
            // copy binrary data
            fastring bufdata(buf, resize);
            block.data = bufdata;

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
    _checkerTimer.run_every(std::bind([this]() {
        // update the file info
        for (auto& pair : _file_info_maps) {
            if (pair.second.time_spended >= 0) {
                pair.second.time_spended += 1;

                // double speed = pair.second.current_size / (1024 * 1024) / pair.second.time_spended;
                // LOG << pair.second.name << " (" << pair.second.current_size << ") speed: " << speed << " M/s";

                if (pair.second.current_size >= pair.second.total_size) {
                    DLOG << "should notify file finish: " << pair.second.name;
                    _file_info_maps.erase(pair.first);
                    if (!_writejob) {
                    //TODO: check sum
                    // handleUpdate(OK, block.filename.c_str(), "");
                    }
                }
           }
        }
        if (_file_info_maps.empty()) {
            if (!_writejob) {
                DLOG << "all file finished, send job finish.";
                handleUpdate(FINIASH, _path.c_str(), "");
            } else {
                _empty_max_count--;
                if (_empty_max_count < 0) {
                    DLOG << " wait block data timeout NOW!";
                    this->stop();
                }
            }
        } else {
            //reset timeout
            _empty_max_count = 5;
        }
    }), 1);

    bool exception = false;
    while (!_stoped) {
        if (_block_queue.empty()) {
            // job has been sat wait finish.
            if (_waitfinish) {
                break;
            }
            co::sleep(10);
            continue;
        }
        FSDataBlock block = _block_queue.front();

        int32 job_id = block.job_id;
        int32 file_id = block.file_id;
        uint32 blk_id = block.blk_id;
        fastring name = path::join(DaemonConfig::instance()->getStorageDir(), block.filename);
        fastring buffer = block.data;
        size_t len = buffer.size();
        bool comp = block.compressed;

        if (_writejob) {
            int64 offset = blk_id * BLOCK_SIZE;
//            ELOG << "file : " << name << " write : " << len;
            bool good = FSAdapter::writeBlock(name.c_str(), offset, buffer.data(), len);
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
            file_block.set_filename(block.filename.c_str());
            file_block.set_blk_id(blk_id);
            file_block.set_data(buffer.data(), len);
            file_block.set_compressed(comp);
//            DLOG << "(" << job_id << ") send block " << block.filename << " size: " << len;

            int send = _rpcBinder->doSendFileBlock(file_block);
            if (send <= 0) {
                ELOG << "rpc disconnect, break : " << name;
                exception = true;
                break;
            }
        }
        _block_queue.pop_front();

        // update the file info
        auto it = _file_info_maps.find(file_id);
        if (it != _file_info_maps.end()) {
            it->second.current_size += len;
            if (blk_id == 0) {
                // first block data, file begin, start record time
                it->second.time_spended = 0;
            }
        }
    };
    if (exception) {
        DLOG << "trans job exception hanpend";
    } else {
        LOG << "trans job finished!!";
        _finished = true;
    }
    _checkerTimer.stop();
}

void TransferJob::handleUpdate(FileTransRe result, const char *path, const char *emsg)
{
    go ([this, result, path, emsg] {
        FileTransJobReport *report = new FileTransJobReport();
        FileTransUpdate update;

        report->set_job_id(_jobid);
        report->set_path(path);
        report->set_result(result);
        report->set_error(emsg);

        update.set_allocated_report(report);
        int res = _rpcBinder->doUpdateTrans(update);
        if (res <= 0) {
            ELOG << "update failed: " << _path << " result=" << result;
        }

        if (FINIASH == result) {
            this->stop();
        }
    });
}
