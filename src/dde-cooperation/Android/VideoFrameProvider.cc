// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VideoFrameProvider.h"

#include <QFile>
#include <QFileInfo>
#include <QSharedPointer>
#include <QTimer>

class VideoFrameProviderPrivate {
public:
    QString m_videoUrl;
    QSharedPointer<QVideoFrame> m_frame = nullptr;

    /**
     * 以下代码仅供示例使用
     * 实际上，此处应放你的视频源，它可以来自你自己的解码器
     */
    QTimer *m_testTimer = nullptr;
};

VideoFrameProvider::VideoFrameProvider(QObject *parent)
    : QObject(parent) {
}

VideoFrameProvider::~VideoFrameProvider() {
}

QAbstractVideoSurface *VideoFrameProvider::videoSurface() {
    return m_surface;
}

void VideoFrameProvider::setVideoSurface(QAbstractVideoSurface *surface) {
    if (m_surface && m_surface != surface && m_surface->isActive()) {
        m_surface->stop();
    }

    m_surface = surface;

    if (m_surface && m_format.isValid()) {
        m_format = m_surface->nearestFormat(m_format);
        m_surface->start(m_format);
    }
}

void VideoFrameProvider::onFrame(const QVideoFrame &frame) {
    if (m_surface) {
        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat());
        m_format = m_surface->nearestFormat(format);
        if (m_surface->surfaceFormat() != m_format) {
            m_surface->stop();
        }

        if (!m_surface->isActive()) {
            m_surface->start(m_format);
        }

        m_surface->present(frame);
    }
}
