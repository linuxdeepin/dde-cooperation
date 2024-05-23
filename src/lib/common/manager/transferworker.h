// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include "session/asioservice.h"
#include "httpweb/fileserver.h"
#include "httpweb/fileclient.h"

#include <QObject>
#include <QTimer>

class TransferWorker : public QObject, public ProgressCallInterface
{
    Q_OBJECT
    struct file_stats_s {
        std::string path;
        int64_t total {0};   // 总量
        int64_t current {-1};   // 当前已接收量
        int64_t second {0};   // 最大已用时间
    };

public:
    explicit TransferWorker(const std::shared_ptr<AsioService> &service, QObject *parent = nullptr);

    bool onProgress(const std::string &path, uint64_t current, uint64_t total) override;

    void onWebChanged(int state, std::string msg) override;

    void stop();

    void updateSaveRoot(const QString &dir);

    bool tryStartSend(QStringList paths, int port, std::vector<std::string> *nameVector, std::string *token);
    bool tryStartReceive(QStringList names, QString &ip, int port, QString &token, QString &dirname);

    void cancel(bool send);
    bool isSyncing();

signals:

public slots:
    void calculateSpeed();

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
    QString _saveRoot = "";
//    QString _connectedAddress = "";
    QTimer _speedTimer;

    file_stats_s _status;
    bool _startTrans { false };
};

#endif // TRANSFERWORKER_H
