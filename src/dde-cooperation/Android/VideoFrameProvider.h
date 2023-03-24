// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef ANDROID_VIDEOFRAMEPROVIDER_H
#define ANDROID_VIDEOFRAMEPROVIDER_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include "QtScrcpyCore.h"

class VideoFrameProvider : public QObject, public qsc::DeviceObserver {
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)

public:
    VideoFrameProvider(QObject *parent = nullptr);
    ~VideoFrameProvider();

    QAbstractVideoSurface *videoSurface();
    void setVideoSurface(QAbstractVideoSurface *surface);

signals:
    /**
     * @brief newVideoFrame 有新的视频帧
     * @param frame 视频帧
     */
    void newVideoFrame(const QVideoFrame &frame);

private:
    QAbstractVideoSurface *m_surface = nullptr;
    QVideoFrame *m_frame = nullptr;
    QVideoSurfaceFormat m_format;

    virtual void onFrame(const QVideoFrame &frame) override;
};

#endif // ANDROID_VIDEOFRAMEPROVIDER_H
