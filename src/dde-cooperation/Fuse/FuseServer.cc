// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FuseServer.h"

#include <filesystem>
#include <algorithm>

#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <QTcpServer>
#include <QTcpSocket>

#include <fmt/core.h>
#include <google/protobuf/util/time_util.h>

#include "Machine/Machine.h"
#include "utils/message_helper.h"
#include "protocol/message.pb.h"

namespace fs = std::filesystem;

static constexpr size_t maxRead = 128 * 1024;

FuseServer::FuseServer(const std::weak_ptr<Machine> &machine)
    : m_machine(machine)
    , m_listen(new QTcpServer(this))
    , m_conn(nullptr) {

    m_listen->listen(QHostAddress::Any);
    m_listen->setMaxPendingConnections(1);
    connect(m_listen, &QTcpServer::newConnection, this, &FuseServer::handleNewConnection);
}

FuseServer::~FuseServer() {
    if (m_conn) {
        m_conn->close();
    }
    m_listen->close();
}

uint16_t FuseServer::port() const {
    return m_listen->serverPort();
}

void FuseServer::handleNewConnection() noexcept {
    if (m_conn) {
        return;
    }

    // TODO: check client
    m_conn = m_listen->nextPendingConnection();
    connect(m_conn, &QTcpSocket::readyRead, this, &FuseServer::handleRequest);
    connect(m_conn, &QTcpSocket::disconnected, this, &FuseServer::handleDisconnected);

    m_listen->close();
}

void FuseServer::handleDisconnected() noexcept {
    m_conn->deleteLater();
    m_conn = nullptr;
}

void FuseServer::handleRequest() noexcept {
    while (m_conn->size() >= header_size) {
        QByteArray buffer = m_conn->peek(header_size);
        auto header = MessageHelper::parseMessageHeader(buffer);
        if (!header.legal()) {
            qWarning() << "illegal message from " << m_conn->peerAddress().toString();
            return;
        }

        if (m_conn->size() < static_cast<qint64>(header_size + header.size())) {
            qDebug() << "partial packet:" << m_conn->size();
            return;
        }

        m_conn->read(header_size);
        auto size = header.size();
        buffer = m_conn->read(size);

        Message msg = MessageHelper::parseMessageBody<Message>(buffer.data(), buffer.size());

        switch (msg.payload_case()) {
        case Message::PayloadCase::kFsMethodGetAttrRequest: {
            const auto &req = msg.fsmethodgetattrrequest();

            Message resp;
            methodGetattr(req, resp.mutable_fsmethodgetattrresponse());
            m_conn->write(MessageHelper::genMessage(resp));
            break;
        }
        case Message::PayloadCase::kFsMethodReadRequest: {
            const auto &req = msg.fsmethodreadrequest();

            Message resp;
            methodRead(req, resp.mutable_fsmethodreadresponse());
            m_conn->write(MessageHelper::genMessage(resp));
            break;
        }
        case Message::PayloadCase::kFsMethodReadDirRequest: {
            const auto &req = msg.fsmethodreaddirrequest();

            Message resp;
            methodReaddir(req, resp.mutable_fsmethodreaddirresponse());
            m_conn->write(MessageHelper::genMessage(resp));
            break;
        }
        case Message::PayloadCase::kFsMethodOpenRequest: {
            const auto &req = msg.fsmethodopenrequest();

            Message resp;
            methodOpen(req, resp.mutable_fsmethodopenresponse());
            m_conn->write(MessageHelper::genMessage(resp));
            break;
        }
        case Message::PayloadCase::kFsMethodReleaseRequest: {
            const auto &req = msg.fsmethodreleaserequest();

            Message resp;
            methodRelease(req, resp.mutable_fsmethodreleaseresponse());
            m_conn->write(MessageHelper::genMessage(resp));
            break;
        }
        default: {
            qWarning() << "FuseServer unknown message type:" << msg.payload_case();
            break;
        }
        }
    }
}

