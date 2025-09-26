// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "transferhelper.h"
#include "transferhelper_p.h"

#include "common/constant.h"
#include "common/commonutils.h"
#include "discover/deviceinfo.h"
#include "gui/mainwindow.h"

#ifdef ENABLE_COMPAT
#include <slotipc/interface.h>
#endif

#include <QDesktopServices>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTime>
#include <QProcess>
#include <QJsonDocument>

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr char TransferButtonId[] { "transfer-button" };

inline constexpr char NotifyCancelAction[] { "cancel" };
inline constexpr char NotifyRejectAction[] { "reject" };
inline constexpr char NotifyAcceptAction[] { "accept" };
inline constexpr char NotifyCloseAction[] { "close" };
inline constexpr char NotifyViewAction[] { "view" };

inline constexpr char BackendProcIPC[] { "dde-cooperation" };

#ifdef linux
inline constexpr char Ksend[] { "send" };
#else
inline constexpr char Ksend[] { ":/icons/deepin/builtin/texts/send_18px.svg" };
#endif

using namespace deepin_cross;
using namespace cooperation_transfer;
using namespace cooperation_core;

TransferHelperPrivate::TransferHelperPrivate(TransferHelper *qq)
    : QObject(qq),
      q(qq)
{
    DLOG << "TransferHelperPrivate constructor entered";
#ifdef ENABLE_COMPAT
    ipcInterface = new SlotIPCInterface();
    DLOG << "SlotIPCInterface created for compatibility mode";
#endif
}

TransferHelperPrivate::~TransferHelperPrivate()
{
    DLOG << "TransferHelperPrivate destructor entered";
#ifdef ENABLE_COMPAT
    // Ensure IPC interface is properly disconnected and cleaned up
    if (ipcInterface) {
        if (backendOk) {
            DLOG << "Disconnecting IPC interface in destructor";
            ipcInterface->disconnectFromServer();
        }
        delete ipcInterface;
        ipcInterface = nullptr;
        DLOG << "IPC interface cleaned up";
    }
#endif
}


TransferHelper::TransferHelper(QObject *parent)
    : QObject(parent),
      d(new TransferHelperPrivate(this))
{
    DLOG << "TransferHelper constructor entered";

    backendWatcher = new QTimer(this);
    backendWatcher->setInterval(2000);
    connect(backendWatcher, &QTimer::timeout, this, &TransferHelper::timeWatchBackend);
    backendWatcher->start();
    DLOG << "Backend watcher timer started with 2s interval";

#ifdef ENABLE_COMPAT
    // try connect backend by delay 500ms
    QTimer::singleShot(500, this, &TransferHelper::timeConnectBackend);
    DLOG << "Scheduled backend connection attempt in 500ms";
#endif
}

TransferHelper::~TransferHelper()
{
}

TransferHelper *TransferHelper::instance()
{
    DLOG << "Getting TransferHelper instance";
    static TransferHelper ins;
    return &ins;
}

void TransferHelper::registBtn(cooperation_core::MainWindow *window)
{
    DLOG << "Registering transfer button to MainWindow";

    ClickedCallback clickedCb = TransferHelper::buttonClicked;
    ButtonStateCallback visibleCb = TransferHelper::buttonVisible;
    ButtonStateCallback clickableCb = TransferHelper::buttonClickable;

    QVariantMap transferInfo { { "id", TransferButtonId },
                               { "description", tr("Send files") },
                               { "icon-name", Ksend },
                               { "location", 3 },
                               { "button-style", 1 },
                               { "clicked-callback", QVariant::fromValue(clickedCb) },
                               { "visible-callback", QVariant::fromValue(visibleCb) },
                               { "clickable-callback", QVariant::fromValue(clickableCb) } };

    window->onRegistOperations(transferInfo);
    DLOG << "Transfer button registered with ID:" << TransferButtonId;
}

