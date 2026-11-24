// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECLIENT_H
#define FILECLIENT_H

#include "asio/ssl_context.h"
#include "syncstatus.h"

#include "webproto.h"

#include <atomic>
#include <cstdint>
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

    // Cumulative HTTP request counters, for benchmarks and diagnostics.
    // Plain request counts only: no paths, no tokens.
    struct WebRequestCounters {
        uint64_t headerRequests {0};
        uint64_t infoRequests {0};
        uint64_t downloadRequests {0};
        uint64_t connections {0};
    };
    WebRequestCounters requestCounters() const;

private:
    void sendInfobyHeader(uint8_t mask, const std::string &name = "");
    // Distinguishes "the peer says it does not exist" from "the request
    // failed", which used to both look like an empty file.
    enum class InfoStatus {
        Ok,        // out is valid; an empty file has out->size == 0
        NotFound,  // HTTP 404
        Error,     // network/JSON/protocol failure
    };
    InfoStatus requestInfo(const std::string &name, InfoEntry *out);
    std::string getHeadKey(const std::string &headstrs, const std::string &keyfind);
    bool downloadFile(const std::string &name, const std::string &rename = "");

    // One request for the whole selection. Fallback means the peer does not
    // speak DTP2 (old server, 404/501): the caller must run the legacy path.
    enum class StreamAttempt {
        Ok,
        Fallback,
        Error,
        Canceled,
    };
    struct StreamResumeState {
        uint64_t nextFrame {0};
        std::string rootPath;
        std::string error;
    };
    StreamAttempt downloadStream(const std::string &name);
    StreamAttempt downloadStreamOnce(const std::string &name,
                                     const StreamResumeState &resume,
                                     StreamResumeState *next, bool *retryable);
    std::string checkpointPath(const std::string &name) const;
    StreamResumeState loadCheckpoint(const std::string &name);
    void saveCheckpoint(const std::string &name, const StreamResumeState &state);
    void clearCheckpoint(const std::string &name);

    void walkDownload(const std::vector<std::string> &webnames);
    bool createNotExistPath(std::string &abspath, bool isfile);
    std::string createNextAvailableName(const std::string &name, bool isfile);

    // Return false when the walk must abort (network/protocol failure); the
    // failing layer already reported the terminal state.
    bool walkFolder(const std::string &foldername);
    bool walkFolderEntry(const std::string &name, std::queue<std::string> *entryQueue, int depth);

    std::shared_ptr<HTTPFileClient> _httpClient { nullptr };
    std::thread _downloadThread;

    std::string _token;
    std::string _savedir;
    std::atomic<bool> _stop { false };
    std::atomic<uint64_t> _headerRequests { 0 };
    std::atomic<uint64_t> _infoRequests { 0 };
    std::atomic<uint64_t> _downloadRequests { 0 };
};

#endif // FILECLIENT_H
