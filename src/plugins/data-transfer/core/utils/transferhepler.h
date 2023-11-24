#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include "transferworker.h"

#include <QMap>
#include <QObject>

#ifndef WIN32
#    include <QDBusMessage>
#endif

#include <QUrl>

class WinApp;
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

    QString getJsonfile(const QJsonObject &jsonData, const QString &save);

    bool cancelTransferJob();

#ifdef WIN32
    QMap<QString, QString> getAppList(QMap<QString, QString> &noRecommedApplist);
    QMap<QString, QString> getBrowserList();
    QStringList getTransferFilePath();
    void startTransfer();
#else
public:
    int getRemainSize();
    bool checkSize(const QString &filepath);
    void setting(const QString &filepath);
    void recordTranferJob(const QString &filepath);
    bool isUnfinishedJob(QString &content);
    void addFinshedFiles(const QString &filepath);

private:
    QStringList finshedFiles;
#endif

Q_SIGNALS:
    // transfer state
    void connectSucceed();
    void connectFailed();
    void transferring();

    // isall mean Have all files been transferred and configured successfully
    void transferSucceed(bool isall);

    // Used to control the current operation content, progress, and estimated completion time
    // during transmission or decompression process
    // progressbar use percentage and the time unit is seconds
    void transferContent(const QString &tpye, const QString &content, int progressbar,
                         int estimatedtime);

    // zip progressbar use percentage and the time unit is seconds
    void zipTransferContent(const QString &content, int progressbar, int estimatedtime);

    // network
    void onlineStateChanged(bool online);

    // outOfStorage
    void outOfStorage(int size);

    // display config failure
    void failure(QString name, QString type, QString reason);

    // Transmission interruption
    void interruption();

    // unfinish json content from latest job
    void unfinishedJob(const QString jsonstr);

private:
    void initOnlineState();
    QString tempCacheDir();

private:
    TransferHandle transferhandle;
    bool online = true;
};

#endif
