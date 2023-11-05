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
TransferHelper::TransferHelper() : QObject()
{
    initOnlineState();
#ifndef WIN32
    SettingHelper::instance();
#endif
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

bool TransferHelper::cancelTransferJob()
{
    return transferhandle.cancelTransferJob();
}

void TransferHelper::tryConnect(const QString &ip, const QString &password)
{
    transferhandle.tryConnect(ip, password);
}

QString TransferHelper::getJsonfile(const QJsonObject &jsonData, const QString &save)
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
        return savePath;
    } else {
        qDebug() << "Failed to open file for writing.";
        return QString();
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
    // add app
    QJsonArray appArray;
    for (auto app : appList) {
        appArray.append(app);
    }

    QString tempSavePath = QDir::tempPath();
    // add bookmarks
    QString bookmarksName;
    if (!browserList.isEmpty()) {
        QSet<QString> browserName(browserList.begin(), browserList.end());
        DrapWindowsData::instance()->getBrowserBookmarkInfo(browserName);
        QString bookmarksPath = DrapWindowsData::instance()->getBrowserBookmarkJSON(tempSavePath);
        transferFilePathList.append(bookmarksPath);
        bookmarksName = QFileInfo(bookmarksPath).fileName();
        OptionsManager::instance()->addUserOption(Options::KBookmarksJsonPath, { bookmarksPath });
    }

    // add wallpaper
    QString wallpaperName;
    if (!configList.isEmpty()) {
        QString wallparerPath = DrapWindowsData::instance()->getDesktopWallpaperPath();
        QFileInfo fileInfo(wallparerPath);
        wallpaperName = fileInfo.fileName();
        transferFilePathList.append(wallparerPath);
        OptionsManager::instance()->addUserOption(Options::KWallpaperPath, { wallparerPath });
    }

    // add file
    QJsonArray fileArray;
    for (QString file : filePathList) {
        qInfo() << QDir::homePath();
        if (file.contains(QDir::homePath()))
            file.replace(QDir::homePath() + "/", "");
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
        jsonObject["browserbookmark"] = bookmarksName;

    // add transfer.json
    QString jsonfilePath = getJsonfile(jsonObject, QString(tempSavePath));
    transferFilePathList.prepend(jsonfilePath);
    OptionsManager::instance()->addUserOption(Options::KUserDataInfoJsonPath, { jsonfilePath });
    qInfo() << "transfer.json save path:" << tempSavePath;
    return transferFilePathList;
}
#else

bool TransferHelper::checkSize(const QString &filepath)
{
    QJsonObject jsonObj = SettingHelper::ParseJson(filepath);
    if (jsonObj.isEmpty())
        return false;
    auto sizestr = jsonObj["user_data"].toString();
    auto size = QVariant(sizestr).toLongLong() / 1024 / 1024 / 1024 + 5;
    qInfo() << sizestr << "   jsonObj[ user_data ];" << size;
    int remainSize = getRemainSize();
    if (size > remainSize) {
        qInfo() << "outOfStorage" << size;
        emit outOfStorage(size);
        cancelTransferJob();
        return false;
    }
    return true;
}

void TransferHelper::recordTranferJob(const QString &filepath)
{
    // 1.copy transferjson to temp
    QFile jsonfile(filepath);
    QFileInfo info(jsonfile);
    QString tempPath(info.path() + "/transfer-temp.json");
    if (!jsonfile.copy(tempPath))
        qWarning() << "Failed to copy file";

    connect(this, &TransferHelper::interruption, this, [this, filepath, tempPath]() {
        // 2.write unfinished files to tempjson file
        QJsonObject jsonObj = SettingHelper::ParseJson(filepath);

        QJsonArray userFileArray = jsonObj["user_file"].toArray();
        QJsonArray updatedFileList;

        foreach (const QJsonValue &fileValue, userFileArray) {
            QString file = fileValue.toString();
            QString filename = file.mid(file.indexOf('/'));

            // skip finished files
            if (finshedFiles.contains(filename)) {
                continue;
            }
            updatedFileList.append(file);
        }
        // 3.save unfinished filelist for retransmission
        jsonObj["user_file"] = updatedFileList;
        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObj);
        QFile tempfile(tempPath);
        if (!tempfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            qWarning() << "Failed to open JSON file for writing";
        tempfile.write(jsonDoc.toJson());
        tempfile.close();
    });

    connect(this, &TransferHelper::transferSucceed, this, [this, tempPath](bool isall) {
        Q_UNUSED(isall)
        finshedFiles.clear();
        QFile tempfile(tempPath);
        tempfile.remove(tempPath);
    });
}

bool TransferHelper::isUnfinishedJob(const QString &user)
{
    QFile f(QDir::homePath() + "/" + user + "/transfer-temp.json");
    if (!f.exists())
        return false;
    qInfo() << user + "has UnfinishedJob";

    QJsonObject jsonObj = SettingHelper::ParseJson(f.fileName());
    QJsonArray fileArray = jsonObj["user_file"].toArray();

    QStringList unfinishedFiles;
    foreach (const QJsonValue &fileValue, fileArray) {
        QString file = fileValue.toString();
        unfinishedFiles.append(file);
    }
    qInfo() << "unfinishedFiles" << unfinishedFiles;
    return true;
}

void TransferHelper::addFinshedFiles(const QString &filepath)
{
    if (filepath.contains("transfer.json"))
        TransferHelper::instance()->recordTranferJob(filepath);
    finshedFiles.append(filepath);
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
