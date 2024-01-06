// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "transferhelper_p.h"
#include "configs/settings/configmanager.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"

#include "common/constant.h"
#include "common/commonutils.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/frontend.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/chan.h"
#include "ipc/proto/backend.h"

#include <QDesktopServices>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QTimer>

#ifdef linux
#include "base/reportlog/reportlogmanager.h"

#include <QDBusInterface>
#include <QDBusReply>
#endif

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr int TransferJobStartId = 1000;

inline constexpr char HistoryButtonId[] { "history-button" };
inline constexpr char TransferButtonId[] { "transfer-button" };

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

    confirmTimer.setInterval(10 * 1000);
    connect(&confirmTimer, &QTimer::timeout, this, &TransferHelperPrivate::onVerifyTimeout);
}

TransferHelperPrivate::~TransferHelperPrivate()
{
}

TransferDialog *TransferHelperPrivate::transDialog()
{
    if (!transferDialog) {
        transferDialog = new TransferDialog(CooperationUtil::instance()->mainWindow());
        transferDialog->setModal(true);
        connect(transferDialog, &TransferDialog::cancel, q, &TransferHelper::cancelTransfer);
    }

    return transferDialog;
}

void TransferHelperPrivate::handleSendFiles(const QStringList &fileList)
{
    // LOG << "send files: " << fileList.toStdList();
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
    transParam.targetSession = CooperRegisterName;   // 发送给后端插件
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
    transInfo.appname = qApp->applicationName().toStdString();
    transInfo.type = type;
    transInfo.tarAppname = CooperRegisterName;   // 发送给后端插件
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
    conParam.targetAppname = CooperRegisterName;   // 发送给后端插件

    req = conParam.as_json();
    req.add_member("api", "Backend.tryConnect");
    rpcClient.call(req, res);
    rpcClient.close();
}

void TransferHelperPrivate::handleCancelTransfer()
{
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
    LOG << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
}

void TransferHelperPrivate::transferResult(bool result, const QString &msg)
{
    transDialog()->switchResultPage(result, msg);
    reportTransferResult(result);
}

void TransferHelperPrivate::updateProgress(int value, const QString &remainTime)
{
    QString title = tr("Sending files to \"%1\"").arg(CommonUitls::elidedText(sendToWho, Qt::ElideMiddle, 15));
    transDialog()->switchProgressPage(title);
    transDialog()->updateProgress(value, remainTime);
}

void TransferHelperPrivate::reportTransferResult(bool result)
{
#ifdef linux
    QVariantMap map;
    map.insert("deliveryResult", result);
    ReportLogManager::instance()->commit(ReportAttribute::FileDelivery, map);
#endif
}

void TransferHelperPrivate::onVerifyTimeout()
{
    isTransTimeout = true;
    if (status.loadAcquire() != TransferHelper::Confirming)
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

    UNIGO([ip, this] {
        d->handleTryConnect(ip);
    });

    waitForConfirm();
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
        if (msg.contains("::not enough")) {
            d->transferResult(false, tr("Insufficient storage space, file delivery failed this time. Please clean up disk space and try again!"));
        } break;
    case JOB_TRANS_DOING:
        break;
    case JOB_TRANS_FINISHED: {
        d->status.storeRelease(Idle);
        d->transferResult(true, tr("File sent successfully"));
    } break;
    case JOB_TRANS_CANCELED:
        d->transferResult(false, tr("The other party has canceled the file transfer"));
        break;
    default:
        break;
    }
}

void TransferHelper::onFileTransStatusChanged(const QString &status)
{
    DLOG_IF(FLG_log_detail) << "file transfer info: " << status.toStdString();
    co::Json statusJson;
    statusJson.parse_from(status.toStdString());
    ipc::FileStatus param;
    param.from_json(statusJson);

    d->transferInfo.totalSize = param.total;
    d->transferInfo.transferSize = param.current;
    d->transferInfo.maxTimeMs = param.millisec;

    // 计算整体进度和预估剩余时间
    double value = static_cast<double>(d->transferInfo.transferSize) / d->transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);
    QTime time(0, 0, 0);
    int remain_time;
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        progressValue = 100;
        remain_time = 0;
    } else {
        remain_time = (d->transferInfo.maxTimeMs * 100 / progressValue - d->transferInfo.maxTimeMs) / 1000;
    }
    time = time.addSecs(remain_time);

    LOG_IF(FLG_log_detail) << "progressbar: " << progressValue << " remain_time=" << remain_time;
    LOG_IF(FLG_log_detail) << "totalSize: " << d->transferInfo.totalSize << " transferSize=" << d->transferInfo.transferSize;

    d->updateProgress(progressValue, time.toString("hh:mm:ss"));
}

void TransferHelper::waitForConfirm()
{
    d->isTransTimeout = false;
    d->transferInfo.clear();
    d->recvFilesSavePath.clear();

    // 超时处理
    d->confirmTimer.start();
    d->transDialog()->switchWaitConfirmPage();
    d->transDialog()->show();
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
