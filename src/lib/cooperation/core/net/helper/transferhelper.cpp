// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "transferhelper_p.h"
#include "configs/settings/configmanager.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "net/networkutil.h"

#include "common/constant.h"
#include "common/commonutils.h"

#include <QDesktopServices>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QProcess>

#ifdef linux
#    include "base/reportlog/reportlogmanager.h"

#    include <QDBusInterface>
#    include <QDBusReply>
#endif

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr int TransferJobStartId = 1000;

inline constexpr char HistoryButtonId[] { "history-button" };
inline constexpr char TransferButtonId[] { "transfer-button" };

inline constexpr char NotifyCancelAction[] { "cancel" };
inline constexpr char NotifyRejectAction[] { "reject" };
inline constexpr char NotifyAcceptAction[] { "accept" };
inline constexpr char NotifyCloseAction[] { "close" };
inline constexpr char NotifyViewAction[] { "view" };

using TransHistoryInfo = QMap<QString, QString>;
Q_GLOBAL_STATIC(TransHistoryInfo, transHistory)

#ifdef linux
inline constexpr char Khistory[] { "history" };
inline constexpr char Ksend[] { "send" };
#else
inline constexpr char Khistory[] { ":/icons/deepin/builtin/texts/history_18px.svg" };
inline constexpr char Ksend[] { ":/icons/deepin/builtin/texts/send_18px.svg" };
#endif

using namespace deepin_cross;
using namespace cooperation_core;

TransferHelperPrivate::TransferHelperPrivate(TransferHelper *qq)
    : QObject(qq),
      q(qq)
{
    *transHistory = HistoryManager::instance()->getTransHistory();
    connect(HistoryManager::instance(), &HistoryManager::transHistoryUpdated, q,
            [] {
                *transHistory = HistoryManager::instance()->getTransHistory();
            });
    notice = new NoticeUtil(q);
    initConnect();
}

TransferHelperPrivate::~TransferHelperPrivate()
{
}

void TransferHelperPrivate::initConnect()
{
    connect(notice, &NoticeUtil::onConfirmTimeout, q, &TransferHelper::onVerifyTimeout);
    connect(notice, &NoticeUtil::ActionInvoked, q, &TransferHelper::onActionTriggered);
}

TransferDialog *TransferHelperPrivate::transDialog()
{
    if (!transferDialog) {
        transferDialog = new TransferDialog(qApp->activeWindow());
        transferDialog->setModal(true);
        connect(transferDialog, &TransferDialog::cancel, q, &TransferHelper::cancelTransfer);
    }

    return transferDialog;
}

void TransferHelperPrivate::reportTransferResult(bool result)
{
#ifdef linux
    QVariantMap map;
    map.insert("deliveryResult", result);
    ReportLogManager::instance()->commit(ReportAttribute::FileDelivery, map);
#endif
}

void TransferHelperPrivate::notifyMessage(const QString &body, const QStringList &actions, int expireTimeout)
{
    notice->notifyMessage(tr("File transfer"), body, actions, QVariantMap(), expireTimeout);
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

void TransferHelper::registBtn()
{
    ClickedCallback clickedCb = TransferHelper::buttonClicked;
    ButtonStateCallback visibleCb = TransferHelper::buttonVisible;
    ButtonStateCallback clickableCb = TransferHelper::buttonClickable;
    QVariantMap historyInfo { { "id", HistoryButtonId },
                              { "description", tr("View transfer history") },
                              { "icon-name", Khistory },
                              { "location", 2 },
                              { "button-style", 0 },
                              { "clicked-callback", QVariant::fromValue(clickedCb) },
                              { "visible-callback", QVariant::fromValue(visibleCb) },
                              { "clickable-callback", QVariant::fromValue(clickableCb) } };

    QVariantMap transferInfo { { "id", TransferButtonId },
                               { "description", tr("Send files") },
                               { "icon-name", Ksend },
                               { "location", 3 },
                               { "button-style", 1 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) },
                               { "clickable-callback", QVariant::fromValue(clickableCb) } };

    CooperationUtil::instance()->registerDeviceOperation(historyInfo);
    CooperationUtil::instance()->registerDeviceOperation(transferInfo);
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

    // send the transfer file RPC request
    NetworkUtil::instance()->sendTransApply(ip);
    //d->status.storeRelease(Confirming);

    waitForConfirm();
}

