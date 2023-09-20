#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include <QMap>
#include <QObject>

#include <QUrl>

class TransferHelper : public QObject
{
//    Q_OBJECT

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

Q_SIGNALS:
    void connectSucceed();
    void transferSucceed();

#ifdef WIN32
    void windowsZipFile(const QList<QUrl> &sourceFilePath, QUrl &zipFileSavePath = QUrl());
    void windowsUnZipFile(const QUrl &zipFilePath, QUrl &unZipFilePath = QUrl());
#endif
};

#endif
