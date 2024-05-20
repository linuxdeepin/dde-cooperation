// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "maincontroller.h"
#include "utils/cooperationutil.h"
#include "utils/historymanager.h"
#include "configs/settings/configmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"
#include "cooperation/cooperationmanager.h"

#include <QDesktopServices>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QHostInfo>
#include <QDir>

using ConnectHistory = QMap<QString, QString>;
Q_GLOBAL_STATIC(ConnectHistory, connectHistory)

using namespace cooperation_core;

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    *connectHistory = HistoryManager::instance()->getConnectHistory();
    connect(HistoryManager::instance(), &HistoryManager::connectHistoryUpdated, this, [] {
        *connectHistory = HistoryManager::instance()->getConnectHistory();
    });

    initConnect();
    initNotifyConnect();
}

MainController::~MainController()
{
}

//public functions----------------------------------------------------------------

MainController *MainController::instance()
{
    static MainController ins;
    return &ins;
}

void MainController::regist()
{
    registApp();
    if (!qApp->property("onlyTransfer").toBool())
        ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled, true);
}

void MainController::unregist()
{
    if (!qApp->property("onlyTransfer").toBool())
        ConfigManager::instance()->setAppAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled, false);

    CooperationUtil::instance()->unregistAppInfo();
}

void MainController::updateDeviceState(const DeviceInfoPointer info)
{
    Q_EMIT deviceOnline({ info });
}

//public slots----------------------------------------------------------------
void MainController::start()
{
    if (isRunning)
        return;

    isOnline = deepin_cross::CommonUitls::getFirstIp().size() > 0;
    networkMonitorTimer->start();

    Q_EMIT startDiscoveryDevice();
    isRunning = true;

    // 延迟1s，为了展示发现界面
    QTimer::singleShot(1000, this, &MainController::discoveryDevice);
}

void MainController::stop()
{
}

//private slots----------------------------------------------------------------

void MainController::checkNetworkState()
{
    // 网络状态检测
    bool isConnected = deepin_cross::CommonUitls::getFirstIp().size() > 0;

    if (isConnected != isOnline) {
        isOnline = isConnected;
        Q_EMIT onlineStateChanged(isConnected);
    }
}

void MainController::updateDeviceList(const QString &ip, const QString &connectedIp, int osType, const QString &info, bool isOnline)
{
    if (!this->isOnline)
        return;

    if (isOnline) {
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(info.toLocal8Bit(), &error);
        if (error.error != QJsonParseError::NoError)
            return;

        auto map = doc.toVariant().toMap();
        if (!map.contains("DeviceName"))
            return;

        map.insert("IPAddress", ip);
        map.insert("OSType", osType);
        auto devInfo = DeviceInfo::fromVariantMap(map);
        if (devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
            if (connectedIp == CooperationUtil::localIPAddress())
                devInfo->setConnectStatus(DeviceInfo::Connected);

            // 处理设备的共享属性发生变化情况
            CooperationManager::instance()->checkAndProcessShare(devInfo);
            Q_EMIT deviceOnline({ devInfo });
            return;
        }
    }

    // 更新设备状态为离线状态
    if (connectHistory->contains(ip)) {
        DeviceInfoPointer info(new DeviceInfo(ip, connectHistory->value(ip)));
        info->setConnectStatus(DeviceInfo::Offline);
        updateDeviceState(info);
        return;
    }

    Q_EMIT deviceOffline(ip);
}

void MainController::onDiscoveryFinished(const QList<DeviceInfoPointer> &infoList)
{
    if (infoList.isEmpty() && connectHistory->isEmpty()) {
        Q_EMIT discoveryFinished(false);
        isRunning = false;
        return;
    }

    Q_EMIT deviceOnline(infoList);
    Q_EMIT discoveryFinished(true);
    isRunning = false;
}

void MainController::onAppAttributeChanged(const QString &group, const QString &key, const QVariant &value)
{
    if (group != AppSettings::GenericGroup)
        return;

    if (key == AppSettings::StoragePathKey)
        CooperationUtil::instance()->setStorageConfig(value.toString());

    regist();
}

