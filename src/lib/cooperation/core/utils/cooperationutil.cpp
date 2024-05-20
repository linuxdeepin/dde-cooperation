// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationutil.h"
#include "cooperationutil_p.h"
#include "gui/mainwindow.h"
#include "maincontroller/maincontroller.h"
#include "transfer/transferhelper.h"
#include "cooperation/cooperationmanager.h"

#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"

#include "manager/sessionmanager.h"
#include "manager/sessionproto.h"
#include "share/cooconstrants.h"
#include "discovercontroller/discovercontroller.h"
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>

#define COO_SESSION_PORT 51566
#define COO_HARD_PIN "515616"

#define COO_WEB_PORT 51568

#ifdef linux
#    include <DFeatureDisplayDialog>
DWIDGET_USE_NAMESPACE
#endif
using namespace cooperation_core;

CooperationUtilPrivate::CooperationUtilPrivate(CooperationUtil *qq)
    : q(qq)
{
    bool onlyTransfer = qApp->property("onlyTransfer").toBool();
    LOG << "This is only transfer?" << onlyTransfer;

    ExtenMessageHandler msg_cb([this](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
        DLOG << "CooperationUtil >> " << mask << " msg_cb, json_msg: " << json_value << std::endl;
        switch (mask) {
        case APPLY_INFO: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;

            // response my device info.
            res.nick = q->deviceInfoStr().toStdString();
            *res_msg = res.as_json().serialize();

            // update this device info to discovery list
            auto devInfo = q->parseDeviceInfo(QString(req.nick.c_str()));
            if (devInfo && devInfo->isValid() && devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
                QList<DeviceInfoPointer> devInfoList;
                devInfoList << devInfo;
                Q_EMIT q->discoveryFinished(devInfoList);
            }
        }
            return true;
        case APPLY_TRANS: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            *res_msg = res.as_json().serialize();
            q->confirmTargetAddress = QString::fromStdString(req.host);
            q->storageFolder = QString::fromStdString(req.nick + "(" + req.host + ")");
            q->metaObject()->invokeMethod(MainController::instance(),
                                          "waitForConfirm",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.host.c_str())));
        }
            return true;
        case APPLY_TRANS_RESULT: {
            ApplyMessage req, res;
            req.from_json(json_value);
            bool agree = (req.flag == REPLY_ACCEPT);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(TransferHelper::instance(),
                                          agree ? "accepted" : "rejected",
                                          Qt::QueuedConnection);
        }
            return true;
        case APPLY_SHARE: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_WAIT;
            QString info = QString(req.host.c_str()) + "," + req.nick.c_str();
            *res_msg = res.as_json().serialize();
            q->confirmTargetAddress = QString::fromStdString(req.host);
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "notifyConnectRequest",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, info));
        }
            return true;
        case APPLY_SHARE_RESULT: {
            ApplyMessage req, res;
            req.from_json(json_value);
            bool agree = (req.flag == REPLY_ACCEPT);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(int, agree ? 1 : 0));
        }
            return true;
        case APPLY_SHARE_STOP: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleDisConnectResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, QString(req.host.c_str())));
        }
            return true;
        case APPLY_CANCELED: {
            ApplyMessage req, res;
            req.from_json(json_value);
            res.flag = DO_DONE;
            *res_msg = res.as_json().serialize();
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleCancelCooperApply",
                                          Qt::QueuedConnection);
        }
            return true;
        }

        // unhandle message
        return false;
    });

    sessionManager = new SessionManager(this);
    sessionManager->setSessionExtCallback(msg_cb);
    sessionManager->updatePin(COO_HARD_PIN);

    sessionManager->sessionListen(COO_SESSION_PORT);

    connect(sessionManager, &SessionManager::notifyConnection, this, &CooperationUtilPrivate::handleConnectStatus);
}

CooperationUtilPrivate::~CooperationUtilPrivate()
{
}

