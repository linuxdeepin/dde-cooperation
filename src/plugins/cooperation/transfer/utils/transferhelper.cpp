// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "transferhelper_p.h"
#include "config/configmanager.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QTimer>
#include <QDebug>

inline constexpr int TransferJobStartId = 1000;
inline constexpr int CooperationPort = UNI_IPC_FRONTEND_COOPERATION_PORT + 1;
inline constexpr int TransferPort = UNI_IPC_FRONTEND_TRANSFER_PORT + 1;
inline constexpr char NotifyServerName[] { "org.freedesktop.Notifications" };
inline constexpr char NotifyServerPath[] { "/org/freedesktop/Notifications" };
inline constexpr char NotifyServerIfce[] { "org.freedesktop.Notifications" };

using namespace cooperation_transfer;

TransferHelperPrivate::TransferHelperPrivate(TransferHelper *qq)
    : QObject(qq),
      q(qq)
{
    transferDialog = new TransferDialog();
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), q);

    connect(transferDialog, &TransferDialog::cancel, q, &TransferHelper::cancelTransfer);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, q, &TransferHelper::onConfigChanged);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
}

TransferHelperPrivate::~TransferHelperPrivate()
{
    delete transferDialog;
}

void TransferHelperPrivate::initConfig()
{
    auto storagePath = ConfigManager::instance()->appAttribute("GenericAttribute", kStoragePathKey);
    handleSetConfig(KEY_APP_STORAGE_DIR, storagePath.isValid() ? storagePath.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    auto settings = ConfigManager::instance()->appAttribute("GenericAttribute", kTransferModeKey);
    handleSetConfig(kTransferModeKey, settings.isValid() ? QString::number(settings.toInt()) : "0");
}

void TransferHelperPrivate::localIPCStart()
{
    if (frontendIpcSer) return;

    frontendIpcSer = new FrontendService(this);

    go([this]() {
        while (!thisDestruct) {
            BridgeJsonData bridge;
            frontendIpcSer->bridgeChan()->operator>>(bridge);   //300ms超时
            if (!frontendIpcSer->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                qWarning() << "parse error from: " << bridge.json.c_str();
                continue;
            }

            switch (bridge.type) {
            case PING: {
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
                res.json = result ? param.session : "";   // 成功则返回session，否则为空

                frontendIpcSer->bridgeResult()->operator<<(res);
                break;
            }
            case FRONT_CONNECT_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString msg(param.msg.c_str());

                metaObject()->invokeMethod(q, "onConnectStatusChanged",
                                           Qt::QueuedConnection,
                                           Q_ARG(int, param.result),
                                           Q_ARG(QString, msg));
            } break;
            case FRONT_TRANS_STATUS_CB: {
                ipc::GenericResult param;
                param.from_json(json_obj);
                QString msg(param.msg.c_str());   // job path

                metaObject()->invokeMethod(q, "onTransJobStatusChanged",
                                           Qt::QueuedConnection,
                                           Q_ARG(int, param.id),
                                           Q_ARG(int, param.result),
                                           Q_ARG(QString, msg));
            } break;
            case FRONT_NOTIFY_FILE_STATUS: {
                QString objstr(bridge.json.c_str());
                metaObject()->invokeMethod(q, "onFileTransStatusChanged", Qt::QueuedConnection, Q_ARG(QString, objstr));
            } break;
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(frontendIpcSer);

    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    rpc::Server().add_service(frontendimp).start("0.0.0.0", onlyTransfer ? TransferPort : CooperationPort, "/frontend", "", "");
}

bool TransferHelperPrivate::handlePingBacked()
{
    co::Json req, res;
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    QString name = qApp->applicationName() + "-temp";
    //PingBackParam
    req = {
        { "who", name.toStdString() },
        { "version", UNI_IPC_PROTO },
        { "cb_port", onlyTransfer ? TransferPort : CooperationPort },
    };

    req.add_member("api", "Backend.ping");   //BackendImpl::ping

    rpcClient->call(req, res);
    sessionId = res.get("msg").as_string().c_str();   // save the return session.

    //CallResult
    return res.get("result").as_bool() && !sessionId.isEmpty();
}

void TransferHelperPrivate::handleSendFiles(const QStringList &fileList)
{
    qInfo() << "send files: " << fileList;
    co::Json req, res, paths;

    for (QString path : fileList) {
        paths.push_back(path.toStdString());
    }

    auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
    QString deviceName = value.isValid() ? value.toString() : QStandardPaths::displayName(QStandardPaths::HomeLocation);

    //TransFilesParam
    req = {
        { "session", sessionId.toStdString() },
        { "id", TransferJobStartId },
        { "paths", paths },
        { "sub", true },
        { "savedir", deviceName.toStdString() },
    };

    req.add_member("api", "Backend.tryTransFiles");   //BackendImpl::tryTransFiles

    rpcClient->call(req, res);
}

void TransferHelperPrivate::handleTryConnect(const QString &ip)
{
    qInfo() << "connect to " << ip;
    co::Json req, res;
    fastring targetIp(ip.toStdString());
    fastring pinCode("");

    req = {
        { "session", qApp->applicationName().toStdString() },
        { "host", targetIp },
        { "password", pinCode },
    };
    req.add_member("api", "Backend.tryConnect");
    rpcClient->call(req, res);
}

void TransferHelperPrivate::handleCancelTransfer()
{
    // TODO
}

void TransferHelperPrivate::showResult(TransferHelperPrivate::TransferMode mode, bool result, const QString &msg)
{
    switch (mode) {
    case ReceiveMode: {
        QStringList actions;
        if (result)
            actions << kNotifyViewAction << tr("View");

        recvNotifyId = notifyMessage(recvNotifyId, msg, actions, 30 * 1000);
    } break;
    case SendMode:
        transferDialog->switchResultPage(result, msg);
        break;
    }
}

void TransferHelperPrivate::waitForConfirm(TransferMode mode, const QString &name)
{
    switch (mode) {
    case ReceiveMode: {
        QStringList actions { kNotifyRefuseAction, tr("Refuse"), kNotifyAcceptAction, tr("Accept") };
        QString msg(tr("Received transfer request from \"%1\""));
        QFontMetrics fm(qApp->font());
        QString ret = fm.elidedText(name, Qt::ElideMiddle, 200);

        recvNotifyId = notifyMessage(recvNotifyId, msg.arg(ret), actions, 30 * 1000);
    } break;
    case SendMode:
        transferDialog->switchWaitConfirmPage();
        transferDialog->exec();
        break;
    }
}

void TransferHelperPrivate::updateProgress(TransferMode mode, int value, const QString &remainTime)
{
    switch (mode) {
    case ReceiveMode: {
        QStringList actions { kNotifyCancelAction, tr("Cancel") };
        QString msg(tr("File receiving %1% | Remaining time %2").arg(QString::number(value), remainTime));

        recvNotifyId = notifyMessage(recvNotifyId, msg, actions, 15 * 1000);
    } break;
    case SendMode:
        QString title = tr("Sending files to \"%1\"").arg(sendToWho);
        transferDialog->switchProgressPage(title);
        transferDialog->updateProgress(value, remainTime);
        break;
    }
}

uint TransferHelperPrivate::notifyMessage(uint replacesId, const QString &body, const QStringList &actions, int expireTimeout)
{
    QDBusReply<uint> reply = notifyIfc->call("Notify", "dde-cooperation", replacesId,
                                             "collaboration", "file transfer", body,
                                             actions, QVariantMap(), expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
}

void TransferHelperPrivate::onActionTriggered(uint replacesId, const QString &action)
{
    qInfo() << "========" << replacesId << action;
}

void TransferHelperPrivate::handleSetConfig(const QString &key, const QString &value)
{
    co::Json req, res;

    req = {
        { "appname", qApp->applicationName().toStdString() },
        { "key", key.toStdString() },
        { "value", value.toStdString() },
    };
    req.add_member("api", "Backend.setAppConfig");
    rpcClient->call(req, res);
}

QString TransferHelperPrivate::handleGetConfig(const QString &key)
{
    co::Json req, res;

    req = {
        { "appname", qApp->applicationName().toStdString() },
        { "key", key.toStdString() },
    };
    req.add_member("api", "Backend.getAppConfig");
    rpcClient->call(req, res);

    QString value = res.get("msg").as_string().c_str();
    return value;
}

TransferHelper::TransferHelper(QObject *parent)
    : QObject(parent),
      d(new TransferHelperPrivate(this))
{
    d->thisDestruct = false;
}

TransferHelper::~TransferHelper()
{
    d->thisDestruct = true;
}

void TransferHelper::init()
{
    d->localIPCStart();

    d->rpcClient = std::shared_ptr<rpc::Client>(new rpc::Client("127.0.0.1", UNI_IPC_BACKEND_PORT, false));

    go([this] {
        co::sleep(3000);
        d->backendOk = d->handlePingBacked();
        d->initConfig();
    });
}

TransferHelper *TransferHelper::instance()
{
    static TransferHelper ins;
    return &ins;
}

void TransferHelper::sendFiles(const QString &ip, const QString &devName, const QStringList &fileList)
{
    d->sendToWho = devName;
    d->readyToSendFiles = fileList;
    if (fileList.isEmpty())
        return;

    d->status = Connecting;
    d->canTransfer = true;
    go([ip, this] {
        d->handleTryConnect(ip);
    });

    d->waitForConfirm(TransferHelperPrivate::SendMode, "");
}

TransferStatus TransferHelper::transferStatus()
{
    return d->status;
}

void TransferHelper::buttonClicked(const QVariantMap &info)
{
    auto id = info.value("id").toString();
    auto ip = info.value("ip").toString();
    auto devName = info.value("device").toString();

    if (id == kTransferButtonId) {
        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty())
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());

        if (selectedFiles.isEmpty())
            return;

        TransferHelper::instance()->sendFiles(ip, devName, selectedFiles);
    } else if (id == kHistoryButtonId) {
        // TODO:
    }
}

bool TransferHelper::buttonVisible(const QVariantMap &info)
{
    auto id = info.value("id").toString();
    int state = info.value("state").toInt();
    if (qApp->property("onlyTransfer").toBool() && id == kHistoryButtonId)
        return false;

    if (id == kTransferButtonId)
        return state != 2 && TransferHelper::instance()->d->backendOk;

    return true;
}

bool TransferHelper::buttonClickable(const QVariantMap &info)
{
    auto id = info.value("id").toString();
    if (id == kTransferButtonId)
        return TransferHelper::instance()->transferStatus() == Idle;

    return true;
}

void TransferHelper::onConnectStatusChanged(int result, const QString &msg)
{
    qInfo() << "connect status: " << result << " msg:" << msg;
    if (result > 0) {
        if (!d->canTransfer) {
            d->status = Idle;
            return;
        }

        d->status = Transfering;
        d->updateProgress(TransferHelperPrivate::SendMode, 1, tr("calculating"));

        go([this] {
            d->handleSendFiles(d->readyToSendFiles);
        });
    } else {
        d->status = Idle;
        d->showResult(TransferHelperPrivate::SendMode, false, "xxxxxxx");
    }
}

void TransferHelper::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    qInfo() << id << result << msg;
    switch (result) {
    case JOB_TRANS_FAILED:
        break;
    case JOB_TRANS_DOING:
        break;
    case JOB_TRANS_FINISHED:
        break;
    default:
        break;
    }
}

void TransferHelper::onFileTransStatusChanged(const QString &status)
{
    co::Json statusJson;
    statusJson.parse_from(status.toStdString());
    ipc::FileStatus param;
    param.from_json(statusJson);

    if (d->fileIds.contains(param.file_id)) {
        // 已经记录过，只更新数据
        int64_t increment = param.current - d->fileIds[param.file_id];
        d->transferInfo.transferSize += increment;   //增量值
        d->fileIds[param.file_id] = param.current;

        if (param.current >= param.total) {
            // 此文件已完成，从文件统计中删除
            d->fileIds.remove(param.file_id);
        }
    } else {
        d->transferInfo.totalSize += param.total;
        d->transferInfo.transferSize += param.current;
        d->fileIds.insert(param.file_id, param.current);
    }

    if (param.second > d->transferInfo.maxTimeSec)
        d->transferInfo.maxTimeSec = param.second;

    // 计算传输进度与剩余时间
    double value = static_cast<double>(d->transferInfo.transferSize) / d->transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        d->updateProgress(TransferHelperPrivate::ReceiveMode, 100, "00:00:00");

        QTimer::singleShot(1000, this, [this] {
            d->status = Idle;
            d->showResult(TransferHelperPrivate::ReceiveMode, true, tr("File sent successfully"));
        });
    } else {
        int remainTime = d->transferInfo.maxTimeSec * 100 / progressValue - d->transferInfo.maxTimeSec;
        QTime time(0, 0, 0);
        time = time.addSecs(remainTime);
        d->updateProgress(TransferHelperPrivate::ReceiveMode, progressValue, time.toString("hh:mm:ss"));
    }
}

void TransferHelper::cancelTransfer()
{
    d->canTransfer = false;
    if (d->status == Transfering) {
        go([this] {
            d->handleCancelTransfer();
        });
    }

    d->status = Idle;
}

void TransferHelper::onConfigChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != "GenericAttribute")
        return;

    if (key == kStoragePathKey)
        d->handleSetConfig(KEY_APP_STORAGE_DIR, value.toString());
}
