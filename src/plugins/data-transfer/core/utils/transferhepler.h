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

    void tryConnect(const QString &ip, const QString &password);

    void startTransfer();

#ifdef WIN32
    QMap<QString, QString> getAppList();
    QMap<QString, QString> getBrowserList();

#else
public:
    bool handleDataConfiguration(const QString &filepath);
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
