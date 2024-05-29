#include "transferhepler.h"

#include "utils/optionsmanager.h"
#include "utils/settinghepler.h"
#include "utils/transferutil.h"
#include "common/commonutils.h"

#include "net/networkutil.h"

#include <QDebug>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#if defined(_WIN32) || defined(_WIN64)
#    include <gui/win/drapwindowsdata.h>
#endif

TransferHelper::TransferHelper()
    : QObject()
{
#ifdef __linux__
    SettingHelper::instance();
    connect(this, &TransferHelper::transferFinished, this, [this]() { isSetting = false; });
#endif
}

TransferHelper::~TransferHelper() {}

TransferHelper *TransferHelper::instance()
{
    static TransferHelper ins;
    return &ins;
}

QString TransferHelper::updateConnectPassword()
{
    QString password = TransferUtil::generateRandomNumber();

    NetworkUtil::instance()->updatePassword(password);
    return password;
}

void TransferHelper::tryConnect(const QString &ip, const QString &password)
{
    NetworkUtil::instance()->doConnect(ip, password);
}

bool TransferHelper::cancelTransferJob()
{
    // return _transferhandle->cancelTransferJob();
    return true;
}

void TransferHelper::disconnectRemote()
{
    //  _transferhandle->disconnectRemote();
}

void TransferHelper::emitDisconnected()
{
#ifdef linux
    if (!isSetting)
#endif
        emit disconnected();
}
#ifndef __linux__

void TransferHelper::startTransfer()
{
    QStringList filePathList = OptionsManager::instance()->getUserOption(Options::kFile);
    QStringList appList = OptionsManager::instance()->getUserOption(Options::kApp);
    QStringList browserList = OptionsManager::instance()->getUserOption(Options::kBrowserBookmarks);
    QStringList configList = OptionsManager::instance()->getUserOption(Options::kConfig);

    QStringList paths = getTransferFilePath(filePathList, appList, browserList, configList);
    qInfo() << "transferring file list: " << paths;
    _transferhandle->sendFiles(paths);
}

QMap<QString, QString> TransferHelper::getAppList(QMap<QString, QString> &noRecommedApplist)
{
    QMap<QString, QString> appList;
    QMap<QString, QString> appNameList =
            DrapWindowsData::instance()->RecommendedInstallationAppList(noRecommedApplist);

    for (auto iterator = appNameList.begin(); iterator != appNameList.end(); iterator++) {
        appList[iterator.key()] = QString(":/icon/AppIcons/%1.svg").arg(iterator.value());
    }

    return appList;
}

QMap<QString, QString> TransferHelper::getBrowserList()
{
    QMap<QString, QString> browserList;
    QStringList borwserNameList = DrapWindowsData::instance()->getBrowserList();
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

QStringList TransferHelper::getTransferFilePath(QStringList filePathList, QStringList appList,
                                                QStringList browserList, QStringList configList)
{
    // add files
    QStringList transferFilePathList;
    for (auto file : filePathList) {
        transferFilePathList.append(file);
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

    // add transfer.json
    QString jsonfilePath = getTransferJson(appList, filePathList, browserList, bookmarksName,
                                           wallpaperName, tempSavePath);

    transferFilePathList.prepend(jsonfilePath);
    OptionsManager::instance()->addUserOption(Options::KUserDataInfoJsonPath, { jsonfilePath });

    OptionsManager::instance()->addUserOption(Options::kTransferFileList, transferFilePathList);

    return transferFilePathList;
}

void TransferHelper::Retransfer(const QString jsonstr)
{
    QJsonObject jsonObj;
    QByteArray jsonData = jsonstr.toUtf8();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        WLOG << "Parsing JSON data failed: " << jsonstr.toStdString();
        return;
    }
    jsonObj = jsonDoc.object();
    if (jsonObj.isEmpty()) {
        WLOG << "this job none file";
        return;
    }

    // parse filePathList appList browserList configList from json.
    QJsonArray userFileArray = jsonObj["user_file"].toArray();
    QStringList filePathList;
    foreach (const QJsonValue &fileValue, userFileArray) {
        QString file = QDir::homePath() + "/" + fileValue.toString();
        filePathList.append(file);
    }
    QJsonArray appArray = jsonObj["app"].toArray();
    QStringList appList;
    foreach (const QJsonValue &value, appArray) {
        QString file = value.toString();
        appList.append(file);
    }
    QJsonArray browserArray = jsonObj["browsersName"].toArray();
    QStringList browserList;
    foreach (const QJsonValue &value, browserArray) {
        QString file = value.toString();
        browserList.append(file);
    }

    QStringList configList;
    if (jsonObj.contains("wallpapers")) {
        configList.append(jsonObj["wallpapers"].toString());
    }

    QString userData = jsonObj["user_data"].toString();
    QStringList sizelist;
    sizelist.append(userData);
    OptionsManager::instance()->addUserOption(Options::KSelectFileSize, sizelist);
    LOG << "user select file size:"
        << OptionsManager::instance()->getUserOption(Options::KSelectFileSize)[0].toStdString();

    QStringList paths = getTransferFilePath(filePathList, appList, browserList, configList);
    qInfo() << "continue last file list: " << paths;
    LOG << "continue last file size:"
        << OptionsManager::instance()->getUserOption(Options::KSelectFileSize)[0].toStdString();
    _transferhandle->sendFiles(paths);

    emit changeWidget(PageName::transferringwidget);
}

QString TransferHelper::defaultBackupFileName()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDateTime = currentDateTime.toString("yyyyMMddhhmm");

    return QString(DrapWindowsData::instance()->getUserName() + "_"
                   + DrapWindowsData::instance()->getIP() + "_" + formattedDateTime);
}
#endif

