// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "global_defines.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"

#include "common/constant.h"
#include "common/commonutils.h"
#include "ipc/proto/frontend.h"

#include <co/co.h>

#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QTime>
#include <QStandardPaths>
#include <QVariantMap>
#include <QJsonDocument>
#include <QFontMetrics>
#include <QDBusReply>
#include <QProcess>

#include <mutex>

inline constexpr char NotifyServerName[] { "org.freedesktop.Notifications" };
inline constexpr char NotifyServerPath[] { "/org/freedesktop/Notifications" };
inline constexpr char NotifyServerIfce[] { "org.freedesktop.Notifications" };

inline constexpr char NotifyCancelAction[] { "cancel" };
inline constexpr char NotifyRejectAction[] { "reject" };
inline constexpr char NotifyAcceptAction[] { "accept" };
inline constexpr char NotifyCloseAction[] { "close" };
inline constexpr char NotifyViewAction[] { "view" };

using namespace daemon_cooperation;
using namespace deepin_cross;

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);

    transTimer.setInterval(10 * 1000);
    transTimer.setSingleShot(true);

    initConnect();
}

void MainController::initConnect()
{
    connect(&transTimer, &QTimer::timeout, this, &MainController::onConfirmTimeout);
    connect(DConfigManager::instance(), &DConfigManager::valueChanged, this, &MainController::onDConfigValueChanged);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &MainController::onAppAttributeChanged);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
}

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

uint MainController::notifyMessage(uint replacesId, const QString &body, const QStringList &actions, QVariantMap hitMap, int expireTimeout)
{
    QDBusReply<uint> reply = notifyIfc->call("Notify", MainAppName, replacesId,
                                             "dde-cooperation", tr("File transfer"), body,
                                             actions, hitMap, expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
}

void MainController::transferResult(bool result, const QString &msg)
{
    QStringList actions;
    if (result)
        actions << NotifyViewAction << tr("View");

    recvNotifyId = notifyMessage(recvNotifyId, msg, actions, {}, 3 * 1000);
}

void MainController::updateProgress(int value, const QString &remainTime)
{
    // 在通知中心中，如果通知内容包含“%”且actions中存在“cancel”，则不会在通知中心显示
    QStringList actions { NotifyCancelAction, tr("Cancel") };
    // dde-session-ui 5.7.2.2 版本后，支持设置该属性使消息不进通知中心
    QVariantMap hitMap { { "x-deepin-ShowInNotifyCenter", false } };
    QString msg(tr("File receiving %1% | Remaining time %2").arg(QString::number(value), remainTime));

    recvNotifyId = notifyMessage(recvNotifyId, msg, actions, hitMap, 15 * 1000);
}

void MainController::openFileLocation(const QString &path)
{
    QProcess::execute("dde-file-manager", QStringList() << path);
}

void MainController::regist()
{
    QVariantMap info;
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 1) ? 1 : mode;
    info.insert(AppSettings::DiscoveryModeKey, mode);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    info.insert(AppSettings::DeviceNameKey,
                value.isValid()
                        ? value.toString()
                        : QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0)).dirName());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    info.insert(AppSettings::PeripheralShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    info.insert(AppSettings::LinkDirectionKey, value.isValid() ? value.toInt() : 0);

    value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::TransferModeKey, 0);
    mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    info.insert(AppSettings::TransferModeKey, mode);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, storagePath); });

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled);
    info.insert(AppSettings::CooperationEnabled, value.isValid() ? value.toBool() : false);

    auto doc = QJsonDocument::fromVariant(info);
    CooperationUtil::instance()->registAppInfo(doc.toJson());
}

void MainController::unregist()
{
    CooperationUtil::instance()->unregistAppInfo();
}

void MainController::onDConfigValueChanged(const QString &config, const QString &key)
{
    Q_UNUSED(key)

    if (config != kDefaultCfgPath)
        return;

    regist();
}

void MainController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != AppSettings::GenericGroup)
        return;

    if (key == AppSettings::StoragePathKey)
        CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, value.toString());

    regist();
}

