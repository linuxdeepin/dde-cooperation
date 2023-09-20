#ifndef TRANSFEHELPER_H
#define TRANSFEHELPER_H

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
    int getConnectPassword();
    QMap<QString, double> getUserDataSize();
    qint64 getRemainStorage();
    QMap<QString, QString> getAppList();

    void startTransfer();
#ifdef WIN32
    void zipFile(const QUrl& sourceFilePath,QUrl& zipFileSavePath = QUrl());
    void unZipFile(const QUrl& zipFilePath, QUrl& unZipFilePath = QUrl());
#endif
};

#endif
