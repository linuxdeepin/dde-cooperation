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

    void getJsonfile(const QJsonObject &jsonData,const QString &save = QString());
#ifdef WIN32
public:
    void windowsZipFile(const QStringList &sourceFilePath,const QString &zipFileSave =QString() );
    void windowsUnZipFile(const QString &zipFilePath,const QString &unZipFile = QString());

    void getUserDataPackagingFile();
#endif

Q_SIGNALS:
    void connectSucceed();
    void transferring();
    void transferSucceed();

private:
    TransferHandle transferhandle;
};

#endif
