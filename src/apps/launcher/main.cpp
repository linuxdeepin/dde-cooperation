// SPDX-FileCopyrightText: 2023-2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "manager/sessionmanager.h"

//#include <unistd.h>
#include <iostream>
#include <QStringList>
#include <QApplication>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QThread>

#define COO_SESSION_PORT 51566
#define COO_HARD_PIN "515616"

#define COO_WEB_PORT 51568

int main(int argc, char** argv)
{
    ExtenMessageHandler msg_cb([](int32_t mask, const picojson::value &json_value, std::string *res_msg) -> bool {
        std::cout << "Main >> " << mask <<" msg_cb, json_msg: " << json_value << std::endl;
        return false;
    });
    auto sessionManager = new SessionManager();
    sessionManager->setSessionExtCallback(msg_cb);
    sessionManager->updatePin(COO_HARD_PIN);
    sessionManager->sessionListen(COO_SESSION_PORT);

    // 获取下载目录
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    sessionManager->setStorageRoot(downloadDir);
    sessionManager->updateSaveFolder("launcher");

    std::cout << "start listen........" << COO_SESSION_PORT << std::endl;

    qputenv("QT_LOGGING_RULES", "cooperation-launcher.debug=true");

#if 0
    QString ip = "10.8.11.52";
    std::cout << "connect remote " << ip.toStdString() << std::endl;
    sessionManager->sessionPing(ip, COO_SESSION_PORT);
    QThread::msleep(100);
    std::cout << "sessionConnect..........." << std::endl;
    sessionManager->sessionConnect(ip, COO_SESSION_PORT, COO_HARD_PIN);
    QThread::msleep(100);
    std::cout << "sending file.........." << ip.toStdString() << std::endl;
    QStringList fileList;
    fileList << "/home/zero1/Downloads/ss-1.bin";
    sessionManager->sendFiles(ip, COO_WEB_PORT, fileList);
#endif
    QApplication app(argc, argv);
    app.exec();

    std::cout << "Done!" << std::endl;

    return 0;
}
