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
#    include "settinghepler.h"
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
    //add app
    QJsonArray appArray;
    for (auto app : appList) {
        appArray.append(app);
    }
    QString tempSavePath = QCoreApplication::applicationDirPath();

    //add bookmarks
    QString bookmarksName;
    if (!browserList.isEmpty()) {
        QSet<QString> browserName(browserList.begin(), browserList.end());
        DrapWindowsData::instance()->getBrowserBookmarkInfo(browserName);
        DrapWindowsData::instance()->getBrowserBookmarkJSON(QString("."));
        transferFilePathList.append(tempSavePath + QString("/bookmarks.json"));
        bookmarksName = "bookmarks.json";
    }

    //add wallpaper
    QString wallpaperName;
    if (!configList.isEmpty()) {
        QString wallparerPath = DrapWindowsData::instance()->getDesktopWallpaperPath();
        QFileInfo fileInfo(wallparerPath);
        wallpaperName = fileInfo.fileName();
        transferFilePathList.append(QString(fileInfo.path() + "/" + wallpaperName));
    }



    //add file
    QJsonArray fileArray;
    for (QString file : filePathList) {
        qInfo()<<QDir::homePath();
        if (file.contains(QDir::homePath())) {
            file.replace(QDir::homePath(), "");
        } else {
            int found = file.indexOf(":/");
            if (found != -1) {
                file = file.mid(found + 2);
            }
        }
        fileArray.append(file);
    }

    QJsonObject jsonObject;
    QString userData = OptionsManager::instance()->getUserOption(Options::KSelectFileSize)[0];
    jsonObject["user_data"] = userData;
    jsonObject["user_file"] = fileArray;
    if (!appArray.isEmpty())
        jsonObject["app"] = appArray;
    if (!wallpaperName.isEmpty())
        jsonObject["wallpapers"] = wallpaperName;
    if (!bookmarksName.isEmpty())
        jsonObject["borwserbookmark"] = bookmarksName;

    //add transfer.json
    QString qjsonPath("/transfer.json");
    getJsonfile(jsonObject, QString(tempSavePath));
    transferFilePathList.prepend(tempSavePath + qjsonPath);

    return transferFilePathList;
}
#else

bool TransferHelper::checkSize(const QString &filepath)
{
    QJsonObject jsonObj = SettingHelper::ParseJson(filepath);
    if (jsonObj.isEmpty())
        return false;
    auto size = jsonObj["user_data"].toInt();
    qInfo() << "jsonObj[ user_data ].toInt();" << size;
    int remainSize = getRemainSize();
    if (size < remainSize) {
        //emit outOfStorage(size);
        return false;
    }
    return true;
}

void TransferHelper::setting(const QString &filepath)
{
    SettingHelper::instance()->handleDataConfiguration(filepath);
}

int TransferHelper::getRemainSize()
{
    QStorageInfo storage("/data");
    auto remainSize = storage.bytesAvailable() / 1024 / 1024 / 1024;
    return static_cast<int>(remainSize);
}
#endif
