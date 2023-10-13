// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "global_defines.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"

#include <co/rpc.h>
#include <co/co.h>

#include <QApplication>
#include <QFileDialog>
#include <QDebug>

using namespace cooperation_transfer;

TransferHelper::TransferHelper(QObject *parent)
    : QObject(parent)
{
}

void TransferHelper::init()
{
    frontendIpcSer = new FrontendService(this);

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(frontendIpcSer);
    rpc::Server().add_service(frontendimp).start("0.0.0.0", UNI_IPC_FRONTEND_PORT, "/frontend", "", "");

    connect(frontendIpcSer, &FrontendService::sigConnectStatus, this, &TransferHelper::onConnectStatusChanged, Qt::QueuedConnection);
    connect(frontendIpcSer, &FrontendService::sigTransJobtatus, this, &TransferHelper::onTransJobStatusChanged, Qt::QueuedConnection);
    connect(frontendIpcSer, &FrontendService::sigFileTransStatus, this, &TransferHelper::onFileTransStatusChanged, Qt::QueuedConnection);

    coPool = std::shared_ptr<co::pool>(new co::pool(
            []() { return reinterpret_cast<void *>(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false)); },
            [](void *p) { delete reinterpret_cast<rpc::Client *>(p); }));

    go([this] {
        backendOk = handlePingBacked();
    });
}

TransferHelper *TransferHelper::instance()
{
    static TransferHelper ins;
    return &ins;
}

void TransferHelper::sendFiles(const QString &ip, const QStringList &fileList)
{
    readyToSendFiles = fileList;
    if (fileList.isEmpty())
        return;

    status = Connecting;
    go([ip, this] {
        this->handleTryConnect(ip);
    });
}

TransferHelper::TransferStatus TransferHelper::transferStatus()
{
    return status;
}

void TransferHelper::buttonClicked(const QVariantMap &info)
{
    auto id = info.value("id").toString();
    auto ip = info.value("ip").toString();

    if (id == kTransferId) {
        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty())
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());

        if (selectedFiles.isEmpty())
            return;

        TransferHelper::instance()->sendFiles(ip, selectedFiles);
    } else if (id == kHistoryId) {
        // TODO:
    }
}

bool TransferHelper::buttonVisible(const QVariantMap &info)
{
    auto appName = qAppName();
    auto id = info.value("id").toString();
    int state = info.value("state").toInt();
    if ("dde-cooperation" != qAppName() && id == kHistoryId)
        return false;

    if (state == 2 && id == kTransferId)
        return false;

    return true;
}

bool TransferHelper::buttonClickable(const QVariantMap &info)
{
    auto id = info.value("id").toString();
    if (id == kTransferId)
        return TransferHelper::instance()->transferStatus() == Idle;

    return true;
}

void TransferHelper::onConnectStatusChanged(int result, const QString &msg)
{
    qInfo() << "connect status: " << result << " msg:" << msg;
    if (result > 0) {
        status = Transfering;
        go([this] {
            this->handleSendFiles(readyToSendFiles);
        });
    } else {
        status = Idle;
    }

    transferDialog.switchResultPage(false, "xxx");
}

void TransferHelper::onTransJobStatusChanged(int id, int result, const QString &msg)
{
}

void TransferHelper::onFileTransStatusChanged(const QString &status)
{
}

bool TransferHelper::handlePingBacked()
{
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res;
    //PingBackParam
    req = {
        { "who", qAppName().toStdString() },
        { "version", UNI_IPC_PROTO },
        { "cb_port", UNI_IPC_FRONTEND_PORT },
    };

    req.add_member("api", "Backend.ping");   //BackendImpl::ping

    c->call(req, res);
    sessionId = res.get("msg").as_string().c_str();   // save the return session.

    //CallResult
    return res.get("result").as_bool();
}

void TransferHelper::handleSendFiles(const QStringList &fileList)
{
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res, paths;

    for (QString path : fileList) {
        paths.push_back(path.toStdString());
    }

    //TransFilesParam
    req = {
        { "session", sessionId.toStdString() },
        { "id", 0 },   // TODO: set trans job id
        { "paths", paths },
        { "sub", true },
        { "savedir", "" },
    };

    req.add_member("api", "Backend.tryTransFiles");   //BackendImpl::tryTransFiles

    c->call(req, res);
}

void TransferHelper::handleTryConnect(const QString &ip)
{
    qInfo() << "connect to " << ip;
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res;
    fastring targetIp(ip.toStdString());
    fastring pinCode("373336");

    req = {
        { "session", sessionId.toStdString() },
        { "host", targetIp },
        { "password", pinCode },
    };
    req.add_member("api", "Backend.tryConnect");
    c->call(req, res);
}
