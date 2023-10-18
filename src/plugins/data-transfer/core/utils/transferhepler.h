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

    QString getConnectPassword();
    bool getOnlineState() const;

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
    bool setFile(QJsonObject jsonObj, QString filepath);
#endif

Q_SIGNALS:
    // transfer state
    void connectSucceed();
    void transferring();
    void transferSucceed();

    //Used to control the current operation content, progress, and estimated completion time
    //during transmission or decompression process
    //progressbar use percentage and the time unit is seconds
    void transferContent(const QString &content, int progressbar, int estimatedtime);

    //network
    void onlineStateChanged(bool online);

private:
    void initOnlineState();

private:
    TransferHandle transferhandle;
    bool online = true;
};

#endif
