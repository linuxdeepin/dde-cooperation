// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InputGrabber.h"

#include <filesystem>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <fmt/core.h>

#include <libevdev/libevdev.h>

#include <QDebug>
#include <QCoreApplication>

namespace fs = std::filesystem;

InputGrabber::InputGrabber(const fs::path &path)
    : m_path(path)
    , m_fd(open(m_path.c_str(), O_RDONLY | O_NONBLOCK))
    , m_notifier(new QSocketNotifier(m_fd, QSocketNotifier::Type::Read, this)) {

    int rc = libevdev_new_from_fd(m_fd, &m_dev);
    if (rc < 0) {
        qCritical() << fmt::format("failed to init libevdev {}", strerror(-rc)).data();
        qApp->quit();
    }

    m_name = libevdev_get_name(m_dev);

    if (libevdev_has_event_type(m_dev, EV_KEY) && libevdev_has_event_type(m_dev, EV_REP)) {
        m_type = InputDeviceType::KEYBOARD;
    } else if (libevdev_has_event_type(m_dev, EV_REL) &&
               libevdev_has_event_code(m_dev, EV_KEY, BTN_LEFT)) {
        m_type = InputDeviceType::MOUSE;
    } else if (libevdev_has_event_type(m_dev, EV_ABS) &&
               !libevdev_has_property(m_dev, INPUT_PROP_DIRECT)) {
        m_type = InputDeviceType::TOUCHPAD;
    }

    m_notifier->setEnabled(false);
    connect(m_notifier, &QSocketNotifier::activated, this, &InputGrabber::handlePollEvent);
}

InputGrabber::~InputGrabber() {
    qDebug() << "InputDevice::~InputDevice()" << m_name;
    libevdev_free(m_dev);
    close(m_fd);
}

bool InputGrabber::shouldIgnore() {
    if (m_name.startsWith("DDE Cooperation")) {
        return true;
    }

    // TODO: allow touchpad devices
    if (m_type == InputDeviceType::KEYBOARD || m_type == InputDeviceType::MOUSE ||
        m_type == InputDeviceType::TOUCHPAD) {
        return false;
    }

    return true;
}

void InputGrabber::start() {
    // clear already existed events
    input_event ev;
    while (-EAGAIN != libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev))
        ;

    m_notifier->setEnabled(true);

    int rc = libevdev_grab(m_dev, LIBEVDEV_GRAB);
    if (rc != 0) {
        qWarning() << fmt::format("failed to grab device {}", strerror(-rc)).data();
        return;
    }
}

void InputGrabber::stop() {
    qDebug() << "stopping input device:" << m_name;
    m_notifier->setEnabled(false);
    libevdev_grab(m_dev, LIBEVDEV_UNGRAB);
    qDebug("stopped");
}

void InputGrabber::handlePollEvent([[maybe_unused]] QSocketDescriptor socket,
                                   [[maybe_unused]] QSocketNotifier::Type type) {
    input_event ev;
    int rc;
    do {
        rc = libevdev_next_event(m_dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
            qDebug() << fmt::format("event: {}, {}, {}",
                                    libevdev_event_type_get_name(ev.type),
                                    libevdev_event_code_get_name(ev.type, ev.code),
                                    ev.value)
                            .data();

            m_onEvent(ev.type, ev.code, ev.value);
        }
    } while (rc != -EAGAIN);
}
