#include "transferhepler.h"
#include "transferworker.h"

#include <co/rpc.h>
#include <co/co.h>

#include <QTimer>

TransferHandle::TransferHandle()
    : QObject()
{
    pollingStatus();
}

TransferHandle::~TransferHandle()
{
}

void TransferHandle::tryConnect(QString ip, QString password)
{
    go([ip, password]() {
        TransferWoker::tryConnect(ip.toStdString(), password.toStdString());
    });
}

QString TransferHandle::getConnectPassWord()
{
    co::wait_group g_wg;
    g_wg.add(1);
    QString password;
    go([&password, g_wg]() {
        password = TransferWoker::getConnectPassWord();
        g_wg.done();
    });
    g_wg.wait();
    return password;
}

void TransferHandle::senFiles(QStringList paths)
{
    go([paths]() {
        TransferWoker::senFiles(paths);
    });
}

void TransferHandle::pollingStatus()
{
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [] {
        go([]() {
            int status = TransferWoker::getStatus();
            switch (status) {
            case 1:
                emit TransferHelper::instance()->connectSucceed();
                break;
            case 2:
                emit TransferHelper::instance()->transferring();
                break;
            case 3:
                emit TransferHelper::instance()->transferSucceed();
                break;
            default:
                break;
            }
        });
    });
    timer->start(1000);
}

QString TransferWoker::getConnectPassWord()
{
    std::unique_ptr<rpc::Client> proto;
    proto.reset(new rpc::Client("127.0.0.1", 7788, false));
    rpc::Client c(*proto);
    co::Json req, res;

    req.add_member("api", "Common.getSettingPassword");

    c.call(req, res);
    c.close();

    return res.get("password").as_string().c_str();
}

void TransferWoker::tryConnect(const std::string &ip, const std::string &password)
{
    std::unique_ptr<rpc::Client> proto;
    proto.reset(new rpc::Client("127.0.0.1", 7788, false));
    rpc::Client c(*proto);
    co::Json req, res;

    req = {
        { "ip", ip },
        { "password", password },
    };
    req.add_member("api", "Common.tryConnect");
    c.call(req, res);

    c.close();
}

void TransferWoker::senFiles(QStringList filepaths)
{
    std::unique_ptr<rpc::Client> proto;
    proto.reset(new rpc::Client("127.0.0.1", 7788, false));
    rpc::Client c(*proto);
    co::Json req, res, paths;

    for (QString path : filepaths) {
        paths.push_back(path.toStdString());
    }

    req.add_member("filepaths", paths);
    req.add_member("api", "FS.sendFiles");

    c.call(req, res);
    c.close();
}

int TransferWoker::getStatus()
{
    std::unique_ptr<rpc::Client> proto;
    proto.reset(new rpc::Client("127.0.0.1", 7788, false));
    rpc::Client c(*proto);
    co::Json req, res;

    req.add_member("api", "Common.syncConfig");

    c.call(req, res);
    c.close();

    QString x = res.str().c_str();
    bool connnect = res.get("connected").as_bool();
    bool transfer = res.get("tranfer").as_bool();
    bool result = res.get("result").as_bool();
    if (connnect & transfer & result)
        return 3;
    if (connnect & transfer)
        return 2;
    if (connnect)
        return 1;
    else
        return 0;
}