void MainController::waitForConfirm(const QString &name)
{
    transferInfo.clear();
    recvFilesSavePath.clear();
    recvNotifyId = 0;
    isReplied = false;
    isRequestTimeout = false;
    requestFrom = name;

    transTimer.start();
    QStringList actions { NotifyRejectAction, tr("Reject"),
                          NotifyAcceptAction, tr("Accept"),
                          NotifyCloseAction, tr("Close") };
    static QString msg(tr("\"%1\" send some files to you"));

    recvNotifyId = notifyMessage(recvNotifyId, msg.arg(CommonUitls::elidedText(name, Qt::ElideMiddle, 25)), actions, {}, 10 * 1000);
}

void MainController::onActionTriggered(uint replacesId, const QString &action)
{
    if (replacesId != recvNotifyId)
        return;

    isReplied = true;
    if (action == NotifyCancelAction) {
        CooperationUtil::instance()->cancelTrans();
    } else if (action == NotifyRejectAction && !isRequestTimeout) {
        CooperationUtil::instance()->replyTransRequest(ApplyTransType::APPLY_TRANS_REFUSED);
    } else if (action == NotifyAcceptAction && !isRequestTimeout) {
        CooperationUtil::instance()->replyTransRequest(ApplyTransType::APPLY_TRANS_CONFIRM);
    } else if (action == NotifyCloseAction) {
        notifyIfc->call("CloseNotification", recvNotifyId);
    } else if (action == NotifyViewAction) {
        if (recvFilesSavePath.isEmpty()) {
            auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
            auto defaultSavePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);

            openFileLocation(defaultSavePath);
            return;
        }

        openFileLocation(recvFilesSavePath);
    }
}

void MainController::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    LOG << "id: " << id << " result: " << result << " msg: " << msg.toStdString();
    switch (result) {
    case JOB_TRANS_FAILED:
        if (msg.contains("::not enough")) {
            transferResult(false, tr("Insufficient storage space, file delivery failed this time. Please clean up disk space and try again!"));
        } break;
    case JOB_TRANS_DOING:
        break;
    case JOB_TRANS_FINISHED: {
        transferResult(true, tr("File sent successfully"));

        // msg: /savePath/deviceName(ip)
        // 获取存储路径和ip
        int startPos = msg.lastIndexOf("(");
        int endPos = msg.lastIndexOf(")");
        if (startPos != -1 && endPos != -1) {
            auto ip = msg.mid(startPos + 1, endPos - startPos - 1);
            recvFilesSavePath = msg;

            HistoryManager::instance()->writeIntoTransHistory(ip, recvFilesSavePath);
        }
    } break;
    case JOB_TRANS_CANCELED:
        transferResult(false, tr("The other party has canceled the file transfer"));
        break;
    default:
        break;
    }
}

void MainController::onFileTransStatusChanged(const QString &status)
{
    LOG << "file transfer info: " << status.toStdString();
    co::Json statusJson;
    statusJson.parse_from(status.toStdString());
    ipc::FileStatus param;
    param.from_json(statusJson);

    transferInfo.totalSize = param.total;
    transferInfo.transferSize = param.current;
    transferInfo.maxTimeMs = param.millisec;

    // 计算整体进度和预估剩余时间
    double value = static_cast<double>(transferInfo.transferSize) / transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);
    QTime time(0, 0, 0);
    int remain_time;
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        progressValue = 100;
        remain_time = 0;
    } else {
        remain_time = (transferInfo.maxTimeMs * 100 / progressValue - transferInfo.maxTimeMs) / 1000;
    }
    time = time.addSecs(remain_time);

    LOG_IF(FLG_log_detail) << "progressbar: " << progressValue << " remain_time=" << remain_time;
    LOG_IF(FLG_log_detail) << "totalSize: " << transferInfo.totalSize << " transferSize=" << transferInfo.transferSize;

    updateProgress(progressValue, time.toString("hh:mm:ss"));
}

void MainController::onConfirmTimeout()
{
    isRequestTimeout = true;
    if (isReplied)
        return;

    static QString msg(tr("\"%1\" delivery of files to you was interrupted due to a timeout"));
    recvNotifyId = notifyMessage(recvNotifyId, msg.arg(CommonUitls::elidedText(requestFrom, Qt::ElideMiddle, 25)), {}, {}, 3 * 1000);
}

void MainController::onNetworkMiss()
{
    if (recvNotifyId == 0)
        return;
    QStringList actions;
    actions << NotifyViewAction << tr("View");
    static QString msg(tr("Network not connected, file delivery failed this time.\
                             Please connect to the network and try again!"));
    recvNotifyId = notifyMessage(recvNotifyId, msg, actions, {}, 15 * 1000);
}