void MainController::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    LOG << "id: " << id << " result: " << result << " msg: " << msg.toStdString();
    switch (result) {
    case JOB_TRANS_FAILED:
        if (msg.contains("::not enough")) {
            showToast(false, tr("Insufficient storage space, file delivery failed this time. Please clean up disk space and try again!"));
        } else if (msg.contains("::off line")) {
            showToast(false, tr("Network not connected, file delivery failed this time. Please connect to the network and try again!"));
        } else {
            showToast(false, tr("File read/write exception"));
        }
        break;
    case JOB_TRANS_DOING:
        break;
    case JOB_TRANS_FINISHED: {
        showToast(true, tr("File sent successfully"));

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
        showToast(false, tr("The other party has canceled the file transfer"));
        break;
    default:
        break;
    }
}

void MainController::onFileTransStatusChanged(const QString &status)
{
    LOG << "file transfer info: " << status.toStdString();
#if 0
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

    DLOG << "progressbar: " << progressValue << " remain_time=" << remain_time;
    DLOG << "totalSize: " << transferInfo.totalSize << " transferSize=" << transferInfo.transferSize;

    updateProgress(progressValue, time.toString("hh:mm:ss"));
#endif
}

void MainController::onConfirmTimeout()
{
    isRequestTimeout = true;
    if (isReplied)
        return;

    static QString msg(tr("\"%1\" delivery of files to you was interrupted due to a timeout"));
    showToast(false, msg.arg(deepin_cross::CommonUitls::elidedText(requestFrom, Qt::ElideMiddle, MID_FRONT)));
}

void MainController::onAccepted()
{
    CooperationUtil::instance()->replyTransRequest(true);
#if defined(_WIN32) || defined(_WIN64)
    cooperationDlg->hide();
#endif
}

void MainController::onRejected()
{
    CooperationUtil::instance()->replyTransRequest(false);
#if defined(_WIN32) || defined(_WIN64)
    cooperationDlg->close();
#endif
}

void MainController::onCanceled()
{
    CooperationUtil::instance()->cancelTrans();
#if defined(_WIN32) || defined(_WIN64)
    cooperationDlg->close();
#endif
}

void MainController::onCompleted()
{
#if defined(_WIN32) || defined(_WIN64)
    cooperationDlg->close();
#else
    notifyIfc->call("CloseNotification", recvNotifyId);
#endif
}

void MainController::onViewed()
{
    QString path = recvFilesSavePath;
    if (path.isEmpty()) {
        auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
        path = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }
#if defined(_WIN32) || defined(_WIN64)
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    cooperationDlg->close();
#else
    QProcess::execute("dde-file-manager", QStringList() << path);
#endif
}

//private functions----------------------------------------------------------------

void MainController::initConnect()
{
    networkMonitorTimer = new QTimer(this);
    networkMonitorTimer->setInterval(1000);

    connect(networkMonitorTimer, &QTimer::timeout, this, &MainController::checkNetworkState);
    connect(ConfigManager::instance(), &ConfigManager::appAttributeChanged, this, &MainController::onAppAttributeChanged);
    connect(CooperationUtil::instance(), &CooperationUtil::discoveryFinished, this, &MainController::onDiscoveryFinished, Qt::QueuedConnection);
}


void MainController::discoveryDevice()
{
    if (!isOnline) {
        isRunning = false;
        Q_EMIT onlineStateChanged(isOnline);
        return;
    }

    QList<DeviceInfoPointer> offlineDevList;
    auto iter = connectHistory->begin();
    for (; iter != connectHistory->end(); ++iter) {
        DeviceInfoPointer info(new DeviceInfo(iter.key(), iter.value()));
        info->setConnectStatus(DeviceInfo::Offline);
        offlineDevList << info;
    }

    if (!offlineDevList.isEmpty())
        deviceOnline(offlineDevList);

    CooperationUtil::instance()->asyncDiscoveryDevice();
}