void FuseServer::methodGetattr(const FsMethodGetAttrRequest &req, FsMethodGetAttrResponse *resp) {
    qInfo() << fmt::format("methodGetattr: {}", req.path()).data();

    resp->set_serial(req.serial());

    struct stat st;
    int result = stat(req.path().c_str(), &st);

    resp->set_result(result);
    resp->mutable_stat()->set_ino(st.st_ino);
    resp->mutable_stat()->set_nlink(st.st_nlink);
    resp->mutable_stat()->set_mode(st.st_mode);
    resp->mutable_stat()->set_uid(st.st_uid);
    resp->mutable_stat()->set_gid(st.st_gid);
    resp->mutable_stat()->set_size(st.st_size);
    resp->mutable_stat()->set_blksize(st.st_blksize);
    resp->mutable_stat()->set_blocks(st.st_blocks);

    resp->mutable_stat()->mutable_atime()->set_seconds(st.st_atim.tv_sec);
    resp->mutable_stat()->mutable_atime()->set_nanos(st.st_atim.tv_nsec);
    resp->mutable_stat()->mutable_mtime()->set_seconds(st.st_mtim.tv_sec);
    resp->mutable_stat()->mutable_mtime()->set_nanos(st.st_mtim.tv_nsec);
    resp->mutable_stat()->mutable_ctime()->set_seconds(st.st_ctim.tv_sec);
    resp->mutable_stat()->mutable_ctime()->set_nanos(st.st_ctim.tv_nsec);

    qDebug() << fmt::format("path: {}, mode: {}, S_IFDIR: {}, S_IFREG: {}, nlink: {}",
                            req.path(),
                            static_cast<uint32_t>(st.st_mode),
                            static_cast<bool>(st.st_mode & S_IFDIR),
                            static_cast<bool>(st.st_mode & S_IFREG),
                            st.st_nlink)
                    .data();
}

void FuseServer::methodOpen(const FsMethodOpenRequest &req, FsMethodOpenResponse *resp) {
    qInfo() << fmt::format("methodOpen: {}", req.path()).data();

    resp->set_serial(req.serial());

    int flags = 0;
    if (req.has_fi()) {
        flags = req.fi().flags();
    }

    int result = 0;
    int fd = open(req.path().c_str(), flags);
    if (fd == -1) {
        result = errno;
    }

    resp->set_result(result);
    resp->set_fh(fd);
}

void FuseServer::methodRead(const FsMethodReadRequest &req, FsMethodReadResponse *resp) {
    resp->set_serial(req.serial());

    if (!req.has_fi()) {
        qWarning("methodRead: no fi");
        resp->set_result(-EBADF);
        return;
    }

    qInfo() << fmt::format("methodRead: fh: {}", req.fi().fh()).data();

    off_t off = lseek(req.fi().fh(), req.offset(), SEEK_SET);
    if (off == -1) {
        qWarning() << fmt::format("lseek failed: {}({})", strerror(errno), errno).data();
        resp->set_result(-errno);
        return;
    }

    size_t size = req.size();
    if (size > maxRead) {
        size = maxRead;
    }

    resp->mutable_data()->resize(size);
    size = read(req.fi().fh(), resp->mutable_data()->data(), size);
    resp->mutable_data()->resize(size);
    resp->set_result(size);
}

void FuseServer::methodRelease(const FsMethodReleaseRequest &req, FsMethodReleaseResponse *resp) {
    resp->set_serial(req.serial());

    if (!req.has_fi()) {
        qWarning("methodRelease: no fi");
        resp->set_result(EBADF);
        return;
    }

    qInfo() << fmt::format("methodRelease: fh: {}", req.fi().fh()).data();
    int result = close(req.fi().fh());
    resp->set_result(result);
}

void FuseServer::methodReaddir(const FsMethodReadDirRequest &req, FsMethodReadDirResponse *resp) {
    qInfo() << fmt::format("methodReaddir: {}", req.path()).data();

    resp->set_serial(req.serial());

    resp->set_result(0);

    fs::path p = req.path();

    auto iter = fs::directory_iterator(p, fs::directory_options::skip_permission_denied);
    auto end_iter = fs::end(iter);
    auto ec = std::error_code();
    for (; iter != end_iter; iter.increment(ec)) {
        if (ec) {
            continue;
        }

        resp->mutable_item()->Add(iter->path().filename().string());
    }
}
