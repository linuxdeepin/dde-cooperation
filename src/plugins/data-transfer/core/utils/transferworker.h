#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include <QObject>
#include <co/rpc.h>
#include <co/co.h>

class FrontendService;
class TransferHandle : public QObject
{
    Q_OBJECT

public:
    TransferHandle();
    ~TransferHandle();

    void tryConnect(QString ip, QString password);
    QString getConnectPassWord();
    void sendFiles(QStringList paths);

public slots:
    void saveSession(QString sessionid);
    void handleConnectStatus(int result, QString msg);
    void handleTransJobStatus(int id, int result, QString msg);
    void handleFileTransStatus(QString statusstr);

private:
    FrontendService *_frontendIpcService = nullptr;
    bool _backendOK = false;
    QString _sessionid = "";
};

class TransferWoker
{

public:
    ~TransferWoker();

    bool pingBackend();
    QString getConnectPassWord();
    void sendFiles(QStringList paths);
    void tryConnect(const std::string &ip, const std::string &password);

    static TransferWoker *instance()
    {
        static TransferWoker ins;
        return &ins;
    }

private:
    TransferWoker();

    co::pool *_gPool = nullptr;
    fastring _session_id = "";
};

#endif
