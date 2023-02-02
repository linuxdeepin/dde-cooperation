// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PCMachine.h"

PCMachine::PCMachine(Manager *manager,
                     ClipboardBase *clipboard,
                     QDBusConnection service,
                     uint32_t id,
                     const std::filesystem::path &dataDir,
                     const std::string &ip,
                     uint16_t port,
                     const DeviceInfo &sp)
    : Machine(manager, clipboard, service, id, dataDir, ip, port, sp) {
}

void PCMachine::handleConnected() {
    mountFs("/");
}

void PCMachine::handleDisconnected() {
}

void PCMachine::sendFiles(const QStringList &filePaths) {
    if (isLinux()) {
        transferSendFiles(filePaths);
        return;
    }

    for (const QString &filePath : filePaths) {
        Message msg;
        FsSendFileRequest *send = msg.mutable_fssendfilerequest();
        send->set_path(filePath.toStdString());
        sendMessage(msg);
    }
}

void PCMachine::mountFs(const std::string &path) {
    Message msg;
    auto *request = msg.mutable_fsrequest();
    request->set_path(path);
    sendMessage(msg);
}
