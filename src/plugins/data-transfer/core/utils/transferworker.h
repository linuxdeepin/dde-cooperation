#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <co/json.h>
#include <co/co.h>


class TransferHandle : public QObject
{
    Q_OBJECT
    struct file_stats_s
    {
        int64_t all_total_size;   // 总量
        int64_t all_current_size;   // 当前已接收量
        int64_t cast_time_ms;   // 最大已用时间
    };

public:
    TransferHandle();
    ~TransferHandle();

    void tryConnect(QString ip, QString password);
    QString getConnectPassWord();
    void sendFiles(QStringList paths);
    void sendMessage(json::Json &message);

    bool cancelTransferJob();
    bool isTransferring();
    void disconnectRemote();

public slots:
    void handleConnectStatus(int result, QString msg);
    void handleTransJobStatus(int id, int result, QString path);
    void handleFileTransStatus(QString statusstr);
    void handleMiscMessage(QString jsonmsg);

    void backendMessageSlot(int type, const QString& msg);

private:

    bool _backendOK = false;
    // <jobid, jobpath>
    QMap<int, QString> _job_maps;
    int _request_job_id;

    //record transfering files ans calculate the progress rate
    file_stats_s _file_stats;
};

class CuteIPCInterface;
class TransferWoker
{

public:
    ~TransferWoker();

    CuteIPCInterface *ipc();

    bool pingBackend(const std::string &who);
    bool cancelTransferJob(int jobid);
    void setEmptyPassWord();
    QString getConnectPassWord();
    void sendFiles(int reqid, QStringList filepaths);
    void sendMessage(json::Json &message);
    void tryConnect(const std::string &ip, const std::string &password);
    void setSessionId(QString &seesionid);
    void disconnectRemote();

    static TransferWoker *instance()
    {
        static TransferWoker ins;
        return &ins;
    }

private:
    TransferWoker();

    QString _session_id = "";
    CuteIPCInterface *ipcInterface { nullptr };
};

#endif
