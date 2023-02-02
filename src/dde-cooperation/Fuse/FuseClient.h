// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FUSE_FUSECLIENT_H
#define FUSE_FUSECLIENT_H

#include <filesystem>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>

#include <google/protobuf/message.h>

#include <QObject>

class QTcpSocket;

class FuseClient : public QObject {
    Q_OBJECT

public:
    explicit FuseClient(const std::string &ip,
                        uint16_t port,
                        const std::filesystem::path &mountpoint);
    ~FuseClient();

    bool mount();
    void unmount() { exit(); }
    void exit();

private:
    QTcpSocket *m_conn;

    std::string m_ip;
    uint16_t m_port;
    const std::filesystem::path m_mountpoint;

    fuse_args m_args;
    std::unique_ptr<fuse, decltype(&fuse_destroy)> m_fuse;
    uint16_t m_serial;

    std::thread m_mountThread;
    std::mutex m_mut;
    std::condition_variable m_cv;
    std::shared_ptr<google::protobuf::Message> m_buff;

    int getattr(const char *path, struct stat *st, struct fuse_file_info *fi);
    int open(const char *path, struct fuse_file_info *fi);
    int read(const char *path, char *buf, size_t size, off_t offser, struct fuse_file_info *fi);
    int release(const char *path, struct fuse_file_info *fi);
    int readdir(const char *path,
                void *buf,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

    void handleResponse() noexcept;
    std::shared_ptr<google::protobuf::Message> waitForServerReply();
};

#endif // !FUSE_FUSECLIENT_H
