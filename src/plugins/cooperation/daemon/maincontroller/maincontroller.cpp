// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "global_defines.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "config/configmanager.h"

#include "common/constant.h"
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

using TransHistoryInfo = QMap<QString, QString>;
Q_GLOBAL_STATIC(TransHistoryInfo, transHistory)

using namespace daemon_cooperation;

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);

    initConnect();
}

void MainController::initConnect()
{
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
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    info.insert(AppSettings::DiscoveryModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    info.insert(AppSettings::DeviceNameKey,
                value.isValid()
                        ? value.toString()
                        : QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0)).dirName());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    info.insert(AppSettings::PeripheralShareKey, value.isValid() ? value.toBool() : false);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    info.insert(AppSettings::LinkDirectionKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    info.insert(AppSettings::TransferModeKey, value.isValid() ? value.toInt() : 0);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setAppConfig(KEY_APP_STORAGE_DIR, storagePath); });

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : false);

    auto doc = QJsonDocument::fromVariant(info);
    CooperationUtil::instance()->registAppInfo(doc.toJson());
}

void MainController::unregist()
{
    CooperationUtil::instance()->unregistAppInfo();
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
    isTransTimeout = false;
    transferInfo.clear();
    recvFilesSavePath.clear();
    fileIds.clear();
    recvNotifyId = 0;

    // 超时处理
    QTimer::singleShot(10 * 1000, this, [this] { isTransTimeout = true; });

    QStringList actions { NotifyRejectAction, tr("Reject"),
                          NotifyAcceptAction, tr("Accept"),
                          NotifyCloseAction, tr("Close") };
    QString msg(tr("\"%1\" send some files to you"));

    recvNotifyId = notifyMessage(recvNotifyId, msg.arg(name), actions, {}, 10 * 1000);
}

void MainController::onActionTriggered(uint replacesId, const QString &action)
{
    if (replacesId != recvNotifyId)
        return;

    if (action == NotifyCancelAction) {
        CooperationUtil::instance()->cancelTrans();
    } else if (action == NotifyRejectAction) {
        CooperationUtil::instance()->replyTransRequest(ApplyTransType::APPLY_TRANS_REFUSED);
    } else if (action == NotifyAcceptAction) {
        if (isTransTimeout)
            return;

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
        break;
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

            transHistory->insert(ip, recvFilesSavePath);
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
        updateProgress(100, "00:00:00");
    } else {
        int remainTime = transferInfo.maxTimeSec * 100 / progressValue - transferInfo.maxTimeSec;
        QTime time(0, 0, 0);
        time = time.addSecs(remainTime);
        updateProgress(progressValue, time.toString("hh:mm:ss"));
    }
}
