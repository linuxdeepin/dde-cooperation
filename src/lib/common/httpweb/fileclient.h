// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECLIENT_H
#define FILECLIENT_H

#include "server/http/http_client.h"
#include "syncstatus.h"

#include "filesystem/file.h"
#include "webproto.h"

class HTTPFileClient;
class FileClient : public WebInterface
{
    friend class HTTPFileClient;
public:
    FileClient(const std::shared_ptr<CppServer::Asio::Service> &service, const std::string &address, int port);
    ~FileClient();

    std::vector<std::string> parseWeb(const std::string &token);

    void setConfig(const std::string &token, const std::string &savedir);
    void cancel(bool cancel = true);
    bool downloading();

    // start download in new thread
    void startFileDownload(const std::vector<std::string> &webnames);

private:
    InfoEntry requestInfo(const std::string &name);
    std::string getHeadKey(const std::string &headstrs, const std::string &keyfind);
    bool downloadFile(const std::string &name);
    void downloadFolder(const std::string &folderName);
    void walkDownload(const std::vector<std::string> &webnames);

    std::shared_ptr<HTTPFileClient> _httpClient { nullptr };

    std::string _token;
    std::string _savedir;
    bool _cancel { false };
    bool _running { false };

    CppCommon::File tempFile;
};

#endif // FILECLIENT_H
