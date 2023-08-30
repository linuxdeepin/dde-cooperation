// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "X11.h"

#include <stdexcept>

#include <cstdlib>
#include <cstdio>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xproto.h>
#include <xcb/xfixes.h>
#include <xcb/xinput.h>

#include <fmt/core.h>

#include <QDebug>

namespace X11 {

X11::X11(QObject *parent)
    : QObject(parent) {
    int screenDefaultNbr;
    m_conn = xcb_connect(nullptr, &screenDefaultNbr);

    if (int err = xcb_connection_has_error(m_conn)) {
        throw std::runtime_error(fmt::format("failed to connect to X11: {}", err));
    }

    m_xcbFd = xcb_get_file_descriptor(m_conn);
    qInfo() << fmt::format("xcb fd: {}", m_xcbFd).data();
    m_socketNotifier = new QSocketNotifier(m_xcbFd, QSocketNotifier::Type::Read, this);
    connect(m_socketNotifier, &QSocketNotifier::activated, this, &X11::onEvent);

    m_setup = xcb_get_setup(m_conn);
    m_screen = screenOfDisplay(screenDefaultNbr);
}

X11::~X11() {
    xcb_disconnect(m_conn);
}

xcb_screen_t *X11::screenOfDisplay(int screen) {
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(m_setup);
    for (; iter.rem; --screen, xcb_screen_next(&iter)) {
        if (screen == 0) {
            return iter.data;
        }
    }

    return nullptr;
}

void X11::onEvent() {
    std::shared_ptr<xcb_generic_event_t> event;
    while (event.reset(xcb_poll_for_event(m_conn)), event) {
        handleEvent(event);
    }
}

} // namespace X11
