// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vncviewer.h"
#include "qt2keysum.h"

#include <QDebug>
#include <QElapsedTimer>

using namespace cooperation_core;

enum Operation {
    BACK = 0,
    HOME,
    RECENTS
};

VncViewer::VncViewer(QWidget *parent)
    : QWidget(parent),
      m_surfacePixmap(-1, -1),
      m_scale(1.0),
      m_scaled(true),
      m_buttonMask(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_frameTimer = new QTimer(this);
    m_frameTimer->setTimerType(Qt::CoarseTimer);
    m_frameTimer->setInterval(500);

    connect(m_frameTimer, SIGNAL(timeout()), this, SLOT(frameTimerTimeout()));
}

VncViewer::~VncViewer()
{
}

void VncViewer::setServes(const std::string &ip, int port, const std::string &pwd)
{
    m_serverIp = ip;
    m_serverPort = port;
    m_serverPwd = pwd;
}

void VncViewer::frameTimerTimeout()
{
    setCurrentFps(frameCounter());
    setFrameCounter(0);

#ifdef QT_DEBUG
    qWarning() << " FPS: " << currentFps();
#endif

    if (!m_connected)
        return;

    // Check the screen has been rotated
    const QSize screenSize = {m_rfbCli->width, m_rfbCli->height};
    if (screenSize != m_originalSize) {
        emit sizeChanged(screenSize);
        m_originalSize = screenSize;

        setSurfaceSize(m_originalSize);
    }
}

void VncViewer::onShortcutAction(int action)
{
    if (!m_connected)
        return;

    int key = 0;
    switch (action) {
    case BACK:
        key = Qt::Key_Escape;
        break;
    case HOME:
        key = Qt::Key_Home;
        break;
    case RECENTS:
        key = Qt::Key_PageUp;
        break;
    }

    if (key > 0) {
        SendKeyEvent(m_rfbCli, qt2keysym(key), true);
    }
}

void VncViewer::updateSurface()
{
    resizeEvent(0);
    update();
}

void VncViewer::clearSurface()
{
    setCurrentFps(0);
    setFrameCounter(0);

    if (m_surfacePixmap.isNull() && m_connected)
        setSurfaceSize({m_rfbCli->width, m_rfbCli->height});
    else
        setSurfaceSize(m_surfacePixmap.size());
}

void VncViewer::finishedFramebufferUpdateStatic(rfbClient *cl)
{
    VncViewer *ptr = static_cast<VncViewer *>(rfbClientGetClientData(cl, nullptr));
    ptr->finishedFramebufferUpdate(cl);
}

int VncViewer::translateMouseButton(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton:   return rfbButton1Mask;
    case Qt::MiddleButton: return rfbButton2Mask;
    case Qt::RightButton:  return rfbButton3Mask;
    default:               return 0;
    }
}

void VncViewer::finishedFramebufferUpdate(rfbClient *cl)
{
    m_image = QImage(cl->frameBuffer, cl->width, cl->height, QImage::Format_RGBA8888);

    update();
}

// std::thread* VncViewer::vncThread()
// {
//     return m_vncThread;
// }

void VncViewer::paintEvent(QPaintEvent *event)
{
    if (m_connected) {
        // 创建一个后台缓冲区
        QPixmap bufferPixmap(m_surfacePixmap.size());
        bufferPixmap.fill(Qt::transparent); // 清空背景

        // 在缓冲区中绘制图像
        QPainter bufferPainter(&bufferPixmap);
        if (!m_image.isNull()) {
            bufferPainter.drawImage(0, 0, m_image); // 绘制当前图像
        }

        m_painter.begin(this);
        m_painter.setRenderHints(QPainter::SmoothPixmapTransform);
        m_painter.fillRect(rect(), m_backgroundBrush);
        if (scaled()) {
            m_surfaceRect.moveCenter(rect().center());
            m_painter.scale(m_scale, m_scale);
            m_painter.drawPixmap(m_surfaceRect.x() / m_scale, m_surfaceRect.y() / m_scale, bufferPixmap);
        } else {
            m_painter.scale(1.0, 1.0);
            m_painter.drawPixmap((width() - bufferPixmap.width()) / 2, (height() - bufferPixmap.height()) / 2, bufferPixmap);
        }
        m_painter.end();
    } else {
        m_painter.begin(this);
        m_painter.fillRect(rect(), backgroundBrush());
        m_painter.setPen(Qt::black);
        m_painter.setFont(font());
        QRect m_textBoundingRect = m_painter.boundingRect(rect(), Qt::AlignHCenter | Qt::AlignVCenter, tr("Disconnected"));
        m_textBoundingRect.moveCenter(rect().center());
        m_painter.drawText(m_textBoundingRect, tr("Disconnected"));
        m_painter.end();
    }

    incFrameCounter();
}

void VncViewer::setSurfaceSize(QSize surfaceSize)
{
    m_surfacePixmap = QPixmap(surfaceSize);
    m_surfacePixmap.fill(backgroundBrush().color());
    m_surfaceRect = m_surfacePixmap.rect();
    m_surfaceRect.setWidth(m_surfaceRect.width() * m_scale);
    m_surfaceRect.setHeight(m_surfaceRect.height() * m_scale);
    m_transform = QTransform::fromScale(1.0 / m_scale, 1.0 / m_scale);

    QTimer::singleShot(0, this, SLOT(updateSurface()));
}

