// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNCRECVTHREAD_H
#define VNCRECVTHREAD_H

#include <QThread>

#include <QImage>

#include "rfb/rfbclient.h"

class VNCRecvThread : public QThread
{
    Q_OBJECT
public:
    VNCRecvThread(QObject *parent = nullptr);

    void startRun(rfbClient *cl);
    void stopRun();

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

    static bool _skipFirst; // skip the first image
};

#endif // VNCRECVTHREAD_H
