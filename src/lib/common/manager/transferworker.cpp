// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferworker.h"
#include "sessionmanager.h"

#include "common/log.h"

#include <QFile>
#include <QStorageInfo>

TransferWorker::TransferWorker(const std::shared_ptr<AsioService> &service, QObject *parent)
    : QObject(parent)
    , _service(service)
{
    connect(&_speedTimer, &QTimer::timeout, this, &TransferWorker::calculateSpeed);
}



// return true -> cancel
bool TransferWorker::onProgress(const std::string &path, uint64_t current, uint64_t total)
{
//    LOG_IF(FLG_log_detail) << "progressbar: " << progressbar << " remain_time=" << remain_time;
//    LOG_IF(FLG_log_detail) << "all_total_size: " << _file_stats.all_total_size << " all_current_size=" << _file_stats.all_current_size;
//    emit TransferHelper::instance()->transferContent(tr("Transfering"), filepath, progressbar, remain_time);
//    LOG << "path: " << path << " current=" << current << " total=" << total;
    _status.path = path;
    _status.total = total;
    _status.current = current;

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

void TransferWorker::updateSaveRoot(const QString &dir)
{
    _saveRoot = QString(dir);
}

bool TransferWorker::tryStartSend(QStringList paths, int port, std::vector<std::string> *nameVector, std::string *token)
{
    // first try run web, or prompt error
    if (!startWeb(port)) {
        ELOG << "try to start web sever failed!!!";
        return false;
    }

    picojson::array jsonArray;
    _file_server->clearBind();
    for (auto path : paths) {
        QFileInfo fileInfo(path);
//        QString name = "/" + fileInfo.fileName(); //must start /
        std::string name = fileInfo.fileName().toStdString();
        nameVector->push_back(name);
        LOG << "web bind (" << name << ") on dir: " << path.toStdString();
        _file_server->webBind(name, path.toStdString());

        jsonArray.push_back(picojson::value(name));
    }

    // 将picojson对象转换为字符串
    std::string jsonString = picojson::value(jsonArray).serialize();
    *token = _file_server->genToken(jsonString);

    _speedTimer.start(1000);

    return true;
}

bool TransferWorker::tryStartReceive(QStringList names, QString &ip, int port, QString &token, QString &dirname)
{
    if (!startGet(ip.toStdString(), port)) {
        ELOG << "try to create http Geter failed!!!";
        return false;
    }

    std::string accessToken = token.toStdString();
    _file_client->cancel(false);
    QString savePath = _saveRoot + QDir::separator();
    if (!dirname.isEmpty()) {
        savePath += dirname + QDir::separator();
    }
    _file_client->setConfig(accessToken, savePath.toStdString());

    std::vector<std::string> webs = _file_client->parseWeb(accessToken);
#ifdef QT_DEBUG
    for (const auto& web : webs) {
        DLOG << "Web: " << web;
    }
#endif
    _file_client->startFileDownload(webs);
    _speedTimer.start(1000);

    return true;
}

void TransferWorker::cancel(bool send)
{
    if (send) {
        if (_file_server) {
            _file_server->clearBind();
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

void TransferWorker::calculateSpeed()
{
    ++_status.second;

    if (_status.current < 0 && _status.second > 3) {
        // 3秒都没有更新，应该是异常了，停止
        _speedTimer.stop();
        return;
    }

    double speed = (static_cast<double>(_status.current)) / (_status.second * 1024 * 1024); // 计算下载速度，单位为兆字节/秒
    QString formattedSpeed = QString::number(speed, 'f', 2); // 格式化速度为保留两位小数的字符串
    DLOG << "Transfer speed: " << formattedSpeed.toStdString() << " M/s";

    if (_status.current >= _status.total) {
        _speedTimer.stop();
    }
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
