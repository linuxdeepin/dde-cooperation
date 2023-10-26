#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>

#include "zipworker.h"
#include "drapwindowsdata.h"

#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QTextCodec>
#include <QDir>
#include <QElapsedTimer>
#include <QDirIterator>

#include <JlCompress.h>

#pragma execution_character_set("utf-8")
ZipWork::ZipWork()
{
    QObject::connect(this, &ZipWork::backupFileProcessSingal, TransferHelper::instance(),
                     &TransferHelper::transferContent);
}

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

        QRegExp regex("[\\p{Pd}\\x4e00-\\x9fa5]+");
        output.replace(regex, "");
        output.remove(':');
        output.remove('。');
        zipFileNum += currentfileNum;
        int processbar = qBound(0, (int)((float)zipFileNum / (float)allFileNum * 100), 99);
        qint64 needSecond = 1;
        if (!timer.isValid()) {
            timer.start();
        } else {
            // If the timer is started, the elapsed time is calculated and the timer is restarted
            qint64 elapsed = timer.restart();
            if (lastZipFileNum == 0) {
                needSecond = 0;
            } else {
                qint64 elapsedSeconds = elapsed / 1000;
                int elapsedInt = static_cast<int>(elapsedSeconds);
                if (elapsedInt <= 1)
                    elapsedInt = 1;
                needSecond = ((allFileNum - zipFileNum) / lastZipFileNum) * elapsedInt;
                if (needSecond <= 1)
                    needSecond = 1;
            }
        }

        int needMinute = needSecond / 60;
        if (needMinute <= 1)
            needMinute = 1;
        if (needMinute > lastTime) {
            needMinute = lastTime;
        }
        emit TransferHelper::instance()->transferContent(output.right(50), processbar, needMinute);

        // update lastzipfilenum
        lastZipFileNum = currentfileNum;
        // update lasttime
        lastTime = needMinute;
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

void ZipWork::getUserDataPackagingFile()
{
    QStringList zipFilePathList = TransferHelper::instance()->getTransferFilePath();
    QStringList zipFileSavePath =
            OptionsManager::instance()->getUserOption(Options::kBackupFileSavePath);
    QStringList zipFileNameList =
            OptionsManager::instance()->getUserOption(Options::kBackupFileName);

    QString zipFileName;
    if (zipFileNameList[0] == "") {
        zipFileName = zipFileSavePath[0] + "/" + DrapWindowsData::instance()->getUserName() + "_"
                + DrapWindowsData::instance()->getIP() + "_uos.zip";
    } else {
        zipFileName = zipFileSavePath[0] + "/" + zipFileNameList[0] + ".zip";
    }
    qInfo() << "backup file save path:" << zipFileName;

    // Get the number of files to zip
    allFileNum = getAllFileNum(zipFilePathList);

    // zipFile(zipFilePathList, zipFileName);
    backupFile(zipFilePathList, zipFileName);
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

bool ZipWork::addFileToZip(const QString &filePath, const QString &relativeTo, QuaZip &zip)
{
    //    if (QFileInfo(filePath).isHidden())
    //        return true;
    QFile sourceFile(filePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Error reading source file:" << filePath;
        return false;
    }

    QuaZipFile destinationFile(&zip);
    QString destinationFileName = QDir(relativeTo).relativeFilePath(filePath);
    qInfo() << destinationFileName;

    QuaZipNewInfo newInfo(destinationFileName, sourceFile.fileName());
    if (!destinationFile.open(QIODevice::WriteOnly, newInfo)) {
        qCritical() << "Error writing to ZIP file for:" << filePath;
        return false;
    }

    destinationFile.write(sourceFile.readAll());
    destinationFile.close();
    sourceFile.close();

    sendBackupFileProcess(filePath);
    return true;
}

bool ZipWork::addFolderToZip(const QString &sourceFolder, const QString &relativeTo, QuaZip &zip)
{
    QDir directory(sourceFolder);
    QFileInfoList entries = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (QFileInfo entry : entries) {
        if (entry.isDir()) {
            if (!addFolderToZip(entry.absoluteFilePath(), relativeTo, zip)) {
                return false;
            }
        } else {
            if (!addFileToZip(entry.absoluteFilePath(), relativeTo, zip)) {
                return false;
            }
        }
    }

    // If the current folder is empty, then create an empty directory
    if (entries.isEmpty()) {
        QuaZipFile dirZipFile(&zip);
        QString dirFileName = QDir(relativeTo).relativeFilePath(sourceFolder) + "/";
        QuaZipNewInfo newInfo(dirFileName);
        dirZipFile.open(QIODevice::WriteOnly, newInfo);
        dirZipFile.close();
    }

    return true;
}

bool ZipWork::backupFile(const QStringList &entries, const QString &destinationZipFile)
{
    QuaZip zip(destinationZipFile);
    zip.setFileNameCodec("UTF-8");
    if (!zip.open(QuaZip::mdCreate)) {
        qCritical("Error creating the ZIP file.");
        return false;
    }

    for (QString entry : entries) {
        QFileInfo fileInfo(entry);
        if (fileInfo.isDir()) {
            QDir parent =  QDir(entry);
            parent.cdUp();
            if (!addFolderToZip(entry, QDir(parent).absolutePath(), zip)) {
                return false;
            }
        } else if (fileInfo.isFile()) {
            if (!addFileToZip(entry, fileInfo.absolutePath(), zip)) {
                return false;
            }
        }
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK) {
        qCritical() << "Error while compressing. Error code:" << zip.getZipError();
        return false;
    }

    // backup file done
    emit backupFileProcessSingal(QString("压缩完成!"), 100, 0);
    return true;
}

void ZipWork::sendBackupFileProcess(const QString &filePath)
{
    zipFileNum++;
    int progress = (zipFileNum * 100) / allFileNum;

    if (progress <= 0)
        progress = 0;
    if (progress >= 100)
        progress = 98;

    int needtime = 0;
    if (!timer.isValid()) {
        timer.start();
    } else {
        // If the timer is started, the elapsed time is calculated and the timer is restarted
        qint64 elapsed = timer.restart();
        qint64 elapsedSeconds = elapsed * (allFileNum - zipFileNum) / 1000;
        needtime = static_cast<int>(elapsedSeconds);
    }
    if (needtime > lastTime)
        needtime = lastTime;
    if (needtime < 1)
        needtime = 1;
    emit backupFileProcessSingal(filePath, progress, needtime);
}
