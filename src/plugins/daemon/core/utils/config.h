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

static fastring _pinCode;
static uint64 _sessionId;
static fastring _authedToken;
static fastring _storageDir;
static fastring _targetName;

class Config {
public:

    static bool needConfirm()
    {
        return false;
    }

    static const fastring getPin()
    {
        return _pinCode;
    }

    static const fastring refreshPin()
    {
        _pinCode = Util::genRandPin();
        return _pinCode;
    }

    static void saveSession(uint64 session)
    {
        _sessionId = session;
    }

    static const uint64 getSession()
    {
        return _sessionId;
    }

    static void saveAuthed(fastring token)
    {
        _authedToken = token;
    }

    static const fastring getAuthed()
    {
        return _authedToken;
    }

    static void setTargetName(const char *name)
    {
        _targetName = fastring(name);
    }

    static const fastring getStorageDir()
    {
        fastring home = os::homedir();
        if (_targetName.empty()) {
            _storageDir = home;
        } else {
            _storageDir = home + "/" + _targetName;
        }
        return _storageDir;
    }

};

#endif
