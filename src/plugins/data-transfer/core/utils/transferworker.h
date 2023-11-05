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
    struct file_stats_s
    {
        int64_t all_total_size;   // 总量
        int64_t all_current_size;   // 当前已接收量
        int32_t max_time_sec;   // 最大已用时间
    };

public:
    TransferHandle();
    ~TransferHandle();

    void tryConnect(QString ip, QString password);
    QString getConnectPassWord();
    void sendFiles(QStringList paths);

    static void logHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    bool cancelTransferJob();
public slots:
    void saveSession(fastring sessionid);
    void handleConnectStatus(int result, QString msg);
    void handleTransJobStatus(int id, int result, QString path);
    void handleFileTransStatus(QString statusstr);
    void handleMiscMessage(QString jsonmsg);

private:
    void localIPCStart();

    FrontendService *_frontendIpcService = nullptr;
    bool _backendOK = false;
    fastring _sessionid = "";
    // <jobid, jobpath>
    QMap<int, QString> _job_maps;
    int _request_job_id;

    //record transfering files ans calculate the progress rate
    file_stats_s _file_stats;
    // <file_id, last_current_size> 统计正在传输的文件量<文件id，上次已传输量>
    QMap<int, int64_t> _file_ids;

    bool _this_destruct = false;
};

class TransferWoker
{

public:
    ~TransferWoker();

    bool pingBackend(const std::string &who);
    bool cancelTransferJob(int jobid);
    void setEmptyPassWord();
    QString getConnectPassWord();
    void sendFiles(int reqid, QStringList filepaths);
    void tryConnect(const std::string &ip, const std::string &password);
    fastring getSessionId();

    void call(const json::Json& req, json::Json& res);

    static TransferWoker *instance()
    {
        static TransferWoker ins;
        return &ins;
    }

private:
    TransferWoker();

    std::shared_ptr<rpc::Client> coClient { nullptr };
    fastring _session_id = "";
};

#endif
