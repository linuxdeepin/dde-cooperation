// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../maincontroller.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"
#include "share/cooconstrants.h"
#include "common/constant.h"
#include "common/commonutils.h"

#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QTime>
#include <QStandardPaths>
#include <QVariantMap>
#include <QJsonDocument>
#include <QFontMetrics>
#include <QApplication>
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

using namespace cooperation_core;

void MainController::initNotifyConnect()
{
    transTimer.setInterval(10 * 1000);
    transTimer.setSingleShot(true);

    connect(&transTimer, &QTimer::timeout, this, &MainController::onConfirmTimeout);

    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);
    connect(DConfigManager::instance(), &DConfigManager::valueChanged, this, &MainController::onDConfigValueChanged);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
}

void MainController::registApp()
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

void MainController::updateProgress(int value, const QString &remainTime)
{
    // 在通知中心中，如果通知内容包含“%”且actions中存在“cancel”，则不会在通知中心显示
    QStringList actions { NotifyCancelAction, tr("Cancel") };
    // dde-session-ui 5.7.2.2 版本后，支持设置该属性使消息不进通知中心
    QVariantMap hitMap { { "x-deepin-ShowInNotifyCenter", false } };
    QString msg(tr("File receiving %1% | Remaining time %2").arg(QString::number(value), remainTime));

    recvNotifyId = notifyMessage(recvNotifyId, msg, actions, hitMap, 15 * 1000);
}

uint MainController::notifyMessage(uint replacesId, const QString &body, const QStringList &actions, QVariantMap hitMap, int expireTimeout)
{
    QDBusReply<uint> reply = notifyIfc->call("Notify", MainAppName, replacesId,
                                             "dde-cooperation", tr("File transfer"), body,
                                             actions, hitMap, expireTimeout);

    return reply.isValid() ? reply.value() : replacesId;
}

void MainController::onDConfigValueChanged(const QString &config, const QString &key)
{
    Q_UNUSED(key)

    if (config != kDefaultCfgPath)
        return;

    regist();
}

void MainController::onActionTriggered(uint replacesId, const QString &action)
{
    if (replacesId != recvNotifyId)
        return;

    isReplied = true;
    if (action == NotifyCancelAction) {
        onCanceled();
    } else if (action == NotifyRejectAction && !isRequestTimeout) {
        onRejected();
    } else if (action == NotifyAcceptAction && !isRequestTimeout) {
        onAccepted();
    } else if (action == NotifyCloseAction) {
        onCompleted();
    } else if (action == NotifyViewAction) {
        onViewed();
    }
}

void MainController::waitForConfirm(const QString &name)
{
    recvFilesSavePath.clear();
    isReplied = false;
    isRequestTimeout = false;
    requestFrom = name;
    transTimer.start();

    recvNotifyId = 0;
    QStringList actions { NotifyRejectAction, tr("Reject"),
                          NotifyAcceptAction, tr("Accept"),
                          NotifyCloseAction, tr("Close") };
    static QString msg(tr("\"%1\" send some files to you"));

    recvNotifyId = notifyMessage(recvNotifyId, msg.arg(CommonUitls::elidedText(name, Qt::ElideMiddle, MID_FRONT)), actions, {}, 10 * 1000);
}

void MainController::showToast(bool result, const QString &msg)
{
    QStringList actions;
    if (result)
        actions << NotifyViewAction << tr("View");

    recvNotifyId = notifyMessage(recvNotifyId, msg, actions, {}, 3 * 1000);
}

void MainController::onNetworkMiss()
{
    if (recvNotifyId == 0)
        return;
    QStringList actions;
    actions << NotifyViewAction << tr("View");
    static QString msg(tr("Network not connected, file delivery failed this time.\
                             Please connect to the network and try again!"));
    recvNotifyId = notifyMessage(recvNotifyId, msg, actions, {}, 150 * 1000);
}
