// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILESERVER_H
#define FILESERVER_H

#include "webbinder.h"

//#include "server/http/https_server.h"
#include "server/http/http_server.h"
#include "string/string_utils.h"
#include "syncstatus.h"

#include <iostream>
#include <map>
#include <mutex>

class HTTPFileServer;
class FileServer : public WebInterface
{
    friend class HTTPFileServer;
public:
    FileServer(const std::shared_ptr<CppServer::Asio::Service> &service, int port);
    ~FileServer();

    bool start();
    bool stop();

    std::string addFileContent(const CppCommon::Path &path);
    void setWeb(std::string &token, const std::string &path);

    int webBind(std::string webDir, std::string diskDir);
    int webUnbind(std::string webDir);
    void clearBind();

    std::string genToken(std::string info);
    bool verifyToken(std::string &token);
    void clearToken();

protected:

private:
    std::shared_ptr<HTTPFileServer> _httpServer { nullptr };
};

#endif // FILESERVER_H
