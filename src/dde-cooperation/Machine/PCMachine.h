// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MACHINE_PCMACHINE_H
#define MACHINE_PCMACHINE_H

#include "Machine.h"

class PCMachine : public Machine {
    Q_OBJECT

public:
    PCMachine(Manager *manager,
              ClipboardBase *clipboard,
              QDBusConnection service,
              uint32_t id,
              const std::filesystem::path &dataDir,
              const std::string &ip,
              uint16_t port,
              const DeviceInfo &sp);

protected:
    virtual void handleConnected() override;
    virtual void handleDisconnected() override;
    virtual void handleCastRequest([[maybe_unused]] const CastRequest &req) override {}
    virtual void sendFiles(const QStringList &filePaths) override;

private:
    void mountFs(const std::string &path);
};

#endif // !MACHINE_PCMACHINE_H