void TransferHelper::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    DLOG << "Button clicked event received, ID:" << id.toStdString();

    QString ip = info->ipAddress();
    QString name = info->deviceName();
    LOG << "button clicked, button id: " << id.toStdString()
        << " ip: " << ip.toStdString()
        << " device name: " << name.toStdString();

    if (id == TransferButtonId) {
        DLOG << "Transfer button clicked for device:" << name.toStdString() << "IP:" << ip.toStdString();

        QStringList selectedFiles = qApp->property("sendFiles").toStringList();
        if (selectedFiles.isEmpty()) {
            DLOG << "No pre-selected files, opening file dialog";
            selectedFiles = QFileDialog::getOpenFileNames(qApp->activeWindow());
        }

        if (selectedFiles.isEmpty()) {
            WLOG << "No files selected for transfer";
            return;
        }

        DLOG << "Selected" << selectedFiles.size() << "files for transfer";
        // send command to local socket.
        Q_EMIT TransferHelper::instance()->sendFiles(ip, name, selectedFiles);
        
        // Delay exit to ensure signal processing is complete
        QTimer::singleShot(500, TransferHelper::instance(), &TransferHelper::gracefulShutdown);
        DLOG << "Transfer initiated, scheduled graceful shutdown in 500ms";
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
        case DeviceInfo::TransMode::NotAllow:
            DLOG << "Transfer mode is NotAllow, hiding transfer button";
            return false;
        default:
            return false;
        }
    }

    return true;
}

bool TransferHelper::buttonClickable(const QString &id, const DeviceInfoPointer info)
{
    Q_UNUSED(id)
    Q_UNUSED(info)

    return true;
}

void TransferHelper::timeConnectBackend()
{
    DLOG << "Attempting to connect to backend IPC";
#ifdef ENABLE_COMPAT
    QString ipcName = CommonUitls::ipcServerName(BackendProcIPC);
    DLOG << "Connecting to backend IPC:" << ipcName.toStdString();
    d->backendOk = d->ipcInterface->connectToServer(ipcName);
    if (d->backendOk) {
        DLOG << "Backend connection successful";
        // bind SIGNAL to SLOT
        d->ipcInterface->remoteConnect(SIGNAL(searched(QString)), this, SLOT(searchResultSlot(QString)));
        d->ipcInterface->remoteConnect(SIGNAL(refreshed(QStringList)), this, SLOT(refreshResultSlot(QStringList)));
        d->ipcInterface->remoteConnect(SIGNAL(deviceChanged(bool, QString)), this, SLOT(deviceChangedSlot(bool, QString)));

        d->ipcInterface->remoteConnect(this, SIGNAL(refresh()), SLOT(onRefreshDevice()));
        d->ipcInterface->remoteConnect(this, SIGNAL(search(QString)), SLOT(onSearchDevice(QString)));
        d->ipcInterface->remoteConnect(this, SIGNAL(sendFiles(QString, QString, QStringList)), SLOT(onSendFiles(QString, QString, QStringList)));

        LOG << "SUCCESS connect to depending backend: " << ipcName.toStdString();
        // first, refresh & get device list
        Q_EMIT refresh();
        DLOG << "Initial device refresh triggered";
    } else {
        // TODO: show dialog
        WLOG << "can not connect to: " << ipcName.toStdString();
    }
#else
    DLOG << "Compatibility mode not enabled, skipping backend connection";
#endif
}

void TransferHelper::timeWatchBackend()
{
#ifdef __linux__
    DLOG << "Watching backend process on Linux";
    QProcess process;
    // get the related process count
    process.start("pgrep", QStringList() << "-c" << "-f" << "dde-cooperation");

    if (!process.waitForFinished(2000)) {
        DLOG << "pgrep process timed out";
        return;
    }

    QString output = process.readAllStandardOutput();
    bool backendOK = (!output.isEmpty() && output.toInt() > 2);
    if (!backendOK) {
        DLOG << "Backend process not running or count is too low";
        //TODO: show tip
    } else {
        DLOG << "Backend process is running";
    }
#else
    DLOG << "Skipping backend watch on non-Linux platform";
#endif
}

