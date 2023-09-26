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

    void startTransfer();

Q_SIGNALS:
    void connectSucceed();
    void transferSucceed();

#ifdef WIN32
protected: // 之前没加这行，它被识别为信号，会在 moc 文件中创建它的定义，我不清楚这里是 public 还是啥，用到了自己修改
    void windowsZipFile(const QList<QUrl> &sourceFilePath, QUrl &zipFileSavePath = QUrl());
    void windowsUnZipFile(const QUrl &zipFilePath, QUrl &unZipFilePath = QUrl());
#endif

private:
    TransferHandle transferhandle;
};

#endif
