// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILESERVER_H
#define FILESERVER_H

//#include "server/http/https_server.h"
#include "server/http/http_server.h"
#include "string/string_utils.h"
#include "utility/singleton.h"

#include <iostream>
#include <map>
#include <mutex>

class HTTPFileServer;
class FileServer
{
    friend class HTTPFileServer;
public:
    FileServer(const std::shared_ptr<CppServer::Asio::Service> &service, int port);
    ~FileServer();

    std::string AddFileContent(const CppCommon::Path &path);

protected:

private:
    std::shared_ptr<HTTPFileServer> _httpServer { nullptr };
};

#endif // FILESERVER_H
