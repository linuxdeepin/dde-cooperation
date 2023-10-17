#include "optionsmanager.h"
#include "transferhepler.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QDir>
#include <QStorageInfo>
#include <QCoreApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QGuiApplication>
#include <QTextCodec>
#include <QScreen>

#ifdef WIN32
#    include <gui/getbackup/drapwindowsdata.h>
#endif

#pragma execution_character_set("utf-8")
TransferHelper::TransferHelper()
    : QObject() {}

TransferHelper::~TransferHelper() {}

TransferHelper *TransferHelper::instance()
{
    static TransferHelper ins;
    return &ins;
}

const QStringList TransferHelper::getUesr()
{
    return QStringList() << "UOS-user1"
                         << "UOS-user2"
                         << "UOS-user3";
}

QString TransferHelper::getConnectPassword()
{
    return transferhandle.getConnectPassWord();
}

QMap<QString, double> TransferHelper::getUserDataSize()
{
    QMap<QString, double> userStorage;
    userStorage["documents"] = 13;
    userStorage["music"] = 6;
    userStorage["picture"] = 2;
    userStorage["movie"] = 3;
    userStorage["download"] = 9;

    return userStorage;
}

qint64 TransferHelper::getRemainStorage()
{
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    foreach (const QStorageInfo &drive, drives) {
        if (drive.device().startsWith("/dev/sd")) {
            QString deviceName = drive.device();
            QString displayName = drive.displayName();
            qint64 bytesFree = drive.bytesFree();
            qint64 bytesTotal = drive.bytesTotal();

            qInfo() << "Device Name:" << deviceName;
            qInfo() << "Display Name:" << displayName;
            qInfo() << "Free Space (Bytes):" << bytesFree;
            qInfo() << "Total Space (Bytes):" << bytesTotal;
            qInfo() << "Free Space (GB):" << bytesFree / (1024 * 1024 * 1024);
            qInfo() << "Total Space (GB):" << bytesTotal / (1024 * 1024 * 1024);

            return bytesFree / (1024 * 1024 * 1024);
        }
    }

    return 0;
}

void TransferHelper::tryConnect(const QString &ip, const QString &password)
{
    transferhandle.tryConnect(ip, password);
}

void TransferHelper::startTransfer()
{
    qInfo() << OptionsManager::instance()->getUserOptions();
    QStringList paths = OptionsManager::instance()->getUserOptions()["file"];
    transferhandle.sendFiles(paths);
}

#ifdef WIN32
QMap<QString, QString> TransferHelper::getAppList()
{
    QMap<QString, QString> appList;
    QMap<QString, QString> appNameList =
            DrapWindowsData::instance()->RecommendedInstallationAppList();

    for (auto iterator = appNameList.begin(); iterator != appNameList.end(); iterator++) {
        appList[iterator.key()] = QString(":/icon/AppIcons/%1.svg").arg(iterator.value());
    }

    return appList;
}

QMap<QString, QString> TransferHelper::getBrowserList()
{
    QMap<QString, QString> browserList;
    QSet<QString> borwserNameList = DrapWindowsData::instance()->getBrowserList();
    for (QString name : borwserNameList) {
        if (name == BrowserName::GoogleChrome) {
            browserList[name] = ":/icon/AppIcons/cn.google.chrome.svg";
        } else if (name == BrowserName::MicrosoftEdge) {
            browserList[name] = ":/icon/AppIcons/com.browser.softedge.stable.svg";
        } else if (name == BrowserName::MozillaFirefox) {
            browserList[name] = ":/icon/AppIcons/com.mozilla.firefox-zh.svg";
        }
    }
    return browserList;
}
#else
bool TransferHelper::handleDataConfiguration(const QString &filepath)
{
    QFile file(filepath + "/" + "transfer.json");
    qInfo() << "Parsing the configuration file for transmission" << file.fileName();
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "could not open datajson file";
        return false;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        qWarning() << "Parsing JSON data failed";
        return false;
    }
    QJsonObject jsonObj = jsonDoc.object();

    //Configure desktop wallpaper
    QString image = filepath + "/" + jsonObj["wallpapers"].toString();
    setWallpaper(image);

    return true;
}

bool TransferHelper::setWallpaper(const QString &filepath)
{
    qInfo() << "Setting picture as wallpaper" << filepath;
    //服务
    QString service = "com.deepin.daemon.Appearance";
    QString path = "/com/deepin/daemon/Appearance";
    QString interfaceName = "com.deepin.daemon.Appearance";

    //dbus连接
    QDBusInterface interface(service, path, interfaceName);

    //调用方法
    QString func = "SetMonitorBackground";
    QString screenName = QGuiApplication::screens().first()->name();
    QVariant monitorName = QVariant::fromValue(screenName);
    QVariant imageFile = QVariant::fromValue(filepath);

    // 发送DBus消息并等待回复
    QDBusMessage reply = interface.call(func, monitorName, imageFile);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        qDebug() << "SetMonitorBackground method called successfully";
        return true;
    } else {
        qDebug() << "Failed to call SetMonitorBackground method";
        return false;
    }
}
#endif
