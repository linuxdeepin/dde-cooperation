// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DeviceProxy.h"

#include "QtScrcpyCore.h"

DeviceProxy::DeviceProxy(QObject *parent)
    : QObject(parent)
    , m_screenOpen(true) {
}

void DeviceProxy::setDevice(qsc::IDevice *device) {
    m_device = device;
}

void DeviceProxy::onKeyPressed(int key,
                               int modifiers,
                               const QString &text,
                               bool autorep,
                               ushort count) {
    QKeyEvent event(QKeyEvent::KeyPress,
                    key,
                    static_cast<Qt::KeyboardModifiers>(modifiers),
                    text,
                    autorep,
                    count);
    m_device->keyEvent(&event, QSize{}, QSize{});
}

void DeviceProxy::onKeyReleased(int key,
                                int modifiers,
                                const QString &text,
                                bool autorep,
                                ushort count) {
    QKeyEvent event(QKeyEvent::KeyRelease,
                    key,
                    static_cast<Qt::KeyboardModifiers>(modifiers),
                    text,
                    autorep,
                    count);
    m_device->keyEvent(&event, QSize{}, QSize{});
}

void DeviceProxy::onMouseMove(qreal x,
                              qreal y,
                              int button,
                              int buttons,
                              int modifiers,
                              const QSizeF &frameSize,
                              const QSizeF &showSize) {
    QMouseEvent event(QEvent::MouseMove,
                      QPointF{x, y},
                      static_cast<Qt::MouseButton>(button),
                      static_cast<Qt::MouseButtons>(buttons),
                      static_cast<Qt::KeyboardModifiers>(modifiers));
    m_device->mouseEvent(&event, frameSize.toSize(), showSize.toSize());
}

void DeviceProxy::onMouseButtonPressed(qreal x,
                                       qreal y,
                                       int button,
                                       int buttons,
                                       int modifiers,
                                       const QSizeF &frameSize,
                                       const QSizeF &showSize) {
    QMouseEvent event(QEvent::MouseButtonPress,
                      QPointF{x, y},
                      static_cast<Qt::MouseButton>(button),
                      static_cast<Qt::MouseButtons>(buttons),
                      static_cast<Qt::KeyboardModifiers>(modifiers));
    m_device->mouseEvent(&event, frameSize.toSize(), showSize.toSize());
}

void DeviceProxy::onMouseButtonReleased(qreal x,
                                        qreal y,
                                        int button,
                                        int buttons,
                                        int modifiers,
                                        const QSizeF &frameSize,
                                        const QSizeF &showSize) {
    QMouseEvent event(QEvent::MouseButtonRelease,
                      QPointF{x, y},
                      static_cast<Qt::MouseButton>(button),
                      static_cast<Qt::MouseButtons>(buttons),
                      static_cast<Qt::KeyboardModifiers>(modifiers));
    m_device->mouseEvent(&event, frameSize.toSize(), showSize.toSize());
}

void DeviceProxy::onBackButtonClicked() {
    m_device->postGoBack();
}

void DeviceProxy::onHomeButtonClicked() {
    m_device->postGoHome();
}

void DeviceProxy::onOverviewButtonClicked() {
    m_device->postAppSwitch();
}

void DeviceProxy::onSwitchScreenButtonClicked() {
    m_screenOpen = !m_screenOpen;
    m_device->setScreenPowerMode(m_screenOpen);
}

void DeviceProxy::setClipboard() {
    m_device->setDeviceClipboard();
}

void DeviceProxy::getClipboard() {
    m_device->requestDeviceClipboard();
}
