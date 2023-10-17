#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <co/rpc.h>
#include <co/co.h>

class FrontendService;
class TransferHandle : public QObject
{
    Q_OBJECT
    struct file_stats_s {
        int64_t all_total_size; // 总量
        int64_t all_current_size; // 当前已接收量
        int32_t max_time_sec;  // 最大已用时间
    };

public:
    TransferHandle();
    ~TransferHandle();

    void tryConnect(QString ip, QString password);
    QString getConnectPassWord();
    void sendFiles(QStringList paths);

public slots:
    void saveSession(QString sessionid);
    void handleConnectStatus(int result, QString msg);
    void handleTransJobStatus(int id, int result, QString path);
    void handleFileTransStatus(QString statusstr);

private:
    FrontendService *_frontendIpcService = nullptr;
    bool _backendOK = false;
    QString _sessionid = "";
    // <jobid, jobpath>
    QMap<int, QString> _job_maps;
    int _request_job_id;

    //record transfering files ans calculate the progress rate
    file_stats_s _file_stats;
    // <file_id, last_current_size> 统计正在传输的文件量<文件id，上次已传输量>
    QMap<int, int64_t> _file_ids;
};

class TransferWoker
{

public:
    ~TransferWoker();

    bool pingBackend(const std::string &who);
    void setEmptyPassWord();
    QString getConnectPassWord();
    void sendFiles(int reqid, QStringList filepaths);
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
