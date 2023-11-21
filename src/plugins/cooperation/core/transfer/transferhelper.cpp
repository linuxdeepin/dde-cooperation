// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "transferhelper_p.h"
#include "config/configmanager.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"

#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/backend.h"

#include <QDesktopServices>
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

inline constexpr char NotifyCancelAction[] { "cancel" };
inline constexpr char NotifyRejectAction[] { "reject" };
inline constexpr char NotifyAcceptAction[] { "accept" };
inline constexpr char NotifyCloseAction[] { "close" };
inline constexpr char NotifyViewAction[] { "view" };

inline constexpr char HistoryButtonId[] { "history-button" };
inline constexpr char TransferButtonId[] { "transfer-button" };

using TransHistoryInfo = QMap<QString, QString>;
Q_GLOBAL_STATIC(TransHistoryInfo, transHistory)

using namespace cooperation_core;

TransferHelperPrivate::TransferHelperPrivate(TransferHelper *qq)
    : QObject(qq),
      q(qq)
{
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), q);

    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          q, SLOT(onActionTriggered(uint, const QString &)));
    *transHistory = HistoryManager::instance()->getTransHistory();
}

TransferHelperPrivate::~TransferHelperPrivate()
{
    if (transferDialog && !transferDialog->parent())
        delete transferDialog;
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

    QString saveDir = (deviceName + "(%1)").arg(CooperationUtil::localIPAddress());
    ipc::TransFilesParam transParam;
    transParam.session = CooperationUtil::instance()->sessionId().toStdString();
    transParam.targetSession = kMainAppName;
    transParam.id = TransferJobStartId;
    transParam.paths = fileVector;
    transParam.sub = true;
    transParam.savedir = saveDir.toStdString();

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
    conParam.appName = qApp->applicationName().toStdString();
    conParam.host = targetIp;
    conParam.password = pinCode;
    conParam.targetAppname = kMainAppName;

    req = conParam.as_json();
    req.add_member("api", "Backend.tryConnect");
    rpcClient.call(req, res);
    rpcClient.close();
}

void TransferHelperPrivate::handleCancelTransfer()
{
    // TODO
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
    co::Json req, res;

    ipc::TransJobParam jobParam;
    jobParam.session = CooperationUtil::instance()->sessionId().toStdString();
    jobParam.job_id = TransferJobStartId;
    jobParam.appname = qApp->applicationName().toStdString();

    req = jobParam.as_json();
    req.add_member("api", "Backend.cancelTransJob");   //BackendImpl::cancelTransJob
    rpcClient.call(req, res);
    rpcClient.close();
    qInfo() << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
}

void TransferHelperPrivate::transferResult(bool result, const QString &msg)
{
    switch (currentMode) {
    case TransferHelper::ReceiveMode: {
        QStringList actions;
        if (result)
            actions << NotifyViewAction << tr("View");

        recvNotifyId = notifyMessage(recvNotifyId, msg, actions, 3 * 1000);
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
        // 在通知中心中，如果通知内容包含“%”且actions中存在“cancel”，则不会在通知中心显示
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
    QVariantMap hitMap;
    if (status.loadAcquire() == TransferHelper::Transfering) {
        // dde-session-ui 5.7.2.2 版本后，支持设置该属性使消息不进通知中心
        hitMap.insert("x-deepin-ShowInNotifyCenter", false);
    }

    QDBusReply<uint> reply = notifyIfc->call("Notify", kMainAppName, replacesId,
                                             "dde-cooperation", tr("File transfer"), body,
                                             actions, hitMap, expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
}

void TransferHelperPrivate::onVerifyTimeout()
{
    isTransTimeout = true;
    if (currentMode == TransferHelper::ReceiveMode || status.loadAcquire() != TransferHelper::Confirming)
        return;

    transDialog()->switchResultPage(false, tr("The other party did not receive, the files failed to send"));
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

    if (!d->status.testAndSetRelease(TransferHelper::Idle, TransferHelper::Connecting)) {
        d->status.storeRelease(TransferHelper::Idle);
        return;
    }

    d->currentMode = SendMode;
    UNIGO([ip, this] {
        d->handleTryConnect(ip);
    });

    waitForConfirm("");
}

TransferHelper::TransferStatus TransferHelper::transferStatus()
{
    return static_cast<TransferStatus>(d->status.loadAcquire());
}

void TransferHelper::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    LOG << "button clicked, button id: " << id.toStdString()
        << " ip: " << info->ipAddress().toStdString()
        << " device name: " << info->deviceName().toStdString();

    if (id == TransferButtonId) {
        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty())
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());

        if (selectedFiles.isEmpty())
            return;

        TransferHelper::instance()->sendFiles(info->ipAddress(), info->deviceName(), selectedFiles);
    } else if (id == HistoryButtonId) {
        if (!transHistory->contains(info->ipAddress()))
            return;

        QDesktopServices::openUrl(QUrl::fromLocalFile(transHistory->value(info->ipAddress())));
    }
}

