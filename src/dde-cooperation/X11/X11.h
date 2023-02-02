// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef X11_X11_H
#define X11_X11_H

#include <memory>

#include <xcb/xcb.h>

#include <QObject>
#include <QSocketNotifier>

#define XCB_REPLY_CONNECTION_ARG(connection, ...) connection
#define XCB_REPLY(call, ...)                                                                       \
    std::unique_ptr<call##_reply_t>(                                                               \
        call##_reply(XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr))

class QSocketNotifier;

namespace X11 {

class X11 : public QObject {
    Q_OBJECT

public:
    explicit X11(QObject *parent = nullptr);
    virtual ~X11();

    virtual void handleEvent(std::shared_ptr<xcb_generic_event_t> event) = 0;

protected:
    QSocketNotifier *m_socketNotifier;
    xcb_connection_t *m_conn;
    int m_xcbFd;
    const xcb_setup_t *m_setup;
    xcb_screen_t *m_screen;

    xcb_screen_t *screenOfDisplay(int screen);
    void onEvent(QSocketDescriptor socket, QSocketNotifier::Type activationEvent);
};

} // namespace X11

#endif // !X11_X11_H
