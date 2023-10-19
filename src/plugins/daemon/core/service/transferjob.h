// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERJOB_H
#define TRANSFERJOB_H

#include <QObject>
#include <service/rpc/remoteservice.h>
#include <ipc/proto/chan.h>
#include "co/co.h"
#include "co/time.h"

class TransferJob : public QObject
{
    Q_OBJECT

public:
    explicit TransferJob(QObject *parent = nullptr);
    void initRpc(fastring target, uint16 port);
    void initJob(fastring appname, int id, fastring path, bool sub, fastring savedir, bool write);

    void start();
    void stop();
    void waitFinish();
    bool finished();

    bool isRunning();
    bool isWriteJob();
    fastring getAppName();

    void pushQueque(FSDataBlock &block);
    void insertFileInfo(FileInfo &info);

signals:
    // 传输作业结果通知：文件（目录），结果，保存路径
    void notifyJobResult(QString appname, int jobid, int status, QString savedir);

    // 传输文件状态
    void notifyFileTransStatus(QString appname, int status, QString fileinfo);

public slots:

private:
    fastring getSubdir(const char *path);
    void readPath(const char *path, int id);
    bool readFile(const char *filepath, int fileid);
    void readFileBlock(const char *filepath, int fileid, const fastring subname);

    void handleBlockQueque();

    void handleUpdate(FileTransRe result, const char *path, const char *emsg);
    bool syncHandleStatus();
    void handleJobStatus(int status);
    void handleTransStatus(int status, FileInfo &info);

private:
    int _jobid;
    bool _inited = false;
    bool _stoped = true;
    bool _finished = false;
    bool _waitfinish = false;

    bool _sub;
    bool _writejob;
    fastring _app_name; // //前端应用名
    fastring _path; // 目录或文件路径
    fastring _savedir; // 写作业，文件保存的目录

    RemoteServiceBinder *_rpcBinder = nullptr;

    co::mutex _queque_mutex;
    co::deque<FSDataBlock> _block_queue;
    co::map<int32, FileInfo> _file_info_maps;

    int _empty_max_count = 5; // 接收文件最长时间 x秒，认为异常（网络断开或对端退出）
};

#endif // TRANSFERJOB_H
