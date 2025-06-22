// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vncsendworker.h"
#include "common/log.h"

#include <QTimer>
#include <QDebug>

VNCSendWorker::VNCSendWorker(QObject *parent): QObject(parent)
{
    DLOG << "Initializing VNC send worker";
}

void VNCSendWorker::sendMouseUpdateMsg(rfbClient *cl, int x, int y, int button = 0)
{
    // DLOG << "Sending mouse event - x:" << x << "y:" << y << "button:" << button;
    SendPointerEvent(cl, x, y, button);
    SendIncrementalFramebufferUpdateRequest(cl);
    // DLOG << "Mouse event sent";
}

void VNCSendWorker::sendKeyUpdateMsg(rfbClient *cl, int key, bool down)
{
    // DLOG << "Sending key event - key:" << key << "state:" << (down ? "down" : "up");
    SendKeyEvent(cl, key, down);
    SendIncrementalFramebufferUpdateRequest(cl);
    // DLOG << "Key event sent";
}
