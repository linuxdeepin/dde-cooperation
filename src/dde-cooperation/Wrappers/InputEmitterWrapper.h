// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WRAPPERS_INPUTEMITTERWRAPPER_H
#define WRAPPERS_INPUTEMITTERWRAPPER_H

#include <filesystem>

#include <QObject>
#include <QProcess>

#include "common.h"

class QLocalServer;
class QLocalSocket;
class QProcess;

class Manager;
class Machine;

class InputEmitterWrapper : public QObject {
    Q_OBJECT

public:
    explicit InputEmitterWrapper(InputDeviceType type);
    ~InputEmitterWrapper();
    void setMachine(const std::weak_ptr<Machine> &machine);
    bool emitEvent(unsigned int type, unsigned int code, int value) noexcept;

private slots:
    void onProcessClosed(int exitCode, QProcess::ExitStatus exitStatus);
    void handleNewConnection();
    void onReceived();
    void onDisconnected();

private:
    QLocalServer *m_server;
    QLocalSocket *m_conn;
    QProcess *m_process;

    InputDeviceType m_type;
};

#endif // !WRAPPERS_INPUTEMITTERWRAPPER_H