void TransferHelper::recordTranferJob(const QString &filepath)
{
    // 1.copy transferjson to temp
    QFile jsonfile(filepath);
    QFileInfo info(jsonfile);
    QString tempPath(TransferUtil::tempCacheDir() + connectIP + "transfer-temp.json");
    QFile tempfile(tempPath);
    if (tempfile.exists())
        tempfile.remove();
    if (!jsonfile.copy(tempPath))
        WLOG << "Failed to copy recordTranfer file" + tempPath.toStdString();

    connect(this, &TransferHelper::interruption, this, [this, filepath, tempPath]() {
        // 2.write unfinished files to tempjson file
        QJsonObject jsonObj = SettingHelper::ParseJson(filepath);
        QFile jsonfile(filepath);
        QString fileDir = filepath.left(filepath.lastIndexOf('/'));
        if (!jsonfile.exists() || jsonObj.isEmpty()) {
            WLOG << "Failed to recordTranfer file";
            return;
        }
        QJsonArray userFileArray = jsonObj["user_file"].toArray();
        QJsonArray updatedFileList;
        bool ok;
        int64_t userData = jsonObj["user_data"].toString().toLongLong(&ok);

        LOG << "finshedFiles-----" << finshedFiles.size();

        foreach (const QJsonValue &fileValue, userFileArray) {
            QString file = fileValue.toString();
            QString filename = file.mid(file.indexOf('/'));

            // skip finished files
            bool isend = false;
            for (QString key : finshedFiles.keys()) {
                if (key.endsWith(filename)) {
                    isend = true;
                    if (ok)
                        userData -= finshedFiles.value(key);
                    finshedFiles.remove(key);
                    break;
                }
            }
            if (isend) {
                //Move completed files first
                QString targetFile = QDir::homePath() + "/" + file;
                QString originfile = fileDir + filename;
                QFileInfo info = QFileInfo(targetFile);
                auto dir = info.dir();
                if (!dir.exists())
                    dir.mkpath(".");
                SettingHelper::instance()->moveFile(originfile, targetFile);
                continue;
            }

            updatedFileList.append(file);
        }
        // 3.save unfinished filelist for retransmission
        jsonObj["user_file"] = updatedFileList;
        if (ok)
            jsonObj["user_data"] = QString::number(userData);
        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObj);
        QFile tempfile(tempPath);
        if (!tempfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            WLOG << "Failed to open JSON file for writing";
        tempfile.write(jsonDoc.toJson());
        tempfile.close();
        // remove transfer dir
        QDir dir(fileDir);
        if (!dir.removeRecursively()) {
            WLOG << "Failed to remove directory";
        }
    });

    connect(this, &TransferHelper::transferFinished, this, [this, tempPath] {
        finshedFiles.clear();
        QFile tempfile(tempPath);
        tempfile.remove(tempPath);
    });
}

void TransferHelper::addFinshedFiles(const QString &filepath, int64_t size)
{
    finshedFiles.insert(filepath, size);
    if (filepath.endsWith("transfer.json"))
        TransferHelper::instance()->recordTranferJob(filepath);
}

void TransferHelper::setConnectIP(const QString &ip)
{
    connectIP = ip;
}

void TransferHelper::setting(const QString &filepath)
{
    isSetting = true;
    SettingHelper::instance()->handleDataConfiguration(filepath);
}
