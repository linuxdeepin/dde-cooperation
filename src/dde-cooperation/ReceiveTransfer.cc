// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReceiveTransfer.h"

#include <fstream>

#include <fmt/core.h>

#include <QTcpServer>
#include <QTcpSocket>

#include "utils/message_helper.h"

namespace fs = std::filesystem;

static bool isSubPath(const fs::path &base, const fs::path &path) {
    auto baseTmp = base / "";

    return path.string().rfind(baseTmp.string(), 0) == 0;
}

ReceiveTransfer::ReceiveTransfer(const fs::path &dest, QObject *parent)
    : QObject(parent)
    , m_listen(new QTcpServer(this))
    , m_conn(nullptr)
    , m_dest(dest) {
    m_listen->setMaxPendingConnections(1);
    m_listen->listen(QHostAddress::Any);

    connect(m_listen, &QTcpServer::newConnection, this, &ReceiveTransfer::handleNewConnection);
}

uint16_t ReceiveTransfer::port() {
    return m_listen->serverPort();
}

std::filesystem::path ReceiveTransfer::getPath(const std::string &relPath) {
    return m_dest / relPath;
}

void ReceiveTransfer::handleNewConnection() {
    if (m_conn) {
        return;
    }

    m_conn = m_listen->nextPendingConnection();

    connect(m_conn, &QTcpSocket::readyRead, this, &ReceiveTransfer::dispatcher);
    connect(m_conn, &QTcpSocket::disconnected, this, &ReceiveTransfer::handleDisconnected);

    m_listen->close();
}

void ReceiveTransfer::handleDisconnected() {
    deleteLater();
}

void ReceiveTransfer::dispatcher() {
    while (m_conn->size() >= header_size) {
        QByteArray buffer = m_conn->peek(header_size);
        auto header = MessageHelper::parseMessageHeader(buffer);
        if (!header.legal()) {
            qWarning() << "illegal message from " << m_conn->peerAddress().toString();
            return;
        }

        if (m_conn->size() < static_cast<qint64>(header_size + header.size())) {
            qDebug() << "partial content";
            return;
        }

        m_conn->read(header_size);
        auto size = header.size();
        buffer = m_conn->read(size);

        Message msg = MessageHelper::parseMessageBody<Message>(buffer.data(), buffer.size());
        qDebug() << fmt::format("message type: {}", msg.payload_case()).data();

        switch (msg.payload_case()) {
        case Message::PayloadCase::kSendFileRequest: {
            handleSendFileRequest(msg.sendfilerequest());
            break;
        }
        case Message::PayloadCase::kSendFileChunkRequest: {
            handleSendFileChunkRequest(msg.sendfilechunkrequest());
            break;
        }
        case Message::PayloadCase::kStopSendFileRequest: {
            handleStopSendFileRequest(msg.stopsendfilerequest());
            break;
        }
        case Message::PayloadCase::kSendDirRequest: {
            handleSendDirRequest(msg.senddirrequest());
            break;
        }
        default: {
            qWarning() << "ReceiveTransfer invalid message type:" << msg.payload_case();
            break;
        }
        }
    }
}

void ReceiveTransfer::handleSendFileRequest(const SendFileRequest &req) {
    auto path = getPath(req.relpath());
    qDebug() << "save file to:" << QString::fromStdString(path);

    if (!isSubPath(m_dest, path)) {
        qWarning() << "illegal path:" << QString::fromStdString(req.relpath());
        m_conn->abort();
        return;
    }

    decltype(m_streams.begin()) iter;
    bool r;
    std::tie(iter, r) = m_streams.emplace(std::piecewise_construct,
                                          std::forward_as_tuple(path.string()),
                                          std::forward_as_tuple(QString::fromStdString(path)));
    iter->second.open(QFile::ReadWrite | QFile::Truncate);

    Message msg;
    msg.mutable_sendfileresponse();
    m_conn->write(MessageHelper::genMessage(msg));
}

void ReceiveTransfer::handleStopSendFileRequest(const StopSendFileRequest &req) {
    auto path = getPath(req.relpath());
    auto iter = m_streams.find(path);
    if (iter == m_streams.end()) {
        return;
    }

    auto sha256 = req.sha256();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    iter->second.reset();
    if (hash.addData(&iter->second)) {
        auto res = hash.result().toHex().toStdString();
        if (res != sha256) {
            qWarning() << fmt::format("file hash mismatch, {} {}", res, sha256).data();
            m_streams.erase(iter);
            return;
        }
    }

    m_streams.erase(iter);

    Message msg;
    msg.mutable_stopsendfileresponse();
    m_conn->write(MessageHelper::genMessage(msg));
}

void ReceiveTransfer::handleSendFileChunkRequest(const SendFileChunkRequest &req) {
    auto path = getPath(req.relpath());
    auto iter = m_streams.find(path);
    if (iter == m_streams.end()) {
        return;
    }

    QFile &stream = iter->second;
    stream.seek(req.offset());
    stream.write(req.data().data(), req.data().size());

    Message msg;
    auto *sendfilechunkresponse = msg.mutable_sendfilechunkresponse();
    sendfilechunkresponse->set_serial(req.serial());
    m_conn->write(MessageHelper::genMessage(msg));
}

void ReceiveTransfer::handleSendDirRequest(const SendDirRequest &req) {
    auto path = getPath(req.relpath());
    if (!isSubPath(m_dest, path)) {
        qWarning() << "illegal path:" << QString::fromStdString(req.relpath());
        m_conn->abort();
        return;
    }

    fs::create_directories(path);

    Message msg;
    msg.mutable_senddirresponse();
    m_conn->write(MessageHelper::genMessage(msg));
}

void ReceiveTransfer::handleStopTransferRequest([[maybe_unused]] const StopTransferRequest &req) {
    Message msg;
    msg.mutable_stoptransferresponse();
    m_conn->write(MessageHelper::genMessage(msg));
}
