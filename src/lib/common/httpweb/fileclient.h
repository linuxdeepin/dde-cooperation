// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECLIENT_H
#define FILECLIENT_H

#include "asio/ssl_context.h"
#include "syncstatus.h"

#include "webproto.h"

#include <queue>

class HTTPFileClient;
class FileClient : public WebInterface
{
    friend class HTTPFileClient;
public:
    FileClient(const std::shared_ptr<NetUtil::Asio::Service> &service, const std::shared_ptr<NetUtil::Asio::SSLContext>& context, const std::string &address, int port);
    ~FileClient();

    std::vector<std::string> parseWeb(const std::string &token);

    void setConfig(const std::string &token, const std::string &savedir);
    void stop();

    // start download in new thread
    void startFileDownload(const std::vector<std::string> &webnames);

private:
    void sendInfobyHeader(uint8_t mask, const std::string &name = "");
    InfoEntry requestInfo(const std::string &name);
    std::string getHeadKey(const std::string &headstrs, const std::string &keyfind);
    bool downloadFile(const std::string &name, const std::string &rename = "");
    void walkDownload(const std::vector<std::string> &webnames);
    bool createNotExistPath(std::string &abspath, bool isfile);
    std::string createNextAvailableName(const std::string &name, bool isfile);

    void walkFolder(const std::string &foldername);
    void walkFolderEntry(const std::string &name, std::queue<std::string> *entryQueue);

    std::shared_ptr<HTTPFileClient> _httpClient { nullptr };
    std::thread _downloadThread;

    std::string _token;
    std::string _savedir;
    std::atomic<bool> _stop { false };
};

#endif // FILECLIENT_H
