// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILESERVER_H
#define FILESERVER_H

#include "http/https_server.h"
#include "syncstatus.h"

class FileServer : public WebInterface, public NetUtil::HTTP::HTTPSServer
{
    using NetUtil::HTTP::HTTPSServer::HTTPSServer;

public:
    ~FileServer();

    bool start();
    bool stop();

    int webBind(std::string webDir, std::string diskDir);
    int webUnbind(std::string webDir);
    void clearBind();

    std::string genToken(std::string info);
    bool verifyToken(std::string &token);

protected:
    std::shared_ptr<NetUtil::Asio::SSLSession> CreateSession(const std::shared_ptr<NetUtil::Asio::SSLServer> &server) override;
    void onError(int error, const std::string &category, const std::string &message) override;

private:
    std::atomic<bool> _stop { false };
};

#endif // FILESERVER_H