void TransferHelper::searchResultSlot(const QString& info)
{
    DLOG << "Search result received, data:" << info.toStdString();

    if (info.isEmpty()) {
        WLOG << "Empty search result received";
        // From remove onFinishedDiscovery(false)
        Q_EMIT finishDiscovery(false);
        return;
    }

    auto devInfo = parseFromJson(info);
    if (transable(devInfo)) {
        DLOG << "New transable device found:" << devInfo->deviceName().toStdString();
        Q_EMIT onlineDevices({ devInfo });
    } else if (devInfo) {
        DLOG << "Non-transable device filtered out:" << devInfo->deviceName().toStdString();
        // filter the invisible device
        auto ip = devInfo->ipAddress();
        Q_EMIT offlineDevice(ip);
    }
}

void TransferHelper::refreshResultSlot(const QStringList& infoList)
{
    DLOG << "Received refresh result with" << infoList.size() << "devices";
    QList<DeviceInfoPointer> devList;
    for (auto info : infoList) {
        auto devInfo = parseFromJson(info);
        if (transable(devInfo)) {
            DLOG << "Device is transable:" << devInfo->deviceName().toStdString();
            devList << devInfo;
        } else {
            DLOG << "Device is not transable:" << devInfo->deviceName().toStdString();
        }
    }

    bool found = !devList.isEmpty();
    if (found) {
        DLOG << "Online devices found, emitting signal";
        Q_EMIT onlineDevices(devList);
    } else {
        DLOG << "No online devices found";
    }

    Q_EMIT finishDiscovery(found);
}

void TransferHelper::deviceChangedSlot(bool found, const QString& info)
{
    DLOG << "Device changed slot, found:" << found << "info:" << info.toStdString();
    if (found) {
        DLOG << "Device found, calling searchResultSlot";
        searchResultSlot(info);
    } else {
        DLOG << "Device not found, emitting offlineDevice";
        Q_EMIT offlineDevice(info);
    }
}

DeviceInfoPointer TransferHelper::parseFromJson(const QString &info)
{
    DLOG << "Parsing device info from JSON, data length:" << info.length();

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(info.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        WLOG << "parse search info error, info:" << info.toStdString();
        return nullptr;
    }

    auto map = doc.toVariant().toMap();
    auto devInfo = DeviceInfo::fromVariantMap(map);
    devInfo->setConnectStatus(DeviceInfo::Connectable);

    DLOG << "Successfully parsed device info for:" << devInfo->deviceName().toStdString();
    return devInfo;
}

bool TransferHelper::transable(const DeviceInfoPointer devInfo)
{
    DLOG << "Checking transability for device:" << devInfo->deviceName().toStdString();
    if (!devInfo || !devInfo->isValid()) {
        DLOG << "Device info is invalid or null, returning false";
        return false;
    }

    if (DeviceInfo::TransMode::Everyone == devInfo->transMode()) {
        DLOG << "Transfer mode is Everyone, returning true";
        return true;
    }

    if (DeviceInfo::TransMode::OnlyConnected == devInfo->transMode() &&
        DeviceInfo::ConnectStatus::Connected == devInfo->connectStatus()) {
        DLOG << "Transfer mode is OnlyConnected and device is Connected, returning true";
        return true;
    }

    // For NotAllow mode, return true to show device in list
    // Button visibility will be controlled by buttonVisible() function
    if (DeviceInfo::TransMode::NotAllow == devInfo->transMode()) {
        DLOG << "Transfer mode is NotAllow, showing device but disabling transfer";
        return true;
    }

    DLOG << "Device is not transable, returning false";
    return false;
}

void TransferHelper::gracefulShutdown()
{
    DLOG << "Starting graceful shutdown process";
    
#ifdef ENABLE_COMPAT
    // Disconnect IPC interface first to ensure clean resource cleanup
    if (d && d->ipcInterface && d->backendOk) {
        DLOG << "Disconnecting from IPC interface";
        d->ipcInterface->disconnectFromServer();
        d->backendOk = false;
        DLOG << "IPC interface disconnected successfully";
    }
#endif
    
    // Give a small additional delay for cleanup to complete
    QTimer::singleShot(100, qApp, []() {
        DLOG << "Exiting application after cleanup";
        qApp->exit(0);
    });
}
