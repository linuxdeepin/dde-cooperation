// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDE_COOPERATION_INPUT_GRABBERS_MANAGER_H
#define DDE_COOPERATION_INPUT_GRABBERS_MANAGER_H

#include <QObject>
#include <QMap>

#include "InputGrabberWrapper.h"

class QTimer;
class Machine;
class QFileSystemWatcher;

class InputGrabbersManager : public QObject {
    Q_OBJECT

public:
    explicit InputGrabbersManager(QObject *parent = nullptr);
    ~InputGrabbersManager() {}

    void stopGrab();
    void startGrabEvents(const std::weak_ptr<Machine> &machine);
    void removeInputGrabber(const QString &path);
    void addInputGrabber(const QString &path);

private:
    void initGrabbers();
    void dirChanged();

private:
    bool m_isGrabbing;
    QTimer *m_timer;
    std::weak_ptr<Machine> m_curMachine;
    QStringList m_files;
    QMap<QString, InputGrabberWrapper *> m_inputGrabbers;
};

#endif // DDE_COOPERATION_INPUT_GRABBERS_MANAGER_H