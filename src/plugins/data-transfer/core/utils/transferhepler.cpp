#include "optionsmanager.h"
#include "transferhepler.h"
#include "drapwindowsdata.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QDir>
#include <QStorageInfo>
#include <QCoreApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextCodec>

#ifdef WIN32
#    include <QProcess>
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

QMap<QString, QString> TransferHelper::getAppList()
{
    QMap<QString, QString> appList;
    appList["企业微信"] = "com.qq.weixin.work.deepin";
    appList["微信"] = "com.qq.weixin.deepin";
    return appList;
}

QMap<QString, QString> TransferHelper::getBrowserList()
{
    QMap<QString, QString> browserList;
    browserList["火狐"] = "com.FireFox.deepin";
    browserList["Goole"] = "com.Goole.deepin";
    return browserList;
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

void TransferHelper::getJsonfile(const QJsonObject &jsonData, const QString &save)
{
    QString savePath = save;
    // 创建一个 JSON 文档
    QJsonDocument jsonDoc(jsonData);

    if (savePath.isEmpty()) {
        savePath = QString("./transfer.json");
    } else {
        savePath += "/transfer.json";
    }
    // 打开文件以保存 JSON 数据
    QFile file(savePath);
    if (file.open(QIODevice::WriteOnly)) {
        // 将 JSON 数据写入文件
        file.write(jsonDoc.toJson());
        file.close();
        qDebug() << "JSON data exported to transfer.json";
    } else {
        qDebug() << "Failed to open file for writing.";
    }
}

#ifdef WIN32
void TransferHelper::windowsZipFile(const QStringList &sourceFilePath, const QString &zipFileSave)
{
    QString zipFileSavePath = zipFileSave;
    QString destination;
    if (zipFileSavePath.isEmpty()) {
        destination = QString("./uos.zip");
    } else {
        destination = zipFileSavePath;
        destination += "/uos.zip";
    }
    QList<QString> filePaths;
    for (auto sourcefile : sourceFilePath) {
        filePaths.push_back(sourcefile);
    }
    QProcess process;

    QString command = "powershell";
    QStringList arguments;
    arguments << "-Command";
    QString cmd = "Compress-Archive -Path ";
    for (const QString &path : filePaths) {
        cmd.append(QString("'%1',").arg(path));
    }
    cmd.chop(1);
    cmd.append(QString(" -DestinationPath '%1'").arg(destination));

    qInfo() << cmd;
    arguments << cmd;
    //    process.setReadBufferSize(65536);
    //    process.setProcessChannelMode(QProcess::MergedChannels);
    //    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(command, arguments);
    process.waitForFinished(-1);

    if (!process.readAllStandardOutput().isEmpty()) {
        qInfo() << process.readAllStandardOutput();
    }

    if (!process.readAllStandardError().isEmpty()) {
        qInfo() << process.readAllStandardError();
    }
}

void TransferHelper::windowsUnZipFile(const QString &zipFilePath, const QString &unZipFile)
{
    QString unZipFilePath;
    QString destinationDir;
    if (unZipFilePath.isEmpty()) {
        destinationDir = QString("./ousUnzip/");
    } else {
        destinationDir = unZipFilePath;
    }
    QProcess process;

    QString archivePath = zipFilePath;
    QString command = "powershell.exe";
    QStringList arguments;
    arguments << "-Command";

    QString cmd = QString("Expand-Archive -Path '%1' -DestinationPath '%2'")
                          .arg(archivePath, destinationDir);

    arguments << cmd;

    process.start(command, arguments);
    process.waitForFinished(-1);

    if (!process.readAllStandardOutput().isEmpty()) {
        qDebug() << process.readAllStandardOutput();
    }
    if (!process.readAllStandardError().isEmpty()) {
        qDebug() << process.readAllStandardError();
    }
}

void TransferHelper::getUserDataPackagingFile()
{
    QStringList filePathList;   //= OptionsManager::instance()->getUserOption(Options::kFile);
    QStringList appList;   //= OptionsManager::instance()->getUserOption(Options::kApp);
    QStringList
            browserList;   // OptionsManager::instance()->getUserOption(Options::kBrowserBookmarks);
    QStringList configList = OptionsManager::instance()->getUserOption(Options::kConfig);

    browserList.append(BrowerName::MicrosoftEdge);
    browserList.append(BrowerName::MozillaFirefox);
    filePathList.append(QString("C:/Users/deep/Documents/test01/test/1"));
    filePathList.append(QString("C:/Users/deep/Documents/test01/test/cdemo"));
    filePathList.append(QString("C:/Users/deep/Documents/test01/libarchive-master"));
    appList.append(QString("WeChat"));
    appList.append(QString("QQ"));

    QStringList zipFilePathList;
    for (auto file : filePathList) {
        zipFilePathList.append(file);
    }

    QString bookmark("C:/Users/deep/Documents/test01/bookmarks.html");
    if (!browserList.isEmpty()) {
        QSet<QString> browserName(browserList.begin(), browserList.end());
        DrapWindowsData::instance()->getBrowserBookmarkInfo(browserName);
        //      DrapWindowsData::instance()->getBrowserBookmarkHtml(QString("C:/Users/deep/Documents/test01"));
        DrapWindowsData::instance()->getBrowserBookmarkJSON(QString("C:/Users/deep/Documents/test01"));
        zipFilePathList.append(bookmark);
    }

    QString wallparerPath;
    // if (!configList.isEmpty()) {
    wallparerPath = DrapWindowsData::instance()->getDesktopWallpaperPath();
    zipFilePathList.append(wallparerPath);
    // }

    QJsonArray appArray;
    for (auto app : appList) {
        appArray.append(app);
    }
    QJsonArray fileArray;
    //    for(auto file :filePathList)
    //    {

    //    }
    fileArray.append("Documents/test01/test/1");
    fileArray.append("Documents/test01/test/cdemo");
    fileArray.append("Documents/test01/libarchive-master");
    // 创建一个 JSON 对象并添加数据
    QJsonObject jsonObject;
    jsonObject["user_data"] = "100";
    jsonObject["user_file"] = fileArray;
    jsonObject["app"] = appArray;
    jsonObject["wallpapers"] = "img0.jpg";
    jsonObject["borwerbookmark"] = "bookmarks.html";
    QString qjsonPath("C:/Users/deep/Documents/test01/transfer.json");
    getJsonfile(jsonObject, QString("C:/Users/deep/Documents/test01"));
    zipFilePathList.append(qjsonPath);

    windowsZipFile(zipFilePathList, QString("C:/Users/deep/Documents"));
}
#else
bool TransferHelper::setWallpaper(const QString &filepath)
{
    qInfo() << OptionsManager::instance()->getUserOptions();
    QStringList paths = OptionsManager::instance()->getUserOptions()["file"];
    transferhandle.sendFiles(paths);
    return true;
}
#endif
