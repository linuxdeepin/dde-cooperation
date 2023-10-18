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
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>

#include <QTextCodec>
#include <QScreen>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>
#include <QJsonArray>

#ifdef WIN32
#    include <gui/getbackup/drapwindowsdata.h>
#else
#    include <QDBusMessage>
#    include <QDBusConnection>
#    include <QDBusInterface>
#endif

#pragma execution_character_set("utf-8")
TransferHelper::TransferHelper() : QObject()
{
    initOnlineState();
}

TransferHelper::~TransferHelper() { }

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
#ifdef WIN32
        pingProcess.start("ping",
                          QStringList() << "-n"
                                        << "1"
                                        << "www.baidu.com");
#else
        pingProcess.start("ping",
                          QStringList() << "-c"
                                        << "1"
                                        << "www.baidu.com");
#endif
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


void TransferHelper::getJsonfile(const QJsonObject &jsonData, const QString &save)
{
    QString savePath = save;
    QJsonDocument jsonDoc(jsonData);

    if (savePath.isEmpty()) {
        savePath = QString("./transfer.json");
    } else {
        savePath += "/transfer.json";
    }

    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonDoc.toJson());
        file.close();
        qDebug() << "JSON data exported to transfer.json";
    } else {
        qDebug() << "Failed to open file for writing.";
    }
}

#ifdef WIN32
void TransferHelper::startTransfer()
{
    QStringList paths = getTransferFilePath();
    qInfo() << "transferring file list: " << paths;
    transferhandle.sendFiles(paths);
}

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

QStringList TransferHelper::getTransferFilePath()
{
    QStringList filePathList = OptionsManager::instance()->getUserOption(Options::kFile);
    QStringList appList = OptionsManager::instance()->getUserOption(Options::kApp);
    QStringList browserList = OptionsManager::instance()->getUserOption(Options::kBrowserBookmarks);
    QStringList configList = OptionsManager::instance()->getUserOption(Options::kConfig);

    QStringList transferFilePathList;
    for (auto file : filePathList) {
        transferFilePathList.append(file);
    }

    QString tempSavePath = QCoreApplication::applicationDirPath();

    QString bookmarksName;
    if (!browserList.isEmpty()) {
        QSet<QString> browserName(browserList.begin(), browserList.end());
        DrapWindowsData::instance()->getBrowserBookmarkInfo(browserName);
        DrapWindowsData::instance()->getBrowserBookmarkJSON(QString("."));
        transferFilePathList.append(tempSavePath + QString("/bookmarks.json"));
        bookmarksName = "bookmarks.json";
    }

    QString wallpaperName;
    if (!configList.isEmpty()) {
        QString wallparerPath = DrapWindowsData::instance()->getDesktopWallpaperPath();
        QFileInfo fileInfo(wallparerPath);
        wallpaperName = fileInfo.fileName();
        transferFilePathList.append(QString(fileInfo.path() + "/" + wallpaperName));
    }

    QJsonArray appArray;
    for (auto app : appList) {
        appArray.append(app);
    }
    QJsonArray fileArray;
    for (QString file : filePathList) {
        if (file.contains("C:/Users/deep/")) {
            file.replace("C:/Users/deep/", "");
        } else {
            int found = file.indexOf(":/");
            if (found != -1) {
                file = file.mid(found + 2);
            }
        }
        fileArray.append(file);
    }

    QJsonObject jsonObject;
    jsonObject["user_data"] = "NA";
    jsonObject["user_file"] = fileArray;
    if (!appArray.isEmpty())
        jsonObject["app"] = appArray;
    //  jsonObject["app"] = appArray;
    if (!wallpaperName.isEmpty())
        jsonObject["wallpapers"] = wallpaperName;
    if (!bookmarksName.isEmpty())
        jsonObject["borwserbookmark"] = bookmarksName;

    QString qjsonPath("/transfer.json");
    getJsonfile(jsonObject, QString(tempSavePath));
    transferFilePathList.append(tempSavePath + qjsonPath);

    return transferFilePathList;
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

    bool ret = true;

    // Configure desktop wallpaper
    QString image = filepath + "/" + jsonObj["wallpapers"].toString();
    if(!jsonObj["wallpapers"].isNull())
         ret &= setWallpaper(image);

    //Configure file
    ret &= setFile(jsonObj, filepath);

    //setBrowserBookMark
    ret &= setBrowserBookMark(jsonObj["browerbookmark"].toString());

    //installApps
    ret &= jsonObj["app"].isNull();

    return ret;
}

bool TransferHelper::setWallpaper(const QString &filepath)
{
    qInfo() << "Setting picture as wallpaper" << filepath;
    //服务
    QString service = "com.deepin.daemon.Appearance";
    QString path = "/com/deepin/daemon/Appearance";
    QString interfaceName = "com.deepin.daemon.Appearance";

    // dbus连接
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

bool TransferHelper::setBrowserBookMark(const QString &filepath)
{
    if (filepath.isEmpty())
        return true;
    else
        return false;
}

bool TransferHelper::installApps(const QStringList &applist)
{
    if (applist.isEmpty())
        return true;
    else
        return false;
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
