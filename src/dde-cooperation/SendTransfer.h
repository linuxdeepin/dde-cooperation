// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENDTRANSFER_H
#define SENDTRANSFER_H

#include <vector>
#include <string>
#include <memory>
#include <filesystem>

#include <QObject>
#include <QStringList>
#include <QDebug>

class Message;
class QStringList;
class QTcpServer;
class QTcpSocket;
class ObjectSendTransfer;

class SendTransfer : public QObject {
    Q_OBJECT

public:
    SendTransfer(const QStringList &filePaths, QObject *parent);

    uint16_t receive();
    void send(const std::string &ip, uint16_t port);
    void stop();

signals:
    void done();

private:
    QTcpSocket *m_conn;
    QStringList m_filePaths;
    ObjectSendTransfer *m_objectSendTransfer;

    void dispatcher();
    void sendNextObject();
    void handleDisconnected();
};

class ObjectSendTransfer : public QObject {
    Q_OBJECT

public:
    ObjectSendTransfer(QTcpSocket *conn,
                       const std::filesystem::path &base,
                       const std::filesystem::path &relPath,
                       QObject *parent)
        : QObject(parent)
        , m_conn(conn)
        , m_base(base)
        , m_relPath(relPath)
        , m_path(m_base / m_relPath) {
        qDebug() << "base:" << QString::fromStdString(m_base)
                 << "relPath:" << QString::fromStdString(m_relPath);
    }
    virtual ~ObjectSendTransfer() = default;

    virtual void handleMessage(const Message &msg) = 0;
    virtual bool done() = 0;
    virtual void sendRequest() = 0;

protected:
    QTcpSocket *m_conn;
    const std::filesystem::path m_base;
    const std::filesystem::path m_relPath;
    const std::filesystem::path m_path;
};

#endif // !SENDTRANSFER_H