TransferHelper::TransferStatus TransferHelper::transferStatus()
{
    return static_cast<TransferStatus>(d->status.loadAcquire());
}

void TransferHelper::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    QString ip = info->ipAddress();
    QString name = info->deviceName();
    LOG << "button clicked, button id: " << id.toStdString()
        << " ip: " << ip.toStdString()
        << " device name: " << name.toStdString();

    if (id == TransferButtonId) {

        // connect remote to prepare transfer
        NetworkUtil::instance()->pingTarget(ip);

        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty())
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());

        if (selectedFiles.isEmpty())
            return;

        TransferHelper::instance()->sendFiles(ip, name, selectedFiles);
    } else if (id == HistoryButtonId) {
        if (!transHistory->contains(ip))
            return;

        QDesktopServices::openUrl(QUrl::fromLocalFile(transHistory->value(ip)));
    }
}

bool TransferHelper::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    if (id == TransferButtonId) {
        switch (info->transMode()) {
        case DeviceInfo::TransMode::Everyone:
            return info->connectStatus() != DeviceInfo::Offline;
        case DeviceInfo::TransMode::OnlyConnected:
            return info->connectStatus() == DeviceInfo::Connected;
        default:
            return false;
        }
    }

    if (id == HistoryButtonId) {
        if (qApp->property("onlyTransfer").toBool())
            return false;

        if (!transHistory->contains(info->ipAddress()))
            return false;

        bool exists = QFile::exists(transHistory->value(info->ipAddress()));
        if (!exists)
            HistoryManager::instance()->removeTransHistory(info->ipAddress());

        return exists;
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

void TransferHelper::handleApplyTransFiles(int type)
{
    // 获取设备名称
    auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
    QString deviceName = value.isValid()
            ? value.toString()
            : QStandardPaths::writableLocation(QStandardPaths::HomeLocation).section(QDir::separator(), -1);

    LOG << "handle apply file, deviceName= " << deviceName.toStdString();
}

void TransferHelper::onVerifyTimeout()
{
    d->isTransTimeout = true;
    if (d->status.loadAcquire() != TransferHelper::Confirming)
        return;

    d->transDialog()->switchResultPage(false, tr("The other party did not receive, the files failed to send"));
}

void TransferHelper::handleCancelTransfer()
{
    //    LOG << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
}

void TransferHelper::transferResult(bool result, const QString &msg)
{
    d->transDialog()->switchResultPage(result, msg);
    d->reportTransferResult(result);
}

void TransferHelper::updateProgress(int value, const QString &remainTime)
{
    QString title = tr("Sending files to \"%1\"").arg(CommonUitls::elidedText(d->sendToWho, Qt::ElideMiddle, 15));
    d->transDialog()->switchProgressPage(title);
    d->transDialog()->updateProgress(value, remainTime);
}

void TransferHelper::onActionTriggered(const QString &action)
{
    if (action == NotifyCancelAction) {
        NetworkUtil::instance()->cancelTrans();
    } else if (action == NotifyRejectAction) {
        NetworkUtil::instance()->replyTransRequest(false);
    } else if (action == NotifyAcceptAction) {
        NetworkUtil::instance()->replyTransRequest(true);
    } else if (action == NotifyCloseAction) {
        d->notice->closeNotification();
    } else if (action == NotifyViewAction) {
        if (d->recvFilesSavePath.isEmpty()) {
            auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
            auto defaultSavePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        }

        openFileLocation(d->recvFilesSavePath);
    }
}

void TransferHelper::openFileLocation(const QString &path)
{
    QProcess::execute("dde-file-manager", QStringList() << path);
}

void TransferHelper::notifyTransferRequest(const QString &info)
{
    DLOG << "notifyTransferRequest info: " << info.toStdString();
    QStringList actions { NotifyRejectAction, tr("Reject"),
                          NotifyAcceptAction, tr("Accept"),
                          NotifyCloseAction, tr("Close") };
    static QString msg(tr("\"%1\" send some files to you"));

    d->notifyMessage(msg.arg(CommonUitls::elidedText(info, Qt::ElideMiddle, 25)), actions, 10 * 1000);
}

void TransferHelper::notifyTransferRescult(bool result, const QString &msg)
{
    QStringList actions;
    if (result)
        actions << NotifyViewAction << tr("View");

    d->notifyMessage(msg, actions, 3 * 1000);
}

void TransferHelper::onConnectStatusChanged(int result, const QString &msg, const bool isself)
{
    LOG << "connect status: " << result << " msg:" << msg.toStdString();
    if (result > 0) {
        if (!isself)
            return;

        d->status.storeRelease(Confirming);
        handleApplyTransFiles(0);
    } else {
        d->status.storeRelease(Idle);
        transferResult(false, tr("Connect to \"%1\" failed").arg(d->sendToWho));
    }
}

void TransferHelper::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    LOG << "id: " << id << " result: " << result << " msg: " << msg.toStdString();
    switch (result) {
    case JOB_TRANS_FAILED:
        if (msg.contains("::not enough")) {
            transferResult(false, tr("Insufficient storage space, file delivery failed this time. Please clean up disk space and try again!"));
        } else if (msg.contains("::off line")) {
            transferResult(false, tr("Network not connected, file delivery failed this time. Please connect to the network and try again!"));
        } else {
            transferResult(false, tr("File read/write exception"));
        }
        break;
    case JOB_TRANS_DOING:
        break;
    case JOB_TRANS_FINISHED: {
        d->status.storeRelease(Idle);
        transferResult(true, tr("File sent successfully"));
    } break;
    case JOB_TRANS_CANCELED:
        transferResult(false, tr("The other party has canceled the file transfer"));
        break;
    default:
        break;
    }
}

