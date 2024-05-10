// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include "session/asioservice.h"
#include "filesync/fileserver.h"
#include "filesync/fileclient.h"

#include <QObject>

class TransferWorker : public QObject, public ProgressCallInterface
{
    Q_OBJECT
    struct file_stats_s {
        int64_t all_total_size;   // 总量
        int64_t all_current_size;   // 当前已接收量
        int64_t cast_time_ms;   // 最大已用时间
    };

public:
    explicit TransferWorker(const std::shared_ptr<AsioService> &service, QObject *parent = nullptr);

    bool onProgress(const std::string &path, uint64_t current, uint64_t total) override;

    void onWebChanged(int state, std::string msg) override;

    void stop();

    bool tryStartSend(QStringList paths, int port, std::vector<std::string> *nameVector);
    bool tryStartReceive(QStringList names, QString &ip, int port, QString &token, QString &dir);

    void cancel(bool send);
    bool isSyncing();

signals:

public slots:

private:
    bool startWeb(int port);
    bool startGet(const std::string &address, int port);

    std::weak_ptr<AsioService> _service;

    // file sync service and client
    std::shared_ptr<FileServer> _file_server { nullptr };
    std::shared_ptr<FileClient> _file_client { nullptr };

    // <jobid, jobpath>
//    QMap<int, QString> _job_maps;
//    int _request_job_id;

//    QString _accessToken = "";
//    QString _saveDir = "";
//    QString _connectedAddress = "";
};

#endif // TRANSFERWORKER_H
