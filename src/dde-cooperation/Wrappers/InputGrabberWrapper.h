// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WRAPPERS_INPUTGRABBERWRAPPER_H
#define WRAPPERS_INPUTGRABBERWRAPPER_H

#include <filesystem>

#include <QObject>
#include <QProcess>

class QLocalServer;
class QLocalSocket;
class QProcess;

class InputGrabbersManager;
class Machine;

class InputGrabberWrapper : public QObject {
    Q_OBJECT

public:
    explicit InputGrabberWrapper(InputGrabbersManager *manager, const QString &path);
    ~InputGrabberWrapper();
    void setMachine(const std::weak_ptr<Machine> &machine);
    void start();
    void stop();

private slots:
    void onProcessClosed(int exitCode, QProcess::ExitStatus exitStatus);
    void handleNewConnection();
    void onReceived();
    void onDisconnected();

private:
    InputGrabbersManager *m_manager;
    QLocalServer *m_server;
    QLocalSocket *m_conn;
    QProcess *m_process;

    const QString m_path;

    uint8_t m_type;
    std::weak_ptr<Machine> m_machine;
};

#endif // !WRAPPERS_INPUTGRABBERWRAPPER_H
