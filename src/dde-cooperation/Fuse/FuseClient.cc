// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FuseClient.h"

#include <functional>
#include <filesystem>
#include <chrono>

#include <sys/stat.h>

#include <fmt/core.h>
#include <google/protobuf/util/time_util.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QProcess>

#include "utils/net.h"
#include "utils/message_helper.h"
#include "protocol/message.pb.h"

using namespace std::chrono_literals;

namespace fs = std::filesystem;

template <auto F>
struct fuseOpsWrapper;

template <typename... Args, int (FuseClient::*F)(Args...)>
struct fuseOpsWrapper<F> {
    static int func(Args... args) {
        auto *ctx = fuse_get_context();
        auto *p = reinterpret_cast<FuseClient *>(ctx->private_data);
        return (p->*F)(args...);
    }
};

FuseClient::FuseClient(const std::string &ip,
                       uint16_t port,
                       const std::filesystem::path &mountpoint)
    : m_conn(new QTcpSocket(this))
    , m_ip(ip)
    , m_port(port)
    , m_mountpoint(mountpoint)
    , m_args(FUSE_ARGS_INIT(0, nullptr))
    , m_fuse(std::unique_ptr<fuse, decltype(&fuse_destroy)>(nullptr, &fuse_destroy)) {
    qInfo() << fmt::format("FuseClient::FuseClient, mountpoint: {}", m_mountpoint.string()).data();

    QProcess *process = new QProcess(this);
    connect(process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [this, process]([[maybe_unused]] int exitCode, QProcess::ExitStatus exitStatus) {
                if (!exitStatus) {
                    qInfo("umount point success");
                }

                if (!fs::exists(m_mountpoint)) {
                    fs::create_directories(m_mountpoint);
                }

                fuse_opt_add_arg(&m_args, m_mountpoint.c_str());
                fuse_opt_add_arg(&m_args, "-d");

                connect(m_conn, &QTcpSocket::connected, [this] {
                    Net::tcpSocketSetKeepAliveOption(m_conn->socketDescriptor());
                    m_mountThread = std::thread(&FuseClient::mount, this);
                });
                connect(m_conn, &QTcpSocket::readyRead, this, &FuseClient::handleResponse);
                m_conn->connectToHost(QHostAddress(QString::fromStdString(m_ip)), m_port);

                process->deleteLater();
            });
    process->start("/usr/bin/umount", QStringList{QString::fromStdString(m_mountpoint.string())});
}

FuseClient::~FuseClient() {
    m_conn->close();
    exit();
}

bool FuseClient::mount() {
    qInfo("FuseClient::mount");

    fuse_operations ops{};
    ops.getattr = fuseOpsWrapper<&FuseClient::getattr>::func,
    ops.open = fuseOpsWrapper<&FuseClient::open>::func,
    ops.read = fuseOpsWrapper<&FuseClient::read>::func,
    ops.release = fuseOpsWrapper<&FuseClient::release>::func,
    ops.readdir = fuseOpsWrapper<&FuseClient::readdir>::func,

    m_fuse.reset(fuse_new(&m_args, &ops, sizeof(ops), this));
    int ret = fuse_mount(m_fuse.get(), m_mountpoint.c_str());
    if (ret != 0) {
        return false;
    }

    ret = fuse_loop(m_fuse.get());
    fuse_unmount(m_fuse.get());
    if (ret != 0) {
        return false;
    }

    m_fuse.reset(nullptr);
    return true;
}

void FuseClient::exit() {
    if (m_fuse) {
        // 此函数只是设置标志，并不能中断 fuse 的阻塞，
        // 所以需要调用一下 stat 解除阻塞
        fuse_exit(m_fuse.get());

        struct stat statbuf;
        stat(m_mountpoint.c_str(), &statbuf);

        if (m_mountThread.joinable()) {
            m_mountThread.join();
        }
    }
}

int FuseClient::getattr(const char *path,
                        struct stat *const st,
                        [[maybe_unused]] struct fuse_file_info *fi) {
    qDebug() << fmt::format("getattr: {}", path).data();

    QMetaObject::invokeMethod(this, [this, path]() {
        Message msg;
        FsMethodGetAttrRequest *req = msg.mutable_fsmethodgetattrrequest();
        m_serial++;
        req->set_serial(m_serial);
        req->set_path(path);
        m_conn->write(MessageHelper::genMessage(msg));
    });

    auto resp = std::static_pointer_cast<FsMethodGetAttrResponse>(waitForServerReply());
    if (!resp) {
        errno = ETIMEDOUT;
        return -1;
    }

    auto retStat = resp->stat();

    memset(st, 0, sizeof(struct stat));
    st->st_ino = retStat.ino();
    st->st_nlink = retStat.nlink();
    st->st_mode = retStat.mode();
    st->st_uid = retStat.uid();
    st->st_gid = retStat.gid();
    st->st_size = retStat.size();
    st->st_blksize = retStat.blksize();
    st->st_blocks = retStat.blocks();
    st->st_atim.tv_sec = retStat.atime().seconds();
    st->st_atim.tv_nsec = retStat.atime().nanos();
    st->st_mtim.tv_sec = retStat.mtime().seconds();
    st->st_mtim.tv_nsec = retStat.mtime().nanos();
    st->st_ctim.tv_sec = retStat.ctime().seconds();
    st->st_ctim.tv_nsec = retStat.ctime().nanos();

    qDebug() << fmt::format("path: {}, mode: {}, S_IFDIR: {}, S_IFREG: {}, nlink: {}",
                            path,
                            static_cast<uint32_t>(st->st_mode),
                            static_cast<bool>(st->st_mode & S_IFDIR),
                            static_cast<bool>(st->st_mode & S_IFREG),
                            st->st_nlink)
                    .data();

    return resp->result();
}

