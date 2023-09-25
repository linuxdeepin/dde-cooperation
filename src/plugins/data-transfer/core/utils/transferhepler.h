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

    void tryConnect(const QString &ip, const QString &password);

    void startTransfer();

Q_SIGNALS:
    void connectSucceed();
    void transferSucceed();

#ifdef WIN32
public:
    void windowsZipFile(const QList<QUrl> &sourceFilePath, QUrl &zipFileSavePath = QUrl());
    void windowsUnZipFile(const QUrl &zipFilePath, QUrl &unZipFilePath = QUrl());
#endif

private:
    TransferHandle transferhandle;
};

#endif
