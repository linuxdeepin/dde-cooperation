#include "optionsmanager.h"

#include "transferhepler.h"
#include "common/commonutils.h"

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
#include <QStandardPaths>
#include <QNetworkInterface>

#ifdef WIN32
#    include <gui/win/drapwindowsdata.h>
#else
#    include "settinghepler.h"
#endif

//#pragma execution_character_set("utf-8")
TransferHelper::TransferHelper()
    : QObject()
{
    initOnlineState();
#ifndef WIN32
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

void TransferHelper::initOnlineState()
{
    //初始化网络监控
    QTimer *timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, [this]() {
        // 网络状态检测
        bool isConnected = deepin_cross::CommonUitls::getFirstIp().size() > 0;
        if (isConnected != online) {
            LOG << "Network is" << isConnected;
            online = isConnected;
            Q_EMIT onlineStateChanged(isConnected);
        }
    });

    timer->start(1000);
}

QString TransferHelper::tempCacheDir()
{
    QString savePath =
            QString("%1/%2/%3/")
                    .arg(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation))
                    .arg(qApp->organizationName())
                    .arg(qApp->applicationName());   //~/.cache/deepin/xx

    QDir cacheDir(savePath);
    if (!cacheDir.exists())
        QDir().mkpath(savePath);

    return savePath;
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

void TransferHelper::disconnectRemote()
{
    transferhandle.disconnectRemote();
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
        DLOG << "JSON data exported to transfer.json";
        return savePath;
    } else {
        DLOG << "Failed to open file for writing.";
        return QString();
    }
}

void TransferHelper::emitDisconnected()
{
#ifdef linux
    if (!isSetting)
#endif
        emit disconnected();
}

void TransferHelper::sendMessage(const QString &type, const QString &message)
{
    json::Json mes;
    mes.add_member(type.toUtf8().constData(), message.toStdString());
    transferhandle.sendMessage(mes);
}

#ifdef WIN32
void TransferHelper::startTransfer()
{
    QStringList filePathList = OptionsManager::instance()->getUserOption(Options::kFile);
    QStringList appList = OptionsManager::instance()->getUserOption(Options::kApp);
    QStringList browserList = OptionsManager::instance()->getUserOption(Options::kBrowserBookmarks);
    QStringList configList = OptionsManager::instance()->getUserOption(Options::kConfig);

    QStringList paths = getTransferFilePath(filePathList, appList, browserList, configList);
    qInfo() << "transferring file list: " << paths;
    transferhandle.sendFiles(paths);
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
QString TransferHelper::getTransferJson(QStringList appList, QStringList fileList,
                                        QStringList browserList, QString bookmarksName,
                                        QString wallPaperName, QString tempSavePath)
{
    // add app
    QJsonArray appArray;
    for (auto app : appList) {
        appArray.append(app);
    }
    // add file
    QJsonArray fileArray;
    LOG << "home_path:" << QDir::homePath().toStdString();
    for (QString file : fileList) {
        if (file.contains(QDir::homePath()))
            file.replace(QDir::homePath() + "/", "");
        fileArray.append(file);
    }
    // add browser
    QJsonArray browserArray;
    for (QString browser : browserList) {
        browserArray.append(browser);
    }

    QJsonObject jsonObject;
    QString userData = OptionsManager::instance()->getUserOption(Options::KSelectFileSize)[0];
    jsonObject["user_data"] = userData;
    jsonObject["user_file"] = fileArray;
    if (!appArray.isEmpty())
        jsonObject["app"] = appArray;
    if (!wallPaperName.isEmpty())
        jsonObject["wallpapers"] = wallPaperName;
    if (!bookmarksName.isEmpty())
        jsonObject["browserbookmark"] = bookmarksName;
    if (!browserList.isEmpty())
        jsonObject["browsersName"] = browserArray;

    QString jsonfilePath = getJsonfile(jsonObject, QString(tempSavePath));
    LOG << "transfer.json save path:" << jsonfilePath.toStdString();
    return jsonfilePath;
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
    transferhandle.sendFiles(paths);

    emit changeWidget(PageName::transferringwidget);
}

QString TransferHelper::defaultBackupFileName()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDateTime = currentDateTime.toString("yyyyMMddhhmm");

    return QString(DrapWindowsData::instance()->getUserName() + "_"
                   + DrapWindowsData::instance()->getIP() + "_" + formattedDateTime);
}

#else

bool TransferHelper::checkSize(const QString &filepath)
{
    QJsonObject jsonObj = SettingHelper::ParseJson(filepath);
    if (jsonObj.isEmpty())
        return false;
    auto sizestr = jsonObj["user_data"].toString();
    auto size = static_cast<int>(QVariant(sizestr).toLongLong() / 1024 / 1024 / 1024) * 2;
    LOG << "The actual size is " << sizestr.toStdString() << "B "
        << "Two times the space needs to be reserved" << size << "G";
    int remainSize = getRemainSize();
    if (size >= remainSize) {
        LOG << "outOfStorage" << size;
        emit outOfStorage(size);
        cancelTransferJob();
        disconnectRemote();
        return false;
    }
    return true;
}

void TransferHelper::recordTranferJob(const QString &filepath)
{
    // 1.copy transferjson to temp
    QFile jsonfile(filepath);
    QFileInfo info(jsonfile);
    QString tempPath(tempCacheDir() + connectIP + "transfer-temp.json");
    if (!jsonfile.copy(tempPath))
        WLOG << "Failed to copy recordTranfer file" + tempPath.toStdString();

    connect(this, &TransferHelper::interruption, this, [this, filepath, tempPath]() {
        // 2.write unfinished files to tempjson file
        QJsonObject jsonObj = SettingHelper::ParseJson(filepath);
        QString fileDir = filepath.left(filepath.lastIndexOf('/'));
        QJsonArray userFileArray = jsonObj["user_file"].toArray();
        QJsonArray updatedFileList;
        bool ok;
        int64 userData = jsonObj["user_data"].toString().toLongLong(&ok);

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

bool TransferHelper::isUnfinishedJob(QString &content)
{
    QString transtempPath(tempCacheDir() + connectIP + "transfer-temp.json");
    QFile f(transtempPath);
    if (!f.exists())
        return false;
    LOG << "has UnfinishedJob: " << transtempPath.toStdString();
    if (!f.open(QIODevice::ReadOnly)) {
        WLOG << "could not open file";
        return false;
    }
    QByteArray bytes = f.readAll();
    content = QString(bytes.data());
    f.close();

    return true;
}

void TransferHelper::addFinshedFiles(const QString &filepath, int64 size)
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

int TransferHelper::getRemainSize()
{
    QStorageInfo storage("/home");
    auto remainSize = storage.bytesAvailable() / 1024 / 1024 / 1024;
    return static_cast<int>(remainSize);
}
#endif