void CooperationUtilPrivate::handleConnectStatus(int result, QString reason)
{
    if (result == -2) {
        // error hanppend
        int code = reason.toInt();
        if (code == 113) {
            // host unreachable
            q->metaObject()->invokeMethod(CooperationManager::instance(),
                                          "handleSearchDeviceResult",
                                          Qt::QueuedConnection,
                                          Q_ARG(bool, false));
        }

    } else if (result == -1) {
        // disconnected
        // if there are trans or share working, notify network disconnect
        q->metaObject()->invokeMethod(MainController::instance(),
                                      "onNetworkMiss",
                                      Qt::QueuedConnection);
    } else if (result == 2) {
        // connected
        //        q->metaObject()->invokeMethod(CooperationManager::instance(),
        //                                                      "handleSearchDeviceResult",
        //                                                      Qt::QueuedConnection,
        //                                                      Q_ARG(bool, true));
    }

    //    q->metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
    //                                  Qt::QueuedConnection,
    //                                  Q_ARG(int, 0),
    //                                  Q_ARG(QString, reason),
    //                                  Q_ARG(bool, result == 2));
}

CooperationUtil::CooperationUtil(QObject *parent)
    : QObject(parent),
      d(new CooperationUtilPrivate(this))
{
}

CooperationUtil::~CooperationUtil()
{
}

CooperationUtil *CooperationUtil::instance()
{
    static CooperationUtil ins;
    return &ins;
}

QWidget *CooperationUtil::mainWindow()
{
    if (!d->window)
        d->window = new MainWindow;

    return d->window;
}

DeviceInfoPointer CooperationUtil::findDeviceInfo(const QString &ip)
{
    if (!d->window)
        return nullptr;

    return d->window->findDeviceInfo(ip);
}

void CooperationUtil::destroyMainWindow()
{
    if (d->window)
        delete d->window;
}

void CooperationUtil::registerDeviceOperation(const QVariantMap &map)
{
    d->window->onRegistOperations(map);
}

void CooperationUtil::pingTarget(const QString &ip)
{
    // session connect and then send rpc request
    bool pong = d->sessionManager->sessionPing(ip, COO_SESSION_PORT);
    //    emit CooperationManager::instance()->handleSearchDeviceResult(pong);

    // send info apply, and sync handle
    ApplyMessage msg;
    msg.flag = ASK_QUIET;
    msg.host = ip.toStdString();
    msg.nick = deviceInfoStr().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    QString res = d->sessionManager->sendRpcRequest(ip, APPLY_INFO, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send APPLY_TRANS failed.";
    } else {
        WLOG << "APPLY_TRANS replay:" << res.toStdString();
        picojson::value json_value;
        std::string err = picojson::parse(json_value, res.toStdString());
        if (!err.empty()) {
            DLOG << "Failed to parse JSON data: " << err;
            return;
        }

        ApplyMessage replay;
        replay.from_json(json_value);
        // update this device info to discovery list
        auto devInfo = parseDeviceInfo(replay.nick.c_str());
        if (devInfo && devInfo->isValid() && devInfo->discoveryMode() == DeviceInfo::DiscoveryMode::Everyone) {
            QList<DeviceInfoPointer> devInfoList;
            devInfoList << devInfo;
            Q_EMIT discoveryFinished(devInfoList);
        }
    }
}

void CooperationUtil::sendTransApply(const QString &ip)
{
    // session connect and then send rpc request
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);

    metaObject()->invokeMethod(TransferHelper::instance(), "onConnectStatusChanged",
                               Qt::QueuedConnection,
                               Q_ARG(int, logind ? 1 : 0),
                               Q_ARG(QString, ip),
                               Q_ARG(bool, true));

    if (logind) {
        // update the target address
        confirmTargetAddress = ip;

//        auto myselfInfo = DeviceInfo::fromVariantMap(CooperationUtil::deviceInfo());
        auto deviceName = deviceInfo().value(AppSettings::DeviceNameKey).toString();

        // send transfer apply, and async handle in RPC recv
        ApplyMessage msg;
        msg.flag = ASK_NEEDCONFIRM;
        msg.nick = deviceName.toStdString();// user define nice name
        msg.host = localIPAddress().toStdString();
        QString jsonMsg = msg.as_json().serialize().c_str();
        QString res = d->sessionManager->sendRpcRequest(ip, APPLY_TRANS, jsonMsg);
        if (res.isEmpty()) {
            // transfer request send exception, it perhaps network error
            WLOG << "Send APPLY_TRANS failed.";
        }
    }
}

