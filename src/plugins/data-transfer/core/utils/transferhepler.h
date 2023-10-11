#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include "transferworker.h"

#include <QMap>
#include <QObject>

#include <QUrl>

class TransferHelper : public QObject
{
    Q_OBJECT

public:
    TransferHelper();
    ~TransferHelper();

    static TransferHelper *instance();

    const QStringList getUesr();
    QString getConnectPassword();
    QMap<QString, double> getUserDataSize();
    qint64 getRemainStorage();
    QMap<QString, QString> getAppList();
    QMap<QString, QString> getBrowserList();

    void tryConnect(const QString &ip, const QString &password);

    void startTransfer();

    void getJsonfile(const QJsonObject &jsonData, const QString &save = QString());

#ifdef WIN32
public:
    void windowsZipFile(const QList<QUrl> &sourceFilePath, QUrl &zipFileSavePath = QUrl());
    void windowsUnZipFile(const QUrl &zipFilePath, QUrl &unZipFilePath = QUrl());
#else
public:
    bool setWallpaper(const QString &filepath);
    bool setBrowserBookMark(const QString &filepath);
#endif

Q_SIGNALS:
    void connectSucceed();
    void transferring();
    void transferSucceed();

    //Used to control the current operation content, progress, and estimated completion time
    //during transmission or decompression process
    //progressbar use percentage and the time unit is seconds
    void transferContent(const QString &content, int progressbar, int estimatedtime);

private:
    TransferHandle transferhandle;
};

#endif
