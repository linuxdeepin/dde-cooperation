#include "optionsmanager.h"
#include "transferhepler.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QDir>
#include <QStorageInfo>

TransferHelper::TransferHelper()
    : QObject()
{
}

TransferHelper::~TransferHelper()
{
}

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

int TransferHelper::getConnectPassword()
{
    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

    // 生成随机的六位数字
    int randomNumber = QRandomGenerator::global()->bounded(100000, 999999);

    qDebug() << randomNumber;
    return randomNumber;
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

void TransferHelper::startTransfer()
{
    qInfo() << OptionsManager::instance()->getUserOptions();
}