bool TransferHelper::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    if (id == TransferButtonId) {
        return (info->connectStatus() != DeviceInfo::Offline && info->transMode() == DeviceInfo::TransMode::Everyone);
    }

    if (id == HistoryButtonId) {
        if (qApp->property("onlyTransfer").toBool())
            return false;

        return transHistory->contains(info->ipAddress()) && QFile::exists(transHistory->value(info->ipAddress()));
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

void TransferHelper::onConnectStatusChanged(int result, const QString &msg, const bool isself)
{
    LOG << "connect status: " << result << " msg:" << msg.toStdString();
    if (result > 0) {
        if (!isself)
            return;
        UNIGO([this] {
            d->status.storeRelease(Confirming);
            d->handleApplyTransFiles(0);
        });
    } else {
        d->status.storeRelease(Idle);
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
    {
        if (d->currentMode == SendMode)
            break;

        // msg: deviceName(ip)
        // 获取存储路径和ip
        int startPos = msg.lastIndexOf("(");
        int endPos = msg.lastIndexOf(")");
        if (startPos != -1 && endPos != -1) {
            auto ip = msg.mid(startPos + 1, endPos - startPos - 1);
            auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
            auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
            d->recvFilesSavePath = storagePath + QDir::separator() + msg;

            transHistory->insert(ip, d->recvFilesSavePath);
            HistoryManager::instance()->writeIntoTransHistory(ip, d->recvFilesSavePath);
        }
    }
        break;
    case JOB_TRANS_CANCELED:
        d->transferResult(false, "xxxxxxxxxxx");
        break;
    default:
        break;
    }
}

void TransferHelper::onFileTransStatusChanged(const QString &status)
{
    DLOG_IF(TEST_LOGOUT) << "file transfer info: " << status.toStdString();
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
            d->status.storeRelease(Idle);
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
    d->isTransTimeout = false;
    d->transferInfo.clear();
    d->fileIds.clear();
    d->recvFilesSavePath.clear();

    // 超时处理
    QTimer::singleShot(10 * 1000, d.data(), &TransferHelperPrivate::onVerifyTimeout);
    switch (d->currentMode) {
    case ReceiveMode: {
        d->recvNotifyId = 0;
        QStringList actions { NotifyRejectAction, tr("Reject"),
                              NotifyAcceptAction, tr("Accept"),
                              NotifyCloseAction, tr("Close") };
        QString msg(tr("\"%1\" send some files to you"));
        QFontMetrics fm(qApp->font());
        QString ret = fm.elidedText(name, Qt::ElideMiddle, 200);

        d->recvNotifyId = d->notifyMessage(d->recvNotifyId, msg.arg(ret), actions, 10 * 1000);
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
        d->status.storeRelease(Idle);
        UNIGO([this] {
            d->handleApplyTransFiles(ApplyTransType::APPLY_TRANS_REFUSED);
        });
    } else if (action == NotifyAcceptAction) {
        if (d->isTransTimeout)
            return;

        d->status.storeRelease(Transfering);
        UNIGO([this] {
            d->handleApplyTransFiles(ApplyTransType::APPLY_TRANS_CONFIRM);
        });
    } else if (action == NotifyCloseAction) {
        d->notifyIfc->call("CloseNotification", d->recvNotifyId);
    } else if (action == NotifyViewAction) {
        if (d->recvFilesSavePath.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));

        QDesktopServices::openUrl(QUrl::fromLocalFile(d->recvFilesSavePath));
    }
}

void TransferHelper::accepted()
{
    if (!d->status.testAndSetRelease(Confirming, Transfering)) {
        d->status.storeRelease(Idle);
        return;
    }

    d->updateProgress(1, tr("calculating"));
    UNIGO([this] {
        d->handleSendFiles(d->readyToSendFiles);
    });
}

void TransferHelper::rejected()
{
    d->status.storeRelease(Idle);
    d->transferResult(false, tr("The other party rejects your request"));
}

void TransferHelper::cancelTransfer()
{
    if (d->status.loadAcquire() == Transfering) {
        UNIGO([this] {
            d->handleCancelTransfer();
        });
    }

    d->status.storeRelease(Idle);
}
