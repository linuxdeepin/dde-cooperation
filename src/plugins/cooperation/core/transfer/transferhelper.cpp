// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "transferhelper_p.h"
#include "config/configmanager.h"
#include "utils/cooperationutil.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/backend.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QTimer>

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr int TransferJobStartId = 1000;
inline constexpr char NotifyServerName[] { "org.freedesktop.Notifications" };
inline constexpr char NotifyServerPath[] { "/org/freedesktop/Notifications" };
inline constexpr char NotifyServerIfce[] { "org.freedesktop.Notifications" };

inline constexpr char NotifyCancelAction[] { "cancel-transfer-action" };
inline constexpr char NotifyRejectAction[] { "reject-action" };
inline constexpr char NotifyAcceptAction[] { "accept-action" };
inline constexpr char NotifyViewAction[] { "view-action" };

inline constexpr char HistoryButtonId[] { "history-button" };
inline constexpr char TransferButtonId[] { "transfer-button" };

using namespace cooperation_core;

TransferHelperPrivate::TransferHelperPrivate(TransferHelper *qq)
    : QObject(qq),
      q(qq)
{
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), q);

    //    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, q, &TransferHelper::onConfigChanged);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
}

TransferHelperPrivate::~TransferHelperPrivate()
{
    if (transferDialog && !transferDialog->parent())
        delete transferDialog;
}

void TransferHelperPrivate::initConfig()
{
    //    auto storagePath = ConfigManager::instance()->appAttribute("GenericAttribute", kStoragePathKey);
    //    handleSetConfig(KEY_APP_STORAGE_DIR, storagePath.isValid() ? storagePath.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    //    auto settings = ConfigManager::instance()->appAttribute("GenericAttribute", kTransferModeKey);
    //    handleSetConfig(kTransferModeKey, settings.isValid() ? QString::number(settings.toInt()) : "0");
}

QWidget *TransferHelperPrivate::mainWindow()
{
    for (auto w : qApp->topLevelWidgets()) {
        if (w->objectName() == "MainWindow")
            return w;
    }

    return nullptr;
}

TransferDialog *TransferHelperPrivate::transDialog()
{
    if (!transferDialog) {
        transferDialog = new TransferDialog(mainWindow());
        connect(transferDialog, &TransferDialog::cancel, q, &TransferHelper::cancelTransfer);
    }

    return transferDialog;
}

void TransferHelperPrivate::handleSendFiles(const QStringList &fileList)
{
    qInfo() << "send files: " << fileList;
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
    co::Json req, res;

    co::vector<fastring> fileVector;
    for (QString path : fileList) {
        fileVector.push_back(path.toStdString());
    }

    auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
    QString deviceName = value.isValid()
            ? value.toString()
            : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1);

    ipc::TransFilesParam transParam;
    transParam.session = CooperationUtil::instance()->sessionId().toStdString();
    transParam.id = TransferJobStartId;
    transParam.paths = fileVector;
    transParam.sub = true;
    transParam.savedir = deviceName.toStdString();

    req = transParam.as_json();
    req.add_member("api", "Backend.tryTransFiles");   //BackendImpl::tryTransFiles

    rpcClient.call(req, res);
    rpcClient.close();
}

void TransferHelperPrivate::handleApplyTransFiles(int type)
{
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
    co::Json res;
    // 获取设备名称
    auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
    QString deviceName = value.isValid()
            ? value.toString()
            : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1);

    ApplyTransFiles transInfo;
    transInfo.session = qApp->applicationName().toStdString();
    transInfo.type = type;
    transInfo.tarSession = kMainAppName;
    transInfo.machineName = deviceName.toStdString();

    co::Json req = transInfo.as_json();
    req.add_member("api", "Backend.applyTransFiles");
    rpcClient.call(req, res);
    rpcClient.close();
}

