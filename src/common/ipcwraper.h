// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IPCWRAPER_H
#define IPCWRAPER_H

#include <map>
#include <functional>
#include <any>
#include <iostream>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/spawn.hpp>


#include <QObject>

#include "ipcaction.h"

namespace asio = boost::asio;
namespace ipc = boost::interprocess;


class IpcSession : public QObject
{
Q_OBJECT
public:
	explicit IpcSession(Whoami ami);
    ~IpcSession();

    bool isValid() const { return ipcValid; };

public slots:
    bool sendMessage(IpcMessage *message);
    // int sendDownMessage(IpcMessage *message);

signals:
    void notifyMessage(IpcMessage *message);

private:
    int createMQ();
    bool beginListen();
    void run(asio::yield_context yield);

    bool ipcValid;
    Whoami identity;
    // IPC 消息队列
    std::unique_ptr<boost::interprocess::message_queue> msg_queue;
};

class IpcWraper : public QObject
{
    Q_OBJECT
public:
    explicit IpcWraper(QObject *parent = nullptr);
    bool bindWho(Whoami ami);

    bool sendMessageTo(Whoami to, IpcEvent event, char *key, char *value, int spare);

    void initFuncs();
    int invokeFunc(const std::string& funname, std::any argument);

signals:

public slots:
    void onMessage(IpcMessage *message);

private:
    IpcSession *session { nullptr };

    // 创建映射表并关联字符串与函数
    std::map<std::string, std::function<void(std::any)>> functionMap;
};

#endif // IPCWRAPER_H