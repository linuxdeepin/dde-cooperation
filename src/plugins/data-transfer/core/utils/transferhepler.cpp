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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QJsonArray>

#ifdef WIN32
#    include <gui/getbackup/drapwindowsdata.h>
#endif

#pragma execution_character_set("utf-8")
TransferHelper::TransferHelper()
    : QObject()
{
    initOnlineState();
}

TransferHelper::~TransferHelper() {}

TransferHelper *TransferHelper::instance()
{
    static TransferHelper ins;
    return &ins;
}

void TransferHelper::initOnlineState()
{
    // 发送网络请求并等待响应
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        QProcess pingProcess;
        pingProcess.start("ping", QStringList() << "-c"
                                                << "1"
                                                << "www.baidu.com");
        pingProcess.waitForFinished(500);
        if (pingProcess.exitCode() == 0 && online != true) {
            online = true;
            emit onlineStateChanged(online);
            qInfo() << "Network is connected";
        }
        if (pingProcess.exitCode() != 0 && online == true) {
            online = false;
            emit onlineStateChanged(online);
            qInfo() << "Network is not connected";
        }
    });

    timer->start(1000);
}

bool TransferHelper::getOnlineState() const
{
    return online;
}

QString TransferHelper::getConnectPassword()
{
    return transferhandle.getConnectPassWord();
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

    //Configure file
    setFile(jsonObj, filepath);

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

bool TransferHelper::setFile(QJsonObject jsonObj, QString filepath)
{
    QJsonValue userFileValue = jsonObj["user_file"];
    if (userFileValue.isArray()) {
        const QJsonArray &userFileArray = userFileValue.toArray();
        for (const auto &value : userFileArray) {
            QString filename = value.toString();
            QString targetFile = QDir::homePath() + "/" + filename;
            QString file = filepath + filename.mid(filename.indexOf('/'));
            bool success = QFile::rename(file, targetFile);
            qInfo() << file << success;
        }
    }
    qInfo() << jsonObj["user_file"].toString();
    return true;
}

#endif
