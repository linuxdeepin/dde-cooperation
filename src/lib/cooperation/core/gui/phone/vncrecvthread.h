// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNCRECVTHREAD_H
#define VNCRECVTHREAD_H

#include <QThread>

#include <QImage>
#include <QAtomicInt>

#include "rfb/rfbclient.h"

class VNCRecvThread : public QThread
{
    Q_OBJECT
public:
    VNCRecvThread(QObject *parent = nullptr);

    void startRun(rfbClient *cl);
    void stopRun();
    void markFrameProcessed();

    static void frameBufferUpdated(rfbClient *cl);
    static void screenSizeChanged(rfbClient *cl, int width, int height);

signals:
    void updateImageSignal(QImage);
    void sizeChangedSignal(int width, int height);

protected:
    void run() override;

private:
    bool _runFlag = false;
    rfbClient *_cl;
    QAtomicInt _framePending { 0 };

    static bool _skipFirst; // skip the first image
};

#endif // VNCRECVTHREAD_H
