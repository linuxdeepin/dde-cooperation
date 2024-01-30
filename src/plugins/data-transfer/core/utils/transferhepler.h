#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include "transferworker.h"

#include <QMap>
#include <QObject>
#include "../gui/type_defines.h"
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
    bool getConnectStatus() const;

    void tryConnect(const QString &ip, const QString &password);

    void disconnectRemote();

    QString getJsonfile(const QJsonObject &jsonData, const QString &save);

    bool cancelTransferJob();

    void emitDisconnected();

    void sendMessage(const QString &type, const QString &message);
#ifdef WIN32
    QMap<QString, QString> getAppList(QMap<QString, QString> &noRecommedApplist);
    QMap<QString, QString> getBrowserList();
    QStringList getTransferFilePath(QStringList filePathList, QStringList appList,
                                    QStringList browserList, QStringList configList);
    void startTransfer();
    QString getTransferJson(QStringList appList, QStringList fileList, QStringList browserList,
                            QString bookmarksName, QString wallPaperName, QString tempSavePath);
    void Retransfer(const QString jsonstr);
    QString defaultBackupFileName();
#else
public:
    static int getRemainSize();
    bool checkSize(const QString &filepath);
    void setting(const QString &filepath);
    void recordTranferJob(const QString &filepath);
    bool isUnfinishedJob(QString &content);
    void addFinshedFiles(const QString &filepath, int64 size);
    void setConnectIP(const QString &ip);

private:
    QMap<QString, int64> finshedFiles;
    bool isSetting = false;
    QString connectIP;
#endif

Q_SIGNALS:
    // transfer state
    void connectSucceed();
    void connectFailed();
    void transferring();

    // isall mean Have all files been transferred and configured successfully
    void transferFinished();

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
    void addResult(QString name, bool success, QString reason);

    // Transmission interruption
    void interruption();

    // unfinish json content from latest job
    void unfinishedJob(const QString jsonstr);

    // clear widget content for next transfer
    void clearWidget();

    // change widget text by select
    void changeWidgetText();

    //change wideget
    void changeWidget(PageName index);

    //disconnect tcp
    void disconnected();

    //remote remainspace
    void remoteRemainSpace(int size);

private:
    void initOnlineState();
    QString tempCacheDir();

private:
    TransferHandle transferhandle;
    bool online = true;
};

#endif
