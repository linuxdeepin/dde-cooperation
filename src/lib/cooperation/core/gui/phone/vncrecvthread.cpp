// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vncrecvthread.h"
#include "common/log.h"

bool VNCRecvThread::_skipFirst = false;

VNCRecvThread::VNCRecvThread(QObject *parent): QThread(parent)
{
    DLOG << "Initializing VNC receive thread";
}

void VNCRecvThread::startRun(rfbClient *cl)
{
    DLOG << "Entering startRun function";
    if (_runFlag) {
        DLOG << "Thread already running, skipping start";
        return;
    }

    DLOG << "Starting VNC receive thread";
    _cl = cl;
    _cl->FinishedFrameBufferUpdate = frameBufferUpdated;
    // _cl->ScreenSizeChanged = screenSizeChanged;
    rfbClientSetClientData(_cl, nullptr, this);
    _runFlag = true;

    this->start();
    DLOG << "Thread started successfully";
}

void VNCRecvThread::stopRun()
{
    DLOG << "Entering stopRun function";
    if (!_runFlag) {
        DLOG << "Thread not running, skipping stop";
        return;
    }

    DLOG << "Stopping VNC receive thread";
    _runFlag = false;
    _skipFirst = false;
    if (_cl) {
        DLOG << "Cleaning up client resources";
        rfbClientSetClientData(_cl, nullptr, nullptr);
        _cl->FinishedFrameBufferUpdate = nullptr;
        // _cl->ScreenSizeChanged = nullptr;
        DLOG << "Client resources cleaned up";
    }
    DLOG << "Thread stopped";
}


void VNCRecvThread::run()
{
    DLOG << "Thread running, waiting for messages";
    while (_runFlag && _cl) {
        int i = WaitForMessage(_cl, 500);
        if (i < 0) {
            DLOG << "Error waiting for message";
            break;
        }

        if (i && !HandleRFBServerMessage(_cl)) {
            DLOG << "Error handling server message";
            break;
        }
    };
    DLOG << "Thread exiting run loop";
}

void VNCRecvThread::frameBufferUpdated(rfbClient *cl)
{
    if (!_skipFirst) {
        // skip the first image buffer which may be incomplete
        DLOG << "Skipping first frame buffer update";
        _skipFirst = true;
        return;
    }
    VNCRecvThread *vncRecvThread = static_cast<VNCRecvThread *>(rfbClientGetClientData(cl, nullptr));

    // DLOG << "Frame buffer updated, emitting image signal";
    emit vncRecvThread->updateImageSignal(QImage(cl->frameBuffer, cl->width, cl->height, QImage::Format_RGBA8888));
}

void VNCRecvThread::screenSizeChanged(rfbClient *cl, int width, int height)
{
    DLOG << "Screen size changed to:" << width << "x" << height;
    VNCRecvThread *vncRecvThread = static_cast<VNCRecvThread *>(rfbClientGetClientData(cl, nullptr));

    emit vncRecvThread->sizeChangedSignal(width, height);
    DLOG << "Size change signal emitted";
}
