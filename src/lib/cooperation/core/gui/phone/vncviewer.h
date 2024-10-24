// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNCVIEWER_H
#define VNCVIEWER_H

#include "rfb/rfbclient.h"

#include <iostream>
#include <thread>
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>
#include <QTimer>

namespace cooperation_core {

class VncViewer : public QWidget
{
    Q_OBJECT

public:
    VncViewer(QWidget *parent = nullptr);
    virtual ~VncViewer();

    void setServes(const std::string &ip, int port, const std::string &pwd);
    void start();
    void stop();

    std::thread *vncThread() const;
    void paintEvent(QPaintEvent *event) override;

    void setBackgroundBrush(QBrush brush) { m_backgroundBrush = brush; }
    QBrush backgroundBrush() { return m_backgroundBrush; }
    bool scaled() { return m_scaled; }
    void setScaled(bool scaled) { m_scaled = scaled; }

Q_SIGNALS:
    void sizeChanged(const QSize &size);

public slots:
    void frameTimerTimeout();
    void onShortcutAction(int action);

    void setSurfaceSize(QSize surfaceSize);
    void updateSurface();
    void clearSurface();

protected:
    bool event(QEvent *e);
    void resizeEvent(QResizeEvent *e);

    // capture the mouse move events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    int currentFps() { return m_currentFps; }
    void setCurrentFps(int fps) { m_currentFps = fps; }

    int frameCounter() { return m_frameCounter; }
    void incFrameCounter() { m_frameCounter++; }
    void setFrameCounter(int counter) { m_frameCounter = counter; }


private:
    static void finishedFramebufferUpdateStatic(rfbClient *cl);
    static int translateMouseButton(Qt::MouseButton button);

    void finishedFramebufferUpdate(rfbClient *cl);

private:
    std::string m_serverIp;
    std::string m_serverPwd;
    int m_serverPort;

    bool m_stop;
    bool m_connected;
    QImage m_image;
    rfbClient *m_rfbCli { nullptr };
    std::thread *m_vncThread;
    QPainter m_painter;

    QBrush m_backgroundBrush;
    QPixmap m_surfacePixmap;
    QRect m_surfaceRect;
    QTransform m_transform;
    int m_buttonMask;
    bool m_scaled;
    qreal m_scale;
    QSize m_originalSize; // 记录原始大小

    QTimer *m_frameTimer;
    uint m_frameCounter;
    uint m_currentFps;
};

}   // namespace cooperation_core

#endif   // VNCVIEWER_H