void TransferHelper::onFileTransStatusChanged(const QString &status)
{
    DLOG << "file transfer info: " << status.toStdString();
    //    co::Json statusJson;
    //    statusJson.parse_from(status.toStdString());
    //    ipc::FileStatus param;
    //    param.from_json(statusJson);

    //    d->transferInfo.totalSize = param.total;
    //    d->transferInfo.transferSize = param.current;
    //    d->transferInfo.maxTimeMs = param.millisec;

    //    // 计算整体进度和预估剩余时间
    //    double value = static_cast<double>(d->transferInfo.transferSize) / d->transferInfo.totalSize;
    //    int progressValue = static_cast<int>(value * 100);
    //    QTime time(0, 0, 0);
    //    int remain_time;
    //    if (progressValue <= 0) {
    //        return;
    //    } else if (progressValue >= 100) {
    //        progressValue = 100;
    //        remain_time = 0;
    //    } else {
    //        remain_time = (d->transferInfo.maxTimeMs * 100 / progressValue - d->transferInfo.maxTimeMs) / 1000;
    //    }
    //    time = time.addSecs(remain_time);

    //    LOG_IF(FLG_log_detail) << "progressbar: " << progressValue << " remain_time=" << remain_time;
    //    LOG_IF(FLG_log_detail) << "totalSize: " << d->transferInfo.totalSize << " transferSize=" << d->transferInfo.transferSize;

    //    d->updateProgress(progressValue, time.toString("hh:mm:ss"));
}

void TransferHelper::waitForConfirm()
{
    d->isTransTimeout = false;
    d->transferInfo.clear();
    d->recvFilesSavePath.clear();

    // 超时处理
    // d->confirmTimer.start();
    d->transDialog()->switchWaitConfirmPage();
    d->transDialog()->show();
}

void TransferHelper::accepted()
{
    if (!d->status.testAndSetRelease(Confirming, Transfering)) {
        d->status.storeRelease(Idle);
        return;
    }
    updateProgress(1, tr("calculating"));
    NetworkUtil::instance()->doSendFiles(d->readyToSendFiles);
}

void TransferHelper::rejected()
{
    DLOG << "file transfer rejected >>> ";
    d->status.storeRelease(Idle);
    transferResult(false, tr("The other party rejects your request"));
}

void TransferHelper::cancelTransfer()
{
    if (d->status.loadAcquire() == Transfering) {
        handleCancelTransfer();
    }

    d->status.storeRelease(Idle);
}
