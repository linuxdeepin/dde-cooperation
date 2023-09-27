// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include <stdint.h>
#include <string.h>
#include "co/byte_order.h"
#include "co/rand.h"
#include "co/os.h"
#include "utils.h"

class DaemonConfig
{
public:
    DaemonConfig() {}
    ~DaemonConfig() {}

    static DaemonConfig *instance()
    {
        static DaemonConfig ins;
        return &ins;
    }

    bool needConfirm()
    {
        return false;
    }

    const fastring getPin()
    {
        return _pinCode;
    }

    const fastring refreshPin()
    {
        _pinCode = Util::genRandPin();
        return _pinCode;
    }

    void saveSession(uint64 session)
    {
        _sessionId = session;
    }

    const uint64 getSession()
    {
        return _sessionId;
    }

    void saveAuthed(fastring token)
    {
        _authedToken = token;
    }

    const fastring getAuthed()
    {
        return _authedToken;
    }

    void setTargetName(const char *name)
    {
        _targetName = fastring(name);
    }

    const fastring getStorageDir()
    {
        fastring home = os::homedir();
        if (_targetName.empty()) {
            _storageDir = home;
        } else {
            _storageDir = home + "/" + _targetName;
        }
        return _storageDir;
    }

    const int getStatus()
    {
        return status;
    }

private:
    fastring _pinCode;
    uint64 _sessionId;
    fastring _authedToken;
    fastring _storageDir;
    fastring _targetName;
    int status = 0;
};

enum status {
    ready = 0,
    connected = 1,
    transferring = 2,
    result = 3,
};

#endif
