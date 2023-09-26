#include "transferworker.h"

#include <co/rpc.h>
#include <co/co.h>

TransferHandle::TransferHandle()
    : QObject()
{
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
