// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECLIENT_H
#define FILECLIENT_H

#include "server/http/http_client.h"

using Progress = std::function<bool(uint64_t current, uint64_t total)>;

class HTTPFileClient;
class FileClient
{
    friend class HTTPFileClient;
public:
    FileClient(const std::shared_ptr<CppServer::Asio::Service> &service, const std::string &address, int port);
    ~FileClient();

    void setToken(const std::string &token);
    void setSave(const std::string &savedir);
    bool prepareDownload(const std::string &resource, std::string *body);
    void downloadFolder(const std::string &folderName, Progress progress);
    bool downloadFile(const std::string &name, Progress progress);
    void cancel();

    void walkDownload(const std::string &name, const std::string &token, const std::string &savedir);
    

private:
    std::shared_ptr<HTTPFileClient> _httpClient { nullptr };
    std::string _token;
    std::string _savedir;
};

#endif // FILECLIENT_H
