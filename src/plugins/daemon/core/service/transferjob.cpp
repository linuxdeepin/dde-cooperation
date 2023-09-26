// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferjob.h"
#include "co/log.h"
#include "co/fs.h"

TransferJob::TransferJob(QObject *parent) : QObject(parent)
{

}

void TransferJob::initJob(int id, QStringList paths, bool sub, QString savedir)
{
    _id = id;
    _paths = paths;
    _sub = sub;
    _savedir = savedir;

    _inited = true;
}

void TransferJob::startJob(bool push, RemoteServiceBinder *binder)
{
    _stoped = false;
    _finished = false;
    int id = 0;
    for (QString path : _paths) {
        QByteArray byteArray = path.toUtf8();
        const char *filepath = byteArray.constData();

        if (push) {
            trySend(filepath, id++, binder);
        }

        if (_stoped) {
            break;
        }
    }
    _finished = true;
}

void TransferJob::stop()
{
    _stoped = true;
}

bool TransferJob::finished()
{
    return _finished;
}


void TransferJob::trySend(const char *path, int id, RemoteServiceBinder *binder)
{
    int file_id = id;
    if (fs::isdir(path)) {
        fs::dir d(path);
        auto v = d.all(); // 读取所有子项
        for (const fastring &file : v) {
            file_id++;
            if (fs::isdir(file)) {
                //TODO: remote create dir
                trySend(file.c_str(), file_id, binder);
            } else {
                int res = binder->doPushfileJob(file_id, file.c_str());
                if (res < 0) {
                    ELOG << "binder doPushfileJob failed: " << res << " filepath: " << file;
                    continue;
                }
            }

            if (_stoped) {
                break;
            }
        }
    } else {
        int res = binder->doPushfileJob(file_id, path);
        if (res < 0) {
            ELOG << "binder doPushfileJob failed: " << res << " filepath: " << path;
        }
    }
}
