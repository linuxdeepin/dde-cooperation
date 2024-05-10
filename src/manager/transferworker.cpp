// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferworker.h"
#include "sessionmanager.h"

#include "common/logger.h"

#include <QFile>
#include <QStorageInfo>

TransferWorker::TransferWorker(const std::shared_ptr<AsioService> &service, QObject *parent)
    : QObject(parent)
    , _service(service)
{

}



// return true -> cancel
bool TransferWorker::onProgress(const std::string &path, uint64_t current, uint64_t total)
{
//    LOG_IF(FLG_log_detail) << "progressbar: " << progressbar << " remain_time=" << remain_time;
//    LOG_IF(FLG_log_detail) << "all_total_size: " << _file_stats.all_total_size << " all_current_size=" << _file_stats.all_current_size;
//    emit TransferHelper::instance()->transferContent(tr("Transfering"), filepath, progressbar, remain_time);
    LOG << "path: " << path << " current=" << current << " total=" << total;

    return false;
}

void TransferWorker::onWebChanged(int state, std::string msg)
{

}

void TransferWorker::stop()
{
    cancel(true);
    cancel(false);
}

bool TransferWorker::tryStartSend(QStringList paths, int port, std::vector<std::string> *nameVector)
{
    // first try run web, or prompt error
    if (!startWeb(port)) {
        ELOG << "try to start web sever failed!!!";
        return false;
    }

    _file_server->clearBind();
//    std::vector<std::string> name_vector;
    for (auto path : paths) {
        QFileInfo fileInfo(path);
        QString name = fileInfo.fileName();
        nameVector->push_back(name.toStdString());
        _file_server->webBind(name.toStdString(), path.toStdString());
    }

    return true;
}

bool TransferWorker::tryStartReceive(QStringList names, QString &ip, int port, QString &token, QString &dir)
{
    if (!startGet(ip.toStdString(), port)) {
        ELOG << "try to create http Geter failed!!!";
        return false;
    }

    _file_client->cancel(false);
    _file_client->setConfig(token.toStdString(), dir.toStdString());
    DLOG << "Download Names: ";
    for (const auto name : names) {
        _file_client->downloadFolder(name.toStdString());
    }
    return true;
}

void TransferWorker::cancel(bool send)
{
    if (send) {
        if (_file_server) {
            _file_server->clearBind();
            _file_server->clearToken();
            _file_server->stop();
        }
    } else {
        if (_file_client && _file_client->downloading()) {
            _file_client->cancel();
        }
    }
}

bool TransferWorker::isSyncing()
{
    return true;
}


bool TransferWorker::startWeb(int port)
{
    auto asioService = _service.lock();
    if (!asioService) {
        WLOG << "weak asio service";
        return false;
    }

    // Create a new file http server
    if (!_file_server) {
        _file_server = std::make_shared<FileServer>(asioService, port);

        auto self(this->shared_from_this());
        _file_server->setCallback(self);
    }

    return _file_server->start();
}

bool TransferWorker::startGet(const std::string &address, int port)
{
    auto asioService = _service.lock();
    if (!asioService) {
        WLOG << "weak asio service";
        return false;
    }

    // Create a new file http client
    if (!_file_client) {
        _file_client = std::make_shared<FileClient>(asioService, address, port); //service, address, port

        auto self(this->shared_from_this());
        _file_client->setCallback(self);
    }

    return true;
}
