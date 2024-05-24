#ifndef TRANSFERWORKER_H
#define TRANSFERWORKER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include "session/asioservice.h"
#include "session/protoserver.h"
#include "session/protoclient.h"

#include "httpweb/fileserver.h"
#include "httpweb/fileclient.h"

class TransferHandle : public QObject, public SessionCallInterface
{
    Q_OBJECT
    struct file_stats_s {
        int64_t all_total_size;   // 总量
        int64_t all_current_size;   // 当前已接收量
        int64_t cast_time_ms;   // 最大已用时间
    };

public:
    TransferHandle();
    ~TransferHandle();

    void init();

    void onReceivedMessage(const proto::OriginMessage &request, proto::OriginMessage *response) override;

    bool onStateChanged(int state, std::string &msg) override;

    bool tryConnect(QString ip, QString password);
    QString getConnectPassWord();
    void sendFiles(QStringList paths);

    bool cancelTransferJob();
    bool isTransferring();
    void disconnectRemote();

public slots:
    void handleConnectStatus(int result, QString msg);
    void handleTransJobStatus(int id, int result, QString path);
    void handleFileTransStatus(QString statusstr);

Q_SIGNALS:
    void notifyTransData(const std::vector<std::string> nameVector);
    void notifyCancelWeb(const std::string jobid);

private slots:
    bool handleDataDownload(const std::vector<std::string> nameVector);
    void handleWebCancel(const std::string jobid);

private:
    void connectRemote(QString &address);

    bool startFileWeb();

    std::shared_ptr<AsioService> _service { nullptr };
    // rpc service and client
    std::shared_ptr<ProtoServer> _server { nullptr };
    std::shared_ptr<ProtoClient> _client { nullptr };

    // file sync service and client
    std::shared_ptr<FileServer> _file_server { nullptr };
    std::shared_ptr<FileClient> _file_client { nullptr };

    bool _backendOK = false;
    // <jobid, jobpath>
    QMap<int, QString> _job_maps;
    int _request_job_id;

    bool _this_destruct = false;

    int ipcPing = 3;

    QString _pinCode = "123456";
    QString _accessToken = "";
    QString _saveDir = "";
    QString _connectedAddress = "";
};

#endif
