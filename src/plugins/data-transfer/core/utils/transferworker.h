#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include <QObject>
#include <co/rpc.h>
#include <co/co.h>

class TransferHandle : public QObject
{
    Q_OBJECT

public:
    TransferHandle();
    ~TransferHandle();

    void tryConnect(QString ip, QString password);
    QString getConnectPassWord();
    void senFiles(QStringList paths);

    void pollingStatus();

private:
};

class TransferWoker
{

public:
    TransferWoker() {};
    ~TransferWoker() {};

    static QString getConnectPassWord();
    static void senFiles(QStringList paths);
    static void tryConnect(const std::string &ip, const std::string &password);

    static int getStatus();

private:
};

#endif
