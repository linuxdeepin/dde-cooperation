// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MACHINE_ANDROIDMACHINE_H
#define MACHINE_ANDROIDMACHINE_H

#include "Machine.h"

class AndroidMachineDBusAdaptor;
class AndroidMainWindow;
class SendTransfer;

class AndroidMachine : public Machine {
    Q_OBJECT

public:
    AndroidMachine(Manager *manager,
                   ClipboardBase *clipboard,
                   QDBusConnection service,
                   uint32_t id,
                   const std::filesystem::path &dataDir,
                   const std::string &ip,
                   uint16_t port,
                   const DeviceInfo &sp);

public:
    void startCast();

protected:
    virtual void handleConnected() override;
    virtual void handleDisconnected() override;
    virtual void handleCastRequest(const CastRequest &req) override;
    virtual void sendFiles(const QStringList &filePaths) override;

private:
    AndroidMachineDBusAdaptor *m_dbusAdaptorAndroid;
    QPointer<AndroidMainWindow> m_mainWindow;
};

#endif // !MACHINE_ANDROIDMACHINE_H