int FuseClient::open(const char *path, struct fuse_file_info *fi) {
    qDebug() << fmt::format("open: {}", path).data();

    QMetaObject::invokeMethod(this, [this, path]() {
        Message msg;
        FsMethodOpenRequest *req = msg.mutable_fsmethodopenrequest();
        m_serial++;
        req->set_serial(m_serial);
        req->set_path(path);
        m_conn->write(MessageHelper::genMessage(msg));
    });

    auto resp = std::static_pointer_cast<FsMethodOpenResponse>(waitForServerReply());
    if (!resp) {
        errno = ETIMEDOUT;
        return -1;
    }

    if (resp->has_fh()) {
        fi->fh = resp->fh();
    }

    fi->direct_io = true;

    return resp->result();
}

int FuseClient::read(const char *path,
                     char *buf,
                     size_t size,
                     off_t offset,
                     struct fuse_file_info *fi) {
    qDebug()
        << fmt::format("read: {}, fh: {}, size: {}, offset: {}", path, fi->fh, size, offset).data();

    QMetaObject::invokeMethod(this, [this, offset, size, fi]() {
        Message msg;
        FsMethodReadRequest *req = msg.mutable_fsmethodreadrequest();
        m_serial++;
        req->set_serial(m_serial);
        req->set_offset(offset);
        req->set_size(size);
        auto rfi = req->mutable_fi();
        rfi->set_fh(fi->fh);
        m_conn->write(MessageHelper::genMessage(msg));
    });

    auto resp = std::static_pointer_cast<FsMethodReadResponse>(waitForServerReply());
    if (!resp) {
        errno = ETIMEDOUT;
        return -1;
    }

    qInfo()
        << fmt::format("readed size: {}, result: {}", resp->data().size(), resp->result()).data();
    memcpy(buf, resp->data().data(), resp->data().size());

    return resp->result();
}

int FuseClient::release(const char *path, struct fuse_file_info *fi) {
    qDebug() << fmt::format("release: {}", path).data();

    QMetaObject::invokeMethod(this, [this, path, fi]() {
        Message msg;
        FsMethodReleaseRequest *req = msg.mutable_fsmethodreleaserequest();
        m_serial++;
        req->set_serial(m_serial);
        req->set_path(path);
        req->mutable_fi()->set_fh(fi->fh);
        m_conn->write(MessageHelper::genMessage(msg));
    });

    auto resp = std::static_pointer_cast<FsMethodReleaseResponse>(waitForServerReply());
    if (!resp) {
        errno = ETIMEDOUT;
        return -1;
    }

    return resp->result();
}

int FuseClient::readdir(const char *path,
                        void *buf,
                        fuse_fill_dir_t filler,
                        [[maybe_unused]] off_t offset,
                        [[maybe_unused]] struct fuse_file_info *fi,
                        [[maybe_unused]] enum fuse_readdir_flags flags) {
    qDebug() << fmt::format("readdir: {}", path).data();

    QMetaObject::invokeMethod(this, [this, path]() {
        Message msg;
        FsMethodReadDirRequest *req = msg.mutable_fsmethodreaddirrequest();
        m_serial++;
        req->set_serial(m_serial);
        req->set_path(path);
        m_conn->write(MessageHelper::genMessage(msg));
    });

    auto resp = std::static_pointer_cast<FsMethodReadDirResponse>(waitForServerReply());
    if (!resp) {
        errno = ETIMEDOUT;
        return -1;
    }

    for (const std::string &i : resp->item()) {
        filler(buf, i.c_str(), nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    }

    return resp->result();
}

void FuseClient::handleResponse() noexcept {
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
        case Message::PayloadCase::kFsMethodGetAttrResponse: {
            const auto &data = msg.fsmethodgetattrresponse();
            m_buff.reset(new FsMethodGetAttrResponse(data));
            m_cv.notify_one();
            break;
        }
        case Message::PayloadCase::kFsMethodOpenResponse: {
            const auto &data = msg.fsmethodopenresponse();
            m_buff.reset(new FsMethodOpenResponse(data));
            m_cv.notify_one();
            break;
        }
        case Message::PayloadCase::kFsMethodReadResponse: {
            const auto &data = msg.fsmethodreadresponse();
            m_buff.reset(new FsMethodReadResponse(data));
            m_cv.notify_one();
            break;
        }
        case Message::PayloadCase::kFsMethodReadDirResponse: {
            const auto &data = msg.fsmethodreaddirresponse();
            m_buff.reset(new FsMethodReadDirResponse(data));
            m_cv.notify_one();
            break;
        }
        case Message::PayloadCase::kFsMethodReleaseResponse: {
            const auto &data = msg.fsmethodreleaseresponse();
            m_buff.reset(new FsMethodReleaseResponse(data));
            m_cv.notify_one();
            break;
        }
        default: {
            qWarning() << "FuseClient unknown message type:" << msg.payload_case();
            break;
        }
        }
    }
}

std::shared_ptr<google::protobuf::Message> FuseClient::waitForServerReply() {
    std::unique_lock lk(m_mut);
    m_buff.reset();
    m_cv.wait_for(lk, 3s);
    if (m_buff) {
        qInfo("fuse responded");
    } else {
        qInfo("fuse not responded");
    }

    return m_buff;
}
