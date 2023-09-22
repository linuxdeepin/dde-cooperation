#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include <QMap>
#include <QObject>

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

Q_SIGNALS:
    void connectSucceed();
    void transferSucceed();
};

#endif
