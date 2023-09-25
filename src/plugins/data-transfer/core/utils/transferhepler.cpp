#include "optionsmanager.h"
#include "transferhepler.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QDir>
#include <QStorageInfo>
#include <QCoreApplication>

#ifdef WIN32
#    include <QProcess>
#endif

#pragma execution_character_set("utf-8")
TransferHelper::TransferHelper()
    : QObject()
{
}

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

void TransferHelper::tryConnect(const QString &ip, const QString &password)
{
    transferhandle.tryConnect(ip , password);
}

void TransferHelper::startTransfer()
{
    qInfo() << OptionsManager::instance()->getUserOptions();
    QStringList paths = OptionsManager::instance()->getUserOptions()["file"];
    transferhandle.senFiles(paths);
}

#ifdef WIN32
void TransferHelper::windowsZipFile(const QList<QUrl> &sourceFilePath, QUrl &zipFileSavePath)
{
    QString destination;
    if (zipFileSavePath.isEmpty()) {
        destination = QString("./uos.zip");
    } else {
        destination = zipFileSavePath.toLocalFile();
    }
    QList<QString> filePaths;
    for (auto sourcefile : sourceFilePath) {
        filePaths.push_back(sourcefile.toLocalFile());
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

    arguments << cmd;
    process.start(command, arguments);
    process.waitForFinished(-1);

    if (process.readAllStandardOutput() != "") {
        qDebug() << process.readAllStandardOutput();
    }
    if (process.readAllStandardError() != "") {
        qDebug() << process.readAllStandardError();
    }
}

void TransferHelper::windowsUnZipFile(const QUrl &zipFilePath, QUrl &unZipFilePath)
{
    QString destinationDir;
    if (unZipFilePath.isEmpty()) {
        destinationDir = QString("./ousUnzip/");
    }
    QProcess process;

    QString archivePath = zipFilePath.toLocalFile();
    QString command = "powershell";
    QStringList arguments;
    arguments << "-Command";

    QString cmd = QString("Expand-Archive -Path '%1' -DestinationPath '%2'")
                          .arg(archivePath, destinationDir);

    arguments << cmd;

    process.start(command, arguments);
    process.waitForFinished(-1);

    if (process.readAllStandardOutput() != "") {
        qDebug() << process.readAllStandardOutput();
    }
    if (process.readAllStandardError() != "") {
        qDebug() << process.readAllStandardError();
    }
}
#endif
