// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERJOB_H
#define TRANSFERJOB_H

#include <QObject>
#include <QQueue>
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
    void initJob(fastring appname, fastring targetappname, int id, fastring path, bool sub, fastring savedir, bool write);

    void start();
    void stop();
    void waitFinish();
    bool finished();
    bool isRunning();
    bool isWriteJob();
    fastring getAppName();

    void cancel();

    void pushQueque(const QSharedPointer<FSDataBlock> block);
    void insertFileInfo(FileInfo &info);

signals:
    // 传输作业结果通知：文件（目录），结果，保存路径
    void notifyJobResult(QString appname, int jobid, int status, QString savedir);

    // 传输文件状态
    void notifyFileTransStatus(QString appname, int status, QString fileinfo);

public slots:

private:
    fastring getSubdir(const char *path, const char *root);
    void scanPath(fastring root, fastring path);
    void readPath(fastring path, fastring root);
    bool readFile(fastring filepath, int fileid, fastring subdir);
    void readFileBlock(fastring filepath, int fileid, const fastring subname);

    void handleBlockQueque();

    void handleUpdate(FileTransRe result, const char *path, const char *emsg);
    bool syncHandleStatus();
    void handleJobStatus(int status);
    void handleTransStatus(int status, FileInfo &info);
    QSharedPointer<FSDataBlock> popQueue();

private:
    int _jobid;
    int _fileid = 0;
    bool _inited = false;
    bool _stoped = true;
    bool _finished = false;
    bool _waitfinish = false;

    bool _sub;
    bool _writejob;
    fastring _app_name; // //前端应用名
    fastring _path; // 目录或文件路径
    fastring _savedir; // 写作业，文件保存的目录
    fastring _tar_app_name; // 发送到目标的应用名称

    RemoteServiceBinder *_rpcBinder = nullptr;

    co::mutex _queque_mutex;
    QQueue<QSharedPointer<FSDataBlock>> _block_queue;
    co::map<int32, FileInfo> _file_info_maps;
};

#endif // TRANSFERJOB_H
