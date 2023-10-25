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
#include "co/log.h"
#include "utils.h"

#include <QSettings>

#define KEY_HOSTUUID "hostuuid"
#define KEY_NICKNAME "nickname"
#define KEY_MODE "privacymode"
#define KEY_AUTHPIN "authpin"

class DaemonConfig
{
public:
    DaemonConfig() {
        _fileConfig = new QSettings(Util::configPath(), QSettings::IniFormat);
    }
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

    // 所有值都设置为string类型
    void setAppConfig(fastring appname, fastring key, fastring value)
    {
        QString groupname(appname.c_str());
        _fileConfig->beginGroup(groupname);
        _fileConfig->setValue(key.c_str(), value.c_str());
        _fileConfig->endGroup();
    }

    // 所有值都返回为string类型，外部再转换
    fastring getAppConfig(fastring appname, fastring key)
    {
        QString groupname(appname.c_str());
        fastring value = "";
        _fileConfig->beginGroup(groupname);
        value = _fileConfig->value(key.c_str(), "").toString().toStdString();
        _fileConfig->endGroup();

        return value;
    }

    void initPin()
    {
        fastring pin = _fileConfig->value(KEY_AUTHPIN).toString().toStdString();
        if (pin.empty()) {
            refreshPin();
        } else {
            _pinCode = pin;
        }
    }

    const fastring getPin()
    {
        return _pinCode;
    }

    void setPin(fastring pin)
    {
        _pinCode = pin;
        _fileConfig->setValue(KEY_AUTHPIN, _pinCode.c_str());
    }

    const fastring refreshPin()
    {
        _pinCode = Util::genRandPin();
        _fileConfig->setValue(KEY_AUTHPIN, _pinCode.c_str());

        return _pinCode;
    }

    const fastring getUUID() {
        QString uuid = _fileConfig->value(KEY_HOSTUUID).toString();
        return uuid.toStdString();
    }

    void setUUID(const char *name) {
        _fileConfig->setValue(KEY_HOSTUUID, name);
    }

    const fastring getNickName() {
        QString nick = _fileConfig->value(KEY_NICKNAME).toString();
        return nick.toStdString();
    }

    void setNickName(const char *name) {
        _fileConfig->setValue(KEY_NICKNAME, name);
    }

    int getMode() {
        return _fileConfig->value(KEY_MODE).toInt();
    }

    void setMode(int mode) {
        _fileConfig->setValue(KEY_MODE, mode);
    }

    void saveRemoteSession(fastring session)
    {
        _remote_sessionId = session;
    }

    fastring getRemoteSession()
    {
        return _remote_sessionId;
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

    void setStatus(int s)
    {
        status = s;
    }

private:
    fastring _pinCode;
    fastring _remote_sessionId;
    fastring _authedToken;
    fastring _storageDir;
    fastring _targetName;
    int status = 0;

    QSettings *_fileConfig;
};

enum status {
    ready = 0,
    connected = 1,
    transferring = 2,
    result = 3,
};

#endif