void TransferHelperPrivate::handleTryConnect(const QString &ip)
{
    LOG << "connect to " << ip.toStdString();
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
    co::Json req, res;
    fastring targetIp(ip.toStdString());
    fastring pinCode("");

    ipc::ConnectParam conParam;
    conParam.session = qApp->applicationName().toStdString();
    conParam.host = targetIp;
    conParam.password = pinCode;

    req = conParam.as_json();
    req.add_member("api", "Backend.tryConnect");
    rpcClient.call(req, res);
    rpcClient.close();
}

void TransferHelperPrivate::handleCancelTransfer()
{
    // TODO
}

void TransferHelperPrivate::transferResult(bool result, const QString &msg)
{
    switch (currentMode) {
    case TransferHelper::ReceiveMode: {
        QStringList actions;
        if (result)
            actions << NotifyViewAction << tr("View");

        recvNotifyId = notifyMessage(recvNotifyId, msg, actions, 30 * 1000);
    } break;
    case TransferHelper::SendMode:
        transDialog()->switchResultPage(result, msg);
        break;
    }
}

void TransferHelperPrivate::updateProgress(int value, const QString &remainTime)
{
    switch (currentMode) {
    case TransferHelper::ReceiveMode: {
        QStringList actions { NotifyCancelAction, tr("Cancel") };
        QString msg(tr("File receiving %1% | Remaining time %2").arg(QString::number(value), remainTime));

        recvNotifyId = notifyMessage(recvNotifyId, msg, actions, 15 * 1000);
    } break;
    case TransferHelper::SendMode:
        QString title = tr("Sending files to \"%1\"").arg(sendToWho);
        transDialog()->switchProgressPage(title);
        transDialog()->updateProgress(value, remainTime);
        break;
    }
}

uint TransferHelperPrivate::notifyMessage(uint replacesId, const QString &body, const QStringList &actions, int expireTimeout)
{
    QDBusReply<uint> reply = notifyIfc->call("Notify", kMainAppName, replacesId,
                                             tr("dde-cooperation"), tr("file transfer"), body,
                                             actions, QVariantMap(), expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
}

TransferHelper::TransferHelper(QObject *parent)
    : QObject(parent),
      d(new TransferHelperPrivate(this))
{
}

TransferHelper::~TransferHelper()
{
}

TransferHelper *TransferHelper::instance()
{
    static TransferHelper ins;
    return &ins;
}

void TransferHelper::regist()
{
    ClickedCallback clickedCb = TransferHelper::buttonClicked;
    ButtonStateCallback visibleCb = TransferHelper::buttonVisible;
    ButtonStateCallback clickableCb = TransferHelper::buttonClickable;
    QVariantMap historyInfo { { "id", HistoryButtonId },
                              { "description", QObject::tr("View transfer history") },
                              { "icon-name", "history" },
                              { "location", 2 },
                              { "button-style", 0 },
                              { "clicked-callback", QVariant::fromValue(clickedCb) },
                              { "visible-callback", QVariant::fromValue(visibleCb) },
                              { "clickable-callback", QVariant::fromValue(clickableCb) } };

    QVariantMap transferInfo { { "id", TransferButtonId },
                               { "description", QObject::tr("Send files") },
                               { "icon-name", "send" },
                               { "location", 3 },
                               { "button-style", 1 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) },
                               { "clickable-callback", QVariant::fromValue(clickableCb) } };

    CooperationUtil::instance()->registerDeviceOperation(historyInfo);
    CooperationUtil::instance()->registerDeviceOperation(transferInfo);
}

void TransferHelper::setTransMode(TransferHelper::TransferMode mode)
{
    d->currentMode = mode;
}

void TransferHelper::sendFiles(const QString &ip, const QString &devName, const QStringList &fileList)
{
    d->sendToWho = devName;
    d->readyToSendFiles = fileList;
    if (fileList.isEmpty())
        return;

    d->currentMode = SendMode;
    d->status = TransferHelper::Connecting;
    UNIGO([ip, this] {
        d->handleTryConnect(ip);
    });

    waitForConfirm("");
}

TransferHelper::TransferStatus TransferHelper::transferStatus()
{
    return d->status;
}

