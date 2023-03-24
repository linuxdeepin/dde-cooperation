// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INPUTEGRABBER_H
#define INPUTEGRABBER_H

#include <filesystem>
#include <thread>
#include <functional>

#include <libevdev/libevdev.h>

#include <QObject>
#include <QSocketNotifier>

#include "common.h"

class QSocketNotifier;

class Machine;

class InputGrabber : public QObject {
    Q_OBJECT

public:
    explicit InputGrabber(const std::filesystem::path &path);
    ~InputGrabber();

    bool shouldIgnore();

    void onEvent(const std::function<void(unsigned int type, unsigned int code, int value)> &cb) {
        m_onEvent = cb;
    }

    void start();
    void stop();

    InputDeviceType type() { return m_type; };

private:
    const std::filesystem::path &m_path;
    int m_fd;

    QSocketNotifier *m_notifier;

    libevdev *m_dev;

    QString m_name;
    InputDeviceType m_type;

    std::function<void(unsigned int type, unsigned int code, int value)> m_onEvent;

    void handlePollEvent(QSocketDescriptor socket, QSocketNotifier::Type type);
};

#endif // !INPUTEGRABBER_H
