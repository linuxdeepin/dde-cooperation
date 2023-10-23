// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "global_defines.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"

#include <co/rpc.h>
#include <co/co.h>

#include <QApplication>
#include <QFileDialog>
#include <QTime>
#include <QTimer>
#include <QDebug>

inline constexpr int TransferJobStartId = 1000;

using namespace cooperation_transfer;

TransferHelper::TransferHelper(QObject *parent)
    : QObject(parent)
{
    thisDestruct = false;
}

TransferHelper::~TransferHelper()
{
    thisDestruct = true;
}

void TransferHelper::localIPCStart()
{
    if (frontendIpcSer) return;

    frontendIpcSer = new FrontendService(this);

    go([this]() {
        while(!thisDestruct) {
            BridgeJsonData bridge;
            frontendIpcSer->bridgeChan()->operator>>(bridge); //300ms超时
            if (!frontendIpcSer->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

//            LOG << "get bridge json: " << bridge.type << " json:" << bridge.json;
            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                qWarning() << "parse error from: " << bridge.json.c_str();
                continue;
            }
            switch (bridge.type) {
            case PING:
            {
                ipc::PingFrontParam param;
                param.from_json(json_obj);

                bool result = false;
                fastring my_ver(FRONTEND_PROTO_VERSION);
                if (my_ver.compare(param.version) == 0 && param.session.compare(sessionId.toStdString()) == 0) {
                    result = true;
                } else {
                    qWarning() << param.version.c_str() << " =version not match= " << my_ver.c_str();
                }

                BridgeJsonData res;
                res.type = PING;
                res.json = result ? param.session : ""; // 成功则返回session，否则为空

                frontendIpcSer->bridgeResult()->operator<<(res);
                break;
            }
            case MISC_MSG:
            {
                QString json(bridge.json.c_str());
                onMiscMessage(json);
                break;
            }
            case FRONT_PEER_CB:
            {
                ipc::GenericResult param;
                param.from_json(json_obj);
                // example to parse string to NodePeerInfo object
                NodePeerInfo peerobj;
                peerobj.from_json(param.msg);

                qInfo() << param.result << " peer : " << param.msg.c_str();

                break;
            }
            case FRONT_CONNECT_CB:
            {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString mesg(param.msg.c_str());
                onConnectStatusChanged(param.result, mesg);
                break;
            }
            case FRONT_TRANS_STATUS_CB:
            {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString mesg(param.msg.c_str()); // job path

                onTransJobStatusChanged(param.id, param.result, mesg);
                break;
            }
            case FRONT_FS_PULL_CB:
            {
                break;
            }
            case FRONT_FS_ACTION_CB:
            {
                break;
            }
            case FRONT_NOTIFY_FILE_STATUS:
            {
                QString objstr(bridge.json.c_str());
                onFileTransStatusChanged(objstr);
                break;
            }
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(frontendIpcSer);
    rpc::Server().add_service(frontendimp)
                 .start("0.0.0.0", UNI_IPC_FRONTEND_PORT, "/frontend", "", "");
}

void TransferHelper::init()
{
    localIPCStart();
    connect(&transferDialog, &TransferDialog::cancel, this, &TransferHelper::cancelTransfer);

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

void TransferHelper::sendFiles(const QString &ip, const QString &devName, const QStringList &fileList)
{
    sendToWho = devName;
    readyToSendFiles = fileList;
    if (fileList.isEmpty())
        return;

    status = Connecting;
    canTransfer = true;
    go([ip, this] {
        this->handleTryConnect(ip);
    });

    transferDialog.switchWaitConfirmPage();
    transferDialog.exec();
}

TransferHelper::TransferStatus TransferHelper::transferStatus()
{
    return status;
}

void TransferHelper::buttonClicked(const QVariantMap &info)
{
    auto id = info.value("id").toString();
    auto ip = info.value("ip").toString();
    auto devName = info.value("device").toString();

    if (id == kTransferId) {
        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty())
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());

        if (selectedFiles.isEmpty())
            return;

        TransferHelper::instance()->sendFiles(ip, devName, selectedFiles);
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
        if (!canTransfer) {
            status = Idle;
            return;
        }

        status = Transfering;
        QString title = tr("Sending files to \"%1\"").arg(sendToWho);
        transferDialog.switchProgressPage(title);
        transferDialog.updateProgress(1, tr("calculating"));

        go([this] {
            this->handleSendFiles(readyToSendFiles);
        });
    } else {
        status = Idle;
        transferDialog.switchResultPage(false, "xxxxxxx");
    }
}

void TransferHelper::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    qInfo() << "======" << id << result << msg;
}

void TransferHelper::onFileTransStatusChanged(const QString &status)
{
    co::Json statusJson;
    statusJson.parse_from(status.toStdString());
    ipc::FileStatus param;
    param.from_json(statusJson);

    if (fileIds.contains(param.file_id)) {
        // 已经记录过，只更新数据
        int64_t increment = param.current - fileIds[param.file_id];
        transferInfo.transferSize += increment;   //增量值
        fileIds[param.file_id] = param.current;

        if (param.current >= param.total) {
            // 此文件已完成，从文件统计中删除
            fileIds.remove(param.file_id);
        }
    } else {
        transferInfo.totalSize += param.total;
        transferInfo.transferSize += param.current;
        fileIds.insert(param.file_id, param.current);
    }

    if (param.second > transferInfo.maxTimeSec)
        transferInfo.maxTimeSec = param.second;

    // 计算传输进度与剩余时间
    double value = static_cast<double>(transferInfo.transferSize) / transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        transferDialog.updateProgress(100, "00:00:00");

        QTimer::singleShot(1000, this, [this] {
            this->status = Idle;
            transferDialog.switchResultPage(true, tr("File sent successfully"));
        });
    } else {
        int remainTime = transferInfo.maxTimeSec * 100 / progressValue - transferInfo.maxTimeSec;
        QTime time(0, 0, 0);
        time = time.addSecs(remainTime);
        transferDialog.updateProgress(progressValue, time.toString("hh:mm:ss"));
    }
}

void TransferHelper::cancelTransfer()
{
    canTransfer = false;
    if (status != Transfering) {
        status = Idle;
        return;
    }

    go([this] {
        this->handleCancelTransfer();
    });
}

void TransferHelper::onMiscMessage(QString jsonmsg)
{
    qInfo() << "misc message arrived:" << jsonmsg;
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
    c->close();
    sessionId = res.get("msg").as_string().c_str();   // save the return session.

    //CallResult
    return res.get("result").as_bool() && !sessionId.isEmpty();
}

void TransferHelper::handleSendFiles(const QStringList &fileList)
{
    qInfo() << "send files: " << fileList;
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res, paths;

    for (QString path : fileList) {
        paths.push_back(path.toStdString());
    }

    //TransFilesParam
    req = {
        { "session", sessionId.toStdString() },
        { "id", TransferJobStartId },
        { "paths", paths },
        { "sub", true },
        { "savedir", sendToWho.toStdString() },
    };

    req.add_member("api", "Backend.tryTransFiles");   //BackendImpl::tryTransFiles

    c->call(req, res);
    c->close();
}

void TransferHelper::handleTryConnect(const QString &ip)
{
    qInfo() << "connect to " << ip;
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res;
    fastring targetIp(ip.toStdString());
    fastring pinCode("373336");
    QString appName = QCoreApplication::applicationName();

    req = {
        { "session", appName.toStdString() },
        { "host", targetIp },
        { "password", pinCode },
    };
    req.add_member("api", "Backend.tryConnect");
    c->call(req, res);
    c->close();
}

void TransferHelper::handleCancelTransfer()
{
    // TODO
}

void TransferHelper::handleSetConfig(const QString &key, const QString &value)
{
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res;

    QString appName = QCoreApplication::applicationName();

    req = {
        { "appname", appName.toStdString() },
        { "key", key.toStdString() },
        { "value", value.toStdString() },
    };
    req.add_member("api", "Backend.setAppConfig");
    c->call(req, res);
}

QString TransferHelper::handleGetConfig(const QString &key)
{
    co::pool_guard<rpc::Client> c(coPool.get());
    co::Json req, res;

    QString appName = QCoreApplication::applicationName();

    req = {
        { "appname", appName.toStdString() },
        { "key", key.toStdString() },
    };
    req.add_member("api", "Backend.getAppConfig");
    c->call(req, res);

    QString value = res.get("msg").as_string().c_str();

    return value;
}