void CooperationUtil::sendShareEvents(const QString &ip)
{
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);
    DeviceInfoPointer selfinfo = DiscoverController::instance()->findDeviceByIP(localIPAddress());
    // session connect and then send rpc request
    ApplyMessage msg;
    msg.flag = ASK_NEEDCONFIRM;
    msg.nick = selfinfo->deviceName().toStdString();
    msg.host = localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    QString res = d->sessionManager->sendRpcRequest(ip, APPLY_SHARE, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send ShareEvents failed.";
    }
}

void CooperationUtil::sendDisconnectShareEvents(const QString &ip)
{
    bool logind = d->sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);
    DeviceInfoPointer selfinfo = DiscoverController::instance()->findDeviceByIP(localIPAddress());
    // session connect and then send rpc request
    ApplyMessage msg;
    msg.flag = ASK_NEEDCONFIRM;
    msg.nick = selfinfo->deviceName().toStdString();
    msg.host = localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();
    QString res = d->sessionManager->sendRpcRequest(ip, APPLY_SHARE_STOP, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send ShareEvents failed.";
    }
}

void CooperationUtil::registAppInfo(const QString &infoJson)
{
    LOG << "regist app info: " << infoJson.toStdString();
    //TODO: ?registerDiscovery
}

void CooperationUtil::unregistAppInfo()
{
    //TODO: ?unregisterDiscovery
}

void CooperationUtil::asyncDiscoveryDevice()
{
    //TODO: ?getDiscovery then
    //    QList<DeviceInfoPointer> infoList;
    //    infoList = d->parseDeviceInfo(obj);
    //    Q_EMIT discoveryFinished(infoList);
}

void CooperationUtil::setStorageConfig(const QString &value)
{
    if (d->sessionManager) {
        d->sessionManager->setStorageRoot(value);
    }
}

void CooperationUtil::replyTransRequest(bool agree)
{
    auto deviceName = deviceInfo().value(AppSettings::DeviceNameKey).toString();
    // send transfer reply, skip result
    ApplyMessage msg;
    msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
    msg.nick = deviceName.toStdString();// user define nice name
    msg.host = localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();

    // _confirmTargetAddress
    QString res = d->sessionManager->sendRpcRequest(confirmTargetAddress, APPLY_TRANS_RESULT, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send APPLY_TRANS_RESULT failed.";
    }

    d->sessionManager->updateSaveFolder(storageFolder);
}

void CooperationUtil::replyShareRequest(bool agree)
{
    // send transfer reply, skip result
    ApplyMessage msg;
    msg.flag = agree ? REPLY_ACCEPT : REPLY_REJECT;
    msg.host = localIPAddress().toStdString();
    QString jsonMsg = msg.as_json().serialize().c_str();

    // _confirmTargetAddress
    QString res = d->sessionManager->sendRpcRequest(confirmTargetAddress, APPLY_SHARE_RESULT, jsonMsg);
    if (res.isEmpty()) {
        // transfer request send exception, it perhaps network error
        WLOG << "Send APPLY_SHARE_RESULT failed.";
    }
}

void CooperationUtil::cancelTrans()
{
    d->sessionManager->cancelSyncFile(confirmTargetAddress);
}

void CooperationUtil::doSendFiles(const QStringList &fileList)
{
    if (confirmTargetAddress.isEmpty()) {
        WLOG << "No confirm address!!!";
        return;
    }
    d->sessionManager->sendFiles(confirmTargetAddress, COO_WEB_PORT, fileList);
}

