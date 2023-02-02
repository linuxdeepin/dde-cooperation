// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RECEIVETRANSFER_H
#define RECEIVETRANSFER_H

#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>
#include <fstream>

#include <QObject>
#include <QFile>
#include <QCryptographicHash>

#include "protocol/message.pb.h"

class QTcpServer;
class QTcpSocket;

class ReceiveTransfer : public QObject {
    Q_OBJECT

public:
    ReceiveTransfer(const std::filesystem::path &dest, QObject *parent = nullptr);

    uint16_t port();

private:
    QTcpServer *m_listen;
    QTcpSocket *m_conn;
    std::filesystem::path m_dest;
    std::unordered_map<std::string, QFile> m_streams;

    std::filesystem::path getPath(const std::string &relpath);

    void handleNewConnection();
    void handleDisconnected();
    void dispatcher();
    void handleSendFileRequest(const SendFileRequest &req);
    void handleStopSendFileRequest(const StopSendFileRequest &req);
    void handleSendFileChunkRequest(const SendFileChunkRequest &req);
    void handleSendDirRequest(const SendDirRequest &req);
    void handleStopTransferRequest(const StopTransferRequest &req);
};

#endif // !RECEIVETRANSFER_H
