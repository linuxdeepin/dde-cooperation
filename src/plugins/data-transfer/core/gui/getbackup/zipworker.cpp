#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>

#include "zipworker.h"
#include "drapwindowsdata.h"

#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextCodec>
#include <QDir>
#include <QElapsedTimer>

#pragma execution_character_set("utf-8")
ZipWork::ZipWork() { }

ZipWork::~ZipWork() { }

void ZipWork::run()
{
    getUserDataPackagingFile();
}

void ZipWork::zipFile(const QStringList &sourceFilePath, const QString &zipFileSave)
{
    // Check if the zipFileSave exists
    QFile file(zipFileSave);
    if (file.exists()) {
        // delete
        if (file.remove()) {
            qDebug() << "Deleted successfully " << zipFileSave;
        } else {
            // can not zip file
            qDebug() << "Can not deleted " << zipFileSave;
            emit TransferHelper::instance()->transferContent(
                    QString("压缩文件%1已存在,请删除后重试。").arg(zipFileSave), -1, 500);
            return;
        }
    }

    QList<QString> filePaths;
    for (auto sourcefile : sourceFilePath) {
        filePaths.push_back(sourcefile);
    }
    QProcess process;

    QString command = "C:/Windows/system32/WindowsPowerShell/v1.0/powershell.exe";
    QStringList arguments;
    arguments << "-Command";
    QString cmd = "Compress-Archive -Path ";
    for (const QString &path : filePaths) {
        cmd.append(QString("'%1',").arg(path));
    }
    cmd.chop(1);
    cmd.append(
            QString(" -DestinationPath '%1' -CompressionLevel Optimal -Verbose").arg(zipFileSave));

    qInfo() << cmd;
    arguments << cmd;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProcessChannelMode(QProcess::SeparateChannels);

    bool success = true;
    QObject::connect(&process, &QProcess::readyReadStandardOutput, &process, [&process, this]() {
        QString output =
                QTextCodec::codecForName("GB18030")->toUnicode(process.readAllStandardOutput());

        int currentfileNum = output.count("正在添加");

        qInfo() << "Standard Output:" << output;
        QRegExp regex("[\\x4e00-\\x9fa5]+");
        output.replace(regex, "");
        output.remove(':');

        zipFileNum += currentfileNum;
        int processbar = qBound(0, (int)((float)zipFileNum / (float)allFileNum * 100), 99);
        int needSecond = 1;
        if (!timer.isValid()) {
            timer.start();
        } else {
            // If the timer is started, the elapsed time is calculated and the timer is restarted
            qint64 elapsed = timer.restart();
            if (lastZipFileNum == 0) {
                needSecond = 100;
            } else {
                needSecond = (allFileNum - zipFileNum) / lastZipFileNum * elapsed / 1000;
                if (needSecond <= 1)
                    needSecond = 1;
            }
        }
        qInfo() << "all file:" << allFileNum;
        qInfo() << "zipFileNum" << zipFileNum;
        qInfo() << "need time: " << needSecond;
        qInfo() << "processbar:" << processbar;
        qInfo() << "currentfileNum" << currentfileNum;

        emit TransferHelper::instance()->transferContent(output, processbar, needSecond);

        // update lastzipfilenum
        lastZipFileNum = currentfileNum;
    });

    QObject::connect(&process, &QProcess::readyReadStandardError, &process, [&process, &success]() {
        QString output =
                QTextCodec::codecForName("GB18030")->toUnicode(process.readAllStandardError());
        qDebug() << "Standard Output:" << output;
        success = false;
        emit TransferHelper::instance()->transferContent(output, -1, 500);
    });
    process.start(command, arguments);

    process.waitForFinished(-1);
    if (success) {
        emit TransferHelper::instance()->transferContent("压缩完成", 100, 500);
    }
}

void ZipWork::unZipFile(const QString &zipFilePath, const QString &unZipFile)
{
    QString unZipFilePath = unZipFile;
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
void ZipWork::getJsonfile(const QJsonObject &jsonData, const QString &save)
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

void ZipWork::getUserDataPackagingFile()
{
    QStringList filePathList = OptionsManager::instance()->getUserOption(Options::kFile);
    QStringList appList = OptionsManager::instance()->getUserOption(Options::kApp);
    QStringList browserList = OptionsManager::instance()->getUserOption(Options::kBrowserBookmarks);
    QStringList configList = OptionsManager::instance()->getUserOption(Options::kConfig);

    QStringList zipFilePathList;
    for (auto file : filePathList) {
        zipFilePathList.append(file);
    }

    if (!browserList.isEmpty()) {
        QSet<QString> browserName(browserList.begin(), browserList.end());
        DrapWindowsData::instance()->getBrowserBookmarkInfo(browserName);
        DrapWindowsData::instance()->getBrowserBookmarkJSON(QString("."));
        zipFilePathList.append(QString("./bookmarks.json"));
    }

    if (!configList.isEmpty()) {
        QString wallparerPath = DrapWindowsData::instance()->getDesktopWallpaperPath();
        zipFilePathList.append(wallparerPath);
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
    jsonObject["user_data"] = "100";
    jsonObject["user_file"] = fileArray;
    jsonObject["app"] = appArray;
    jsonObject["wallpapers"] = "img0.jpg";
    jsonObject["borwerbookmark"] = "./bookmarks.json";

    QString qjsonPath("./transfer.json");
    getJsonfile(jsonObject, QString("."));
    zipFilePathList.append(qjsonPath);

    {
        QStringList zipFileSavePath =
                OptionsManager::instance()->getUserOption(Options::kBackupFileSavePath);
        QStringList zipFileNameList =
                OptionsManager::instance()->getUserOption(Options::kBackupFileName);

        QString zipFileName;
        if (zipFileNameList[0]=="") {
            zipFileName = zipFileSavePath[0] + "/" + DrapWindowsData::instance()->getUserName()
                    + "_" + DrapWindowsData::instance()->getIP() + "_uos.zip";
        } else {
            zipFileName = zipFileSavePath[0] + "/" + zipFileNameList[0] + ".zip";
        }
        qInfo() << "backup file save path:" << zipFileName;

        // Get the number of files to zip
        allFileNum = getAllFileNum(zipFilePathList);

        zipFile(zipFilePathList, zipFileName);
    }
}

int ZipWork::getPathFileNum(const QString &filePath)
{
    int fileCount = 0;
    QDir dir(filePath);
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);

    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            fileCount += getPathFileNum(entry.absoluteFilePath());
        } else {
            fileCount++;
        }
    }

    return fileCount;
}

int ZipWork::getAllFileNum(const QStringList &fileList)
{
    int fileNum = 0;

    for (const QString &filePath : fileList) {
        int curPathFileNum = getPathFileNum(filePath);
        fileNum += curPathFileNum;
    }
    return fileNum;
}