QString CooperationUtil::deviceInfoStr()
{
    auto infomap = deviceInfo();
    infomap.insert("IPAddress", localIPAddress());

    // 将QVariantMap转换为QJsonDocument转换为QString
    QJsonDocument jsonDocument = QJsonDocument::fromVariant(infomap);
    QString jsonString = jsonDocument.toJson(QJsonDocument::Compact);

    return jsonString;
}

DeviceInfoPointer CooperationUtil::parseDeviceInfo(const QString &info)
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(info.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        ELOG << "parse device info error";
        return nullptr;
    }

    auto map = doc.toVariant().toMap();
    //    map.insert("IPAddress", req.host.c_str());
    //                map.insert("OSType", req.flag);
    auto devInfo = DeviceInfo::fromVariantMap(map);
    devInfo->setConnectStatus(DeviceInfo::Connectable);
    //    if (lastInfo.os.share_connect_ip == node.os.ipv4)
    //        devInfo->setConnectStatus(DeviceInfo::Connected);

    return devInfo;
}

QVariantMap CooperationUtil::deviceInfo()
{
    QVariantMap info;
#ifdef linux
    auto value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::DiscoveryModeKey, 0);
    int mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 1) ? 1 : mode;
    info.insert(AppSettings::DiscoveryModeKey, mode);
#else
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DiscoveryModeKey);
    info.insert(AppSettings::DiscoveryModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::DeviceNameKey);
    info.insert(AppSettings::DeviceNameKey,
                value.isValid()
                        ? value.toString()
                        : QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).value(0)).dirName());

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::PeripheralShareKey);
    info.insert(AppSettings::PeripheralShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::LinkDirectionKey);
    info.insert(AppSettings::LinkDirectionKey, value.isValid() ? value.toInt() : 0);

#ifdef linux
    value = DConfigManager::instance()->value(kDefaultCfgPath, DConfigKey::TransferModeKey, 0);
    mode = value.toInt();
    mode = (mode < 0) ? 0 : (mode > 2) ? 2 : mode;
    info.insert(AppSettings::TransferModeKey, mode);
#else
    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::TransferModeKey);
    info.insert(AppSettings::TransferModeKey, value.isValid() ? value.toInt() : 0);
#endif

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    info.insert(AppSettings::StoragePathKey, storagePath);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::ClipboardShareKey);
    info.insert(AppSettings::ClipboardShareKey, value.isValid() ? value.toBool() : true);

    value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::CooperationEnabled);
    info.insert(AppSettings::CooperationEnabled, value.isValid() ? value.toBool() : true);

    value = deepin_cross::BaseUtils::osType();
    info.insert(AppSettings::OSType, value);

    return info;
}

QString CooperationUtil::localIPAddress()
{
    QString ip;
    ip = deepin_cross::CommonUitls::getFirstIp().data();
    return ip;
}

void CooperationUtil::showFeatureDisplayDialog(QDialog *dlg1)
{
#ifdef linux
    DFeatureDisplayDialog *dlg = static_cast<DFeatureDisplayDialog *>(dlg1);
    auto btn = dlg->getButton(0);
    btn->setText(tr("View Help Manual"));
    dlg->setTitle(tr("Welcome to dde-cooperation"));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_kvm.png"),
                                  tr("Keyboard and mouse sharing"), tr("When a connection is made between two devices, the initiator's keyboard and mouse can be used to control the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_clipboard.png"),
                                  tr("Clipboard sharing"), tr("Once a connection is made between two devices, the clipboard will be shared and can be copied on one device and pasted on the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_file.png"),
                                  tr("Delivery of documents"), tr("After connecting between two devices, you can initiate a file delivery to the other device"), dlg));
    dlg->addItem(new DFeatureItem(QIcon::fromTheme(":/icons/deepin/builtin/icons/tip_more.png"),
                                  tr("Usage"), tr("For detailed instructions, please click on the Help Manual below"), dlg));
    dlg->show();
#endif
}
