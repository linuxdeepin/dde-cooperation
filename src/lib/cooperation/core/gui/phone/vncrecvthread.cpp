// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vncrecvthread.h"

bool VNCRecvThread::_skipFirst = false;

VNCRecvThread::VNCRecvThread(QObject *parent): QThread(parent)
{

}

void VNCRecvThread::startRun(rfbClient *cl)
{
    if (_runFlag)
        return;

    _cl = cl;
    _cl->FinishedFrameBufferUpdate = frameBufferUpdated;
    _cl->ScreenSizeChanged = screenSizeChanged;
    rfbClientSetClientData(_cl, nullptr, this);
    _runFlag = true;

    this->start();
}

void VNCRecvThread::stopRun()
{
    if (!_runFlag)
        return;

    _runFlag = false;
    _skipFirst = false;
    if (_cl) {
        rfbClientSetClientData(_cl, nullptr, nullptr);
        _cl->FinishedFrameBufferUpdate = nullptr;
        _cl->ScreenSizeChanged = nullptr;
    }
}


void VNCRecvThread::run()
{
    while (_runFlag && _cl) {
        int i = WaitForMessage(_cl, 500);
        if (i < 0) {
            break;
        }

        if (i && !HandleRFBServerMessage(_cl)) {
            break;
        }
    };
}

void VNCRecvThread::frameBufferUpdated(rfbClient *cl)
{
    if (!_skipFirst) {
        // skip the first image buffer which may be incomplete
        _skipFirst = true;
        return;
    }
    VNCRecvThread *vncRecvThread = static_cast<VNCRecvThread *>(rfbClientGetClientData(cl, nullptr));

    emit vncRecvThread->updateImageSignal(QImage(cl->frameBuffer, cl->width, cl->height, QImage::Format_RGBA8888));
}

void VNCRecvThread::screenSizeChanged(rfbClient *cl, int width, int height)
{
    VNCRecvThread *vncRecvThread = static_cast<VNCRecvThread *>(rfbClientGetClientData(cl, nullptr));

    emit vncRecvThread->sizeChangedSignal(width, height);
}
