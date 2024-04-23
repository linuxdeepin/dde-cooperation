// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECLIENT_H
#define FILECLIENT_H

#include "server/http/http_client.h"
#include "syncstatus.h"

//using Progress = std::function<bool(const std::string &path, uint64_t current, uint64_t total)>;

class HTTPFileClient;
class FileClient : public WebInterface
{
    friend class HTTPFileClient;
public:
    FileClient(const std::shared_ptr<CppServer::Asio::Service> &service, const std::string &address, int port);
    ~FileClient();

//    void setCallback(std::shared_ptr<ProgressCallInterface> callback);
    void setConfig(const std::string &token, const std::string &savedir);
    void cancel(bool cancel = true);
    bool downloading();

    bool downloadFile(const std::string &name);
    void downloadFolder(const std::string &folderName);
//    void walkDownload(const std::string &name, const std::string &token, const std::string &savedir);

private:
    bool getInfo(const std::string &resource, std::string *body);

//    std::shared_ptr<ProgressCallInterface> _callback { nullptr };
    std::shared_ptr<HTTPFileClient> _httpClient { nullptr };

    std::string _token;
    std::string _savedir;
    bool _cancel { false };
    bool _running { false };
};

#endif // FILECLIENT_H
