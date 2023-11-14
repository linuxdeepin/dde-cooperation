#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>

#include "zipworker.h"
#include "../win/drapwindowsdata.h"

#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QTextCodec>
#include <QDir>
#include <QElapsedTimer>
#include <QDirIterator>
#include <QCoreApplication>
#include <JlCompress.h>

#pragma execution_character_set("utf-8")
ZipWork::ZipWork(QObject *parent) : QThread(parent)
{
    qInfo() << "zipwork start.";
    // connect backup file process
    QObject::connect(this, &ZipWork::backupFileProcessSingal, TransferHelper::instance(),
                     &TransferHelper::zipTransferContent);
    // connect the main thread exit signal
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this,
                     &ZipWork::abortingBackupFileProcess);
}

ZipWork::~ZipWork() { }

void ZipWork::run()
{
    getUserDataPackagingFile();
    qInfo() << "backup11 file exit.";
}

void ZipWork::getUserDataPackagingFile()
{
    QStringList zipFilePathList = TransferHelper::instance()->getTransferFilePath();

    // Get the number of files to zip
    allFileNum = getAllFileNum(zipFilePathList);
    backupFile(zipFilePathList, getBackupFilName());
}

int ZipWork::getPathFileNum(const QString &filePath)
{
    if (QFileInfo(filePath).isFile())
        return 1;
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

bool ZipWork::addFileToZip(const QString &filePath, const QString &relativeTo, QuaZip &zip,
                           QElapsedTimer &timer)
{
    if (abort) {
        zip.close();
        QFile::remove(zipFile);
        return false;
    }
    QFile sourceFile(filePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        qCritical() << "Error reading source file:" << filePath;
        // backup file false
        emit backupFileProcessSingal(QString("压缩源文件有误:%1").arg(filePath), -1, -1);
        return false;
    }

    QuaZipFile destinationFile(&zip);
    QString destinationFileName = QDir(relativeTo).relativeFilePath(filePath);

    QuaZipNewInfo newInfo(destinationFileName, sourceFile.fileName());
    if (!destinationFile.open(QIODevice::WriteOnly, newInfo)) {
        qCritical() << "Error writing to ZIP file for:" << filePath;
        // backup file false
        emit backupFileProcessSingal(QString("压缩文件写入错误:%1").arg(filePath), -1, -1);
        return false;
    }

    destinationFile.write(sourceFile.readAll());
    destinationFile.close();
    sourceFile.close();
    sendBackupFileProcess(filePath, timer);
    return true;
}

bool ZipWork::addFolderToZip(const QString &sourceFolder, const QString &relativeTo, QuaZip &zip,
                             QElapsedTimer &timer)
{
    QDir directory(sourceFolder);
    QFileInfoList entries = directory.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (QFileInfo entry : entries) {
        if (entry.isDir()) {
            if (!addFolderToZip(entry.absoluteFilePath(), relativeTo, zip, timer)) {
                return false;
            }
        } else {
            if (!addFileToZip(entry.absoluteFilePath(), relativeTo, zip, timer)) {
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
    zipFile = destinationZipFile;
    QuaZip zip(destinationZipFile);
    QElapsedTimer timer;
    zip.setFileNameCodec("UTF-8");
    if (!zip.open(QuaZip::mdCreate)) {
        qCritical("Error creating the ZIP file.");
        // backup file false
        emit backupFileProcessSingal(QString("创建压缩文件失败,检查文件%1是否已经被打开！").arg(destinationZipFile), -1, -1);
        return false;
    }

    for (QString entry : entries) {

        QFileInfo fileInfo(entry);
        if (fileInfo.isDir()) {
            QDir parent = QDir(entry);
            parent.cdUp();
            if (!addFolderToZip(entry, QDir(parent).absolutePath(), zip, timer)) {
                return false;
            }
        } else if (fileInfo.isFile()) {
            if (!addFileToZip(entry, fileInfo.absolutePath(), zip, timer)) {
                return false;
            }
        }
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK) {
        qCritical() << "Error while compressing. Error code:" << zip.getZipError();
        // backup file false
        emit backupFileProcessSingal(QString("文件压缩失败,错误代码：%1").arg(zip.getZipError()),
                                     -1, -1);
        return false;
    }

    // backup file true
    emit backupFileProcessSingal(QString("文件完成！"), 100, 0);
    return true;
}

void ZipWork::sendBackupFileProcess(const QString &filePath, QElapsedTimer &timer)
{
    zipFileNum++;
    int progress = (zipFileNum * 100) / allFileNum;

    if (progress <= 0)
        progress = 0;
    if (progress >= 100)
        progress = 98;

    if (!timer.isValid()) {
        timer.start();
    } else {
        // If the timer is started, the elapsed time is calculated and the timer is restarted
        if (num == maxNum) {
            qint64 elapsed = timer.restart();
            needTime = static_cast<int>(elapsed) / maxNum * (allFileNum - zipFileNum) / 1000;
        }
    }
    if (needTime < 5)
        needTime = 5;
    if (needTime > 3600)
        needTime = 3600;
    num++;
    if (num > maxNum)
        num = 0;
    emit backupFileProcessSingal(filePath, progress, needTime);
}

QString ZipWork::getBackupFilName()
{
    QStringList zipFileSavePath = OptionsManager::instance()->getUserOption(Options::kBackupFileSavePath);
    QStringList zipFileNameList = OptionsManager::instance()->getUserOption(Options::kBackupFileName);

    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString formattedDateTime = currentDateTime.toString("yyyyMMddhhmm");

    QString zipFileName;

    if (zipFileNameList[0] == "") {
        zipFileName = zipFileSavePath[0] + "/" + DrapWindowsData::instance()->getUserName() + "_"
                + DrapWindowsData::instance()->getIP() +"_" +formattedDateTime+".zip";
    } else {
        zipFileName = zipFileSavePath[0] + "/" + zipFileNameList[0] +".zip";
    }
    qInfo() << "backup file save path:" << zipFileName;

    QFile  file(zipFileName);
    if (file.exists() && file.remove()) {
        qDebug() <<zipFileName <<" exists, and removed!" ;
    } else {
        qDebug() << zipFileName <<" exists, and can not removed!" ;
    }

    return zipFileName;
}

void ZipWork::abortingBackupFileProcess()
{
    abort = true;
    //  quit();
    // wait();
    qInfo() << "backup file exit.";
}