void VncViewer::resizeEvent(QResizeEvent *e)
{
    if (!m_surfacePixmap.isNull()) {
        if (scaled()) {
            // 计算保持比例的缩放因子
            qreal widthScale = (qreal)width() / (qreal)m_surfacePixmap.width();
            qreal heightScale = (qreal)height() / (qreal)m_surfacePixmap.height();

            // 选择较小的缩放因子以保持比例
            m_scale = qMin(widthScale, heightScale);
        }

        m_surfaceRect = m_surfacePixmap.rect();
        m_surfaceRect.setWidth(m_surfaceRect.width() * m_scale);
        m_surfaceRect.setHeight(m_surfaceRect.height() * m_scale);
        m_surfaceRect.moveCenter(rect().center());
        m_transform = QTransform::fromScale(1.0 / m_scale, 1.0 / m_scale);
        m_transform.translate(-m_surfaceRect.x(), -m_surfaceRect.y());
    }
    if (e)
        QWidget::resizeEvent(e);
}

bool VncViewer::event(QEvent *e)
{
    if (m_connected) {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            m_surfaceRect.moveCenter(rect().center()); // fall through!
        case QEvent::MouseMove: {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
            QPoint mappedPos = m_transform.map(mouseEvent->pos());
            int button = translateMouseButton(mouseEvent->button());

            switch (e->type()) {
            case QEvent::MouseButtonPress:
                m_buttonMask |= button;
                break;

            case QEvent::MouseButtonRelease:
                m_buttonMask &= ~button;
                break;

            case QEvent::MouseButtonDblClick:
                SendPointerEvent(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask | button);
                SendPointerEvent(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask);
                SendPointerEvent(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask | button);
                break;

            default:
                break;
            }

            SendPointerEvent(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask);
            break;
        }

        case QEvent::Wheel: // 处理滚轮事件
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(e);
            QPoint mappedPos = m_transform.map(wheelEvent->pos());

            // 定义一个处理滚轮的辅助函数
            auto processWheelEvent = [&](int delta, int positiveButtonMask, int negativeButtonMask) {
                if (delta != 0) {
                    int steps = delta / 120; // 每120个单位视为一步
                    int buttonMask = (steps > 0) ? positiveButtonMask : negativeButtonMask;
                    for (int i = 0; i < abs(steps); ++i) {
                        SendPointerEvent(m_rfbCli, mappedPos.x(), mappedPos.y(), buttonMask);
                        SendPointerEvent(m_rfbCli, mappedPos.x(), mappedPos.y(), 0); // 释放按钮
                    }
                }
            };

            // 处理纵向和横向滚轮事件
            processWheelEvent(wheelEvent->angleDelta().y(), rfbButton4Mask, rfbButton5Mask);
            processWheelEvent(wheelEvent->angleDelta().x(), 0b01000000, 0b00100000);

            break;
        }

        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            SendKeyEvent(m_rfbCli, qt2keysym(keyEvent->key()), e->type() == QEvent::KeyPress);
            if (keyEvent->key() == Qt::Key_Alt)
                setFocus(); // avoid losing focus
            return true; // prevent futher processing of event
        }

        default:
            break;
        }
    }

    return QWidget::event(e);
}

void VncViewer::mousePressEvent(QMouseEvent *event)
{
    event->accept();
}

void VncViewer::mouseMoveEvent(QMouseEvent *event)
{
    event->accept();
}

void VncViewer::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
}

void VncViewer::start()
{
    m_rfbCli = rfbGetClient(8, 3, 4);
    m_rfbCli->format.depth = 32;
    m_rfbCli->FinishedFrameBufferUpdate = finishedFramebufferUpdateStatic;
    m_rfbCli->serverHost = strdup(m_serverIp.c_str());
    m_rfbCli->serverPort = m_serverPort;
    // m_rfbCli->appData.compressLevel = 1;
    m_rfbCli->appData.qualityLevel = 9;
    m_rfbCli->appData.scaleSetting = 1;
    m_rfbCli->appData.forceTrueColour = TRUE;
    m_rfbCli->appData.useRemoteCursor = FALSE;
    m_rfbCli->appData.encodingsString = "tight ultra";

    rfbClientSetClientData(m_rfbCli, nullptr, this);

    if (rfbInitClient(m_rfbCli, 0, nullptr)) {
        m_connected = true;
    } else {
        std::cout << "[INFO] disconnected" << std::endl;
        m_connected = false;
        return;
    }

    std::cout << "[INFO] vnc screen: " << m_rfbCli->width << " x " << m_rfbCli->height << std::endl;
#ifdef HIDED_CURSOR
    QPixmap pixmap(1, 1);
    pixmap.fill(Qt::transparent);
    setCursor(QCursor(pixmap));
#endif
    // 启动帧率计时器
    m_frameTimer->start();
    m_stop = false;

    // record the first mobile display size
    m_originalSize = {m_rfbCli->width, m_rfbCli->height};
    emit sizeChanged(m_originalSize);

    setSurfaceSize(m_originalSize);

    m_vncThread = new std::thread([this]() {
        while (!m_stop) {
            int i = WaitForMessage(m_rfbCli, 500);
            if (i < 0) {
                std::cout << "[INFO] disconnected" << std::endl;
                m_connected = false;
                rfbClientCleanup(m_rfbCli);
                break;
            }

            if (i && !HandleRFBServerMessage(m_rfbCli)) {
                std::cout << "[INFO] disconnected" << std::endl;
                m_connected = false;
                rfbClientCleanup(m_rfbCli);
                break;
            }
        };
    });
}


void VncViewer::stop()
{
    m_stop = true;
    m_frameTimer->stop();
    m_vncThread->join();
    if (m_connected) {
        m_connected = false;
        rfbClientCleanup(m_rfbCli);
    }
}
