// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipcwraper.h"
#include "ipcaction.h"

IpcSession::IpcSession(Whoami ami)
    : identity(ami)
{
    ipcValid = beginListen();
}

IpcSession::~IpcSession()
{
    try {
        ipc::message_queue::remove("uniapi-queue");
        std::cout << "Removed uniapi-queue" << std::endl;
    } catch (ipc::interprocess_exception &ex) {
        std::cout << "Error: " << ex.what() << std::endl;
    }
}

int IpcSession::createMQ()
{
    try {
        msg_queue = std::make_unique<ipc::message_queue>(
            ipc::open_or_create, "uniapi-queue", MAX_MQ_LEN, sizeof(IpcMessage));

        // mq_down = std::make_unique<boost::interprocess::message_queue>(
        //     ipc::open_or_create, "uniapi-down-queue", MAX_MQ_LEN, sizeof(IpcMessage));
    } catch (ipc::interprocess_exception &ex) {
        std::cout << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}

bool IpcSession::beginListen()
{
	asio::io_context ioc;
    if (createMQ()) {
        std::cout << "Error: fail to createMQ" << std::endl;
        return false;
    }

    // 使用 spawn 函数创建协程，并传入消息队列参数 mq
    asio::spawn(ioc, [&](asio::yield_context yield) {
        run(yield);
    });

    ioc.run();

    return true;
}

void IpcSession::run(asio::yield_context yield)
{
    // try {
        unsigned int priority;
        boost::interprocess::message_queue::size_type recvd_size;

        void *ipc_msg = malloc(sizeof(IpcMessage));

        while (true) {
            memset(ipc_msg, 0, sizeof(IpcMessage));
            msg_queue->receive(ipc_msg, sizeof(IpcMessage), recvd_size, priority);

            IpcMessage *msg = (IpcMessage *)ipc_msg;
            printf("Consumed: event=%d %s=%s, %d) len:%ld \n", msg->event, msg->key, msg->value, msg->spare, recvd_size);

            if (msg->from == msg->to || msg->from == identity) {
                printf("SHIT: self event=%d %s=%s, %d) len:%ld \n", msg->event, msg->key, msg->value, msg->spare, recvd_size);
                continue;
            }

            if (msg->to == identity) {
                emit notifyMessage(msg);
            }
        }
    // } catch (ipc::interprocess_exception &ex) {
    //     std::cout << "Error: " << ex.what() << std::endl;
    //     ipcValid = false;
    //     return 1;
    // }

    // return 0;
}

bool IpcSession::sendMessage(IpcMessage *message)
{
    if (msg_queue == nullptr) {
        std::cout << "Error: mq_up is null !" << std::endl;
        return -1;
    }
    try {
        message->from = identity;
         msg_queue->send(message, sizeof(IpcMessage), 0);
    } catch (ipc::interprocess_exception &ex) {
        std::cout << "Error: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

// int IpcSession::sendDownMessage(IpcMessage *message)
// {
//     if (mq_down == nullptr) {
//         std::cout << "Error: mq_down is null !" << std::endl;
//         return -1;
//     }
//     try {
//          mq_down->send(message, sizeof(IpcMessage), 0);
//     } catch (ipc::interprocess_exception &ex) {
//         std::cout << "Error: " << ex.what() << std::endl;
//         return 1;
//     }

//     return 0;
// }

IpcWraper::IpcWraper(QObject *parent) : QObject(parent)
{


}

bool IpcWraper::bindWho(Whoami ami)
{
    if (session == nullptr) {
        session = new IpcSession(ami);
        connect(session, &IpcSession::notifyMessage, this, &IpcWraper::onMessage, Qt::QueuedConnection);
    }

    return session ? session->isValid() : false;
}

bool IpcWraper::sendMessageTo(Whoami to, IpcEvent event, char *key, char *value, int spare)
{
    IpcMessage msg;
    msg.to = to;
    msg.event = event;
    strcpy(msg.key, key);
    strcpy(msg.value, value);
    msg.spare = spare;

    return session->sendMessage(&msg);
}

void IpcWraper::onMessage(IpcMessage *message)
{
    switch (message->from)
    {
        // 处理来自服务端的IPC消息
        case ServiceDeamon:
            /* code */
            break;

        // 处理来自 跨端协同 的IPC消息
        case AppCooperation:
            /* code */
            break;


        // 处理来自 数据迁移 的IPC消息
        case AppDataTrans:
            /* code */
            break;


        // 处理来自 文件投送 的IPC消息
        case AppSendFile:
            /* code */
            break;
        
        default:
            break;
    }
}

void functionWithIntParam(int value) {
    // std::cout << "Calling functionWithIntParam() with value: " << value << std::endl;
}

void functionWithStringParam(const std::string& str) {
    // std::cout << "Calling functionWithStringParam() with string: " << str << std::endl;
}

void IpcWraper::initFuncs()
{
    functionMap.clear();
    // functionMap["functionWithIntParam"] = std::bind(functionWithIntParam, 42);
    // functionMap["functionWithStringParam"] = std::bind(functionWithStringParam, "Hello");

    functionMap["functionWithIntParam"] = [](std::any arg) {
        if (arg.type() == typeid(int)) {
            int value = std::any_cast<int>(arg);
            functionWithIntParam(value);
        }
    };
    functionMap["functionWithStringParam"] = [](std::any arg) {
        if (arg.type() == typeid(std::string)) {
            std::string str = std::any_cast<std::string>(arg);
            functionWithStringParam(str);
        }
    };
}

int IpcWraper::invokeFunc(const std::string& funname, std::any argument)
{
    auto it = functionMap.find(funname);
    if (it != functionMap.end()) {
        it->second(argument);
        return 0;
    }

    return -1;
}