void TransferHelper::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    LOG << "button clicked, button id: " << id.toStdString()
        << "ip: " << info->ipAddress().toStdString()
        << "device name: " << info->deviceName().toStdString();

    if (id == TransferButtonId) {
        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty())
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());

        if (selectedFiles.isEmpty())
            return;

        TransferHelper::instance()->sendFiles(info->ipAddress(), info->deviceName(), selectedFiles);
    } else if (id == HistoryButtonId) {
        // TODO:
    }
}

bool TransferHelper::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    // TODO: history
    if (/*qApp->property("onlyTransfer").toBool() && */ id == HistoryButtonId)
        return false;

    if (id == TransferButtonId) {
        return (info->connectStatus() != DeviceInfo::Offline && info->transMode() == DeviceInfo::TransMode::Everyone);
    }

    return true;
}

bool TransferHelper::buttonClickable(const QString &id, const DeviceInfoPointer info)
{
    Q_UNUSED(info)

    if (id == TransferButtonId)
        return TransferHelper::instance()->transferStatus() == Idle;

    return true;
}

void TransferHelper::onConnectStatusChanged(int result, const QString &msg)
{
    LOG << "connect status: " << result << " msg:" << msg.toStdString();
    if (result > 0) {
        UNIGO([this] {
            d->status = Confirming;
            d->handleApplyTransFiles(0);
        });
    } else {
        d->status = Idle;
        d->transferResult(false, tr("Connect to \"%1\" failed").arg(d->sendToWho));
    }
}

void TransferHelper::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    LOG << "id: " << id << " result: " << result << " msg: " << msg.toStdString();
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
    LOG << "file transfer info: " << status.toStdString();
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
        d->updateProgress(100, "00:00:00");

        QTimer::singleShot(1000, this, [this] {
            d->status = Idle;
            d->transferResult(true, tr("File sent successfully"));
        });
    } else {
        int remainTime = d->transferInfo.maxTimeSec * 100 / progressValue - d->transferInfo.maxTimeSec;
        QTime time(0, 0, 0);
        time = time.addSecs(remainTime);
        d->updateProgress(progressValue, time.toString("hh:mm:ss"));
    }
}

void TransferHelper::waitForConfirm(const QString &name)
{
    switch (d->currentMode) {
    case ReceiveMode: {
        QStringList actions { NotifyRejectAction, tr("Reject"), NotifyAcceptAction, tr("Accept") };
        QString msg(tr("Received transfer request from \"%1\""));
        QFontMetrics fm(qApp->font());
        QString ret = fm.elidedText(name, Qt::ElideMiddle, 200);

        d->recvNotifyId = d->notifyMessage(d->recvNotifyId, msg.arg(ret), actions, 30 * 1000);
    } break;
    case SendMode:
        d->transDialog()->switchWaitConfirmPage();
        d->transDialog()->show();
        break;
    }
}

void TransferHelper::onActionTriggered(uint replacesId, const QString &action)
{
    if (replacesId != d->recvNotifyId)
        return;

    if (action == NotifyCancelAction) {
        cancelTransfer();
    } else if (action == NotifyRejectAction) {
        d->status = Idle;
        UNIGO([this] {
            d->handleApplyTransFiles(ApplyTransType::APPLY_TRANS_REFUSED);
        });
    } else if (action == NotifyAcceptAction) {
        d->status = Transfering;
        UNIGO([this] {
            d->handleApplyTransFiles(ApplyTransType::APPLY_TRANS_CONFIRM);
        });
    } else if (action == NotifyViewAction) {
        // TODO:
    }
}

void TransferHelper::accepted()
{
    d->status = Transfering;
    d->updateProgress(1, tr("calculating"));
    UNIGO([this] {
        d->handleSendFiles(d->readyToSendFiles);
    });
}

void TransferHelper::rejected()
{
    d->status = Idle;
    d->transferResult(false, tr("The other party rejects your request"));
}

void TransferHelper::cancelTransfer()
{
    if (d->status == Transfering) {
        UNIGO([this] {
            d->handleCancelTransfer();
        });
    }

    d->status = Idle;
}
