// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vncviewer.h"
#include "qt2keysum.h"
#include "common/qtcompat.h"
#include "common/log.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QMutexLocker>

using namespace cooperation_core;

enum Operation {
    BACK = 0,
    HOME,
    RECENTS
};

enum PhoneMode {
    PORTRAIT= 0,
    LANDSCAPE,
};

VncViewer::VncViewer(QWidget *parent)
    : QWidget(parent),
      m_surfacePixmap(-1, -1),
      m_scale(1.0),
      m_scaled(true),
      m_buttonMask(0)
{
    DLOG << "Initializing VNC viewer";
    //init the background color
    m_backgroundBrush = QBrush(Qt::black);

    setFocusPolicy(Qt::StrongFocus);
#ifdef TOUCH_MODE
    DLOG << "Touch mode enabled";
    setMouseTracking(false);
#else
    DLOG << "Mouse tracking enabled";
    setMouseTracking(true);
#endif

    _vncSendThread = new QThread(this);
    _vncSendWorker = new VNCSendWorker();
    connect(this, &VncViewer::sendMouseState, _vncSendWorker, &VNCSendWorker::sendMouseUpdateMsg);
    connect(this, &VncViewer::sendKeyState, _vncSendWorker, &VNCSendWorker::sendKeyUpdateMsg);
    _vncSendWorker->moveToThread(_vncSendThread);
    DLOG << "Send worker initialized";

    _vncRecvThread = new VNCRecvThread(this);
    connect(_vncRecvThread, &VNCRecvThread::updateImageSignal, this, &VncViewer::updateImage, Qt::BlockingQueuedConnection);
    connect(_vncRecvThread, &VNCRecvThread::sizeChangedSignal, this, &VncViewer::onSizeChange, Qt::BlockingQueuedConnection);
    connect(_vncRecvThread, &VNCRecvThread::finished, this, &VncViewer::stop);
    DLOG << "Receive thread initialized";

    m_frameTimer = new QTimer(this);
    m_frameTimer->setTimerType(Qt::CoarseTimer);
    m_frameTimer->setInterval(1000);

    connect(m_frameTimer, SIGNAL(timeout()), this, SLOT(frameTimerTimeout()));

    DLOG << "Initialization completed";
}

VncViewer::~VncViewer()
{
    DLOG << "Destroying VNC viewer";
}

void VncViewer::setServes(const std::string &ip, int port, const std::string &pwd)
{
    DLOG << "Setting server connection - IP:" << ip.c_str() << "Port:" << port;
    m_serverIp = ip;
    m_serverPort = port;
    m_serverPwd = pwd;
    DLOG << "Server settings updated";
}

void VncViewer::frameTimerTimeout()
{
    setCurrentFps(frameCounter());
    setFrameCounter(0);

#ifdef QT_DEBUG
    DLOG << " FPS: " << currentFps();
#endif
}

void VncViewer::onSizeChange(int width, int height)
{
    if (!m_connected) {
        DLOG << "Size change ignored - not connected";
        return;
    }

    DLOG << "Handling size change: " << width << "x" << height;

    // 使用互斥锁保护尺寸变更
    QMutexLocker locker(&m_mutex);

    // 检测屏幕是否旋转
    int curentMode = (width < height) ? PORTRAIT : LANDSCAPE;
    if (curentMode != m_phoneMode) {
        DLOG << "Screen rotation detected - mode: " << curentMode;
        m_phoneMode = curentMode;
        int w = (m_phoneMode == PORTRAIT) ? m_realSize.width() : m_realSize.height();
        const QSize size = {static_cast<int>(w * m_phoneScale), height};

        // 清除现有图像，防止尺寸不匹配时绘制
        m_image = QImage();

        setSurfaceSize(size);
        emit sizeChanged(size);
    }
}

void VncViewer::onShortcutAction(int action)
{
    DLOG << "Shortcut action triggered: " << action;
    if (!m_connected) {
        DLOG << "Not connected, ignoring shortcut action";
        return;
    }

    int key = 0;
    switch (action) {
    case BACK:
        DLOG << "Action is BACK";
        key = Qt::Key_Escape;
        break;
    case HOME:
        DLOG << "Action is HOME";
        key = Qt::Key_Home;
        break;
    case RECENTS:
        DLOG << "Action is RECENTS";
        key = Qt::Key_PageUp;
        break;
    default:
        DLOG << "Unknown action: " << action;
        break;
    }

    if (key > 0) {
        DLOG << "Sending key event: " << key;
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

    // 添加连接状态与有效性检查
    if (!m_connected) {
        DLOG << "Clearing surface skipped - not connected";
        return;
    }

    // 使用互斥锁保护此关键部分
    QMutexLocker locker(&m_mutex);

    if (m_surfacePixmap.isNull() && m_rfbCli) {
        DLOG << "Setting initial surface size from RFB client";
        setSurfaceSize({m_rfbCli->width, m_rfbCli->height});
    } else if (!m_surfacePixmap.isNull()) {
        DLOG << "Maintaining existing surface size";
        setSurfaceSize(m_surfacePixmap.size());
    } else {
        DLOG << "Surface pixmap is null and no RFB client, cannot set surface size";
    }
}

int VncViewer::translateMouseButton(Qt::MouseButton button)
{
    switch (button) {
    case Qt::LeftButton:
        DLOG << "Translating LeftButton";
        return rfbButton1Mask;
    case Qt::MiddleButton:
        DLOG << "Translating MiddleButton";
        return rfbButton2Mask;
    case Qt::RightButton:
        DLOG << "Translating RightButton";
        return rfbButton3Mask;
    default:
        DLOG << "Unknown button: " << button;
        return 0;
    }
}

void VncViewer::setMobileRealSize(const int w, const int h)
{
    // set the phone real resolution by only once
    m_realSize = w < h ? QSize(w, h) : QSize(h, w);
}

void VncViewer::updateImage(const QImage &image)
{
    // 使用互斥锁保护图像更新
    QMutexLocker locker(&m_mutex);

    // 检查图像有效性
    if (image.isNull()) {
        return;
    }

    // 检测屏幕是否旋转 (根据图像宽高比判断)
    int currentMode = (image.width() < image.height()) ? PORTRAIT : LANDSCAPE;
    if (currentMode != m_phoneMode) {
        DLOG << "Screen rotation detected from image - old mode: " << m_phoneMode << " new mode: " << currentMode;
        m_phoneMode = currentMode;

        // 更新显示尺寸
        int w = (m_phoneMode == PORTRAIT) ? m_realSize.width() : m_realSize.height();
        const QSize size = {static_cast<int>(w * m_phoneScale), image.height()};

        setSurfaceSize(size);
        emit sizeChanged(size);
    }

    m_image = image;

    update();
}

void VncViewer::paintEvent(QPaintEvent *event)
{
    if (m_connected) {
        // 使用互斥锁保护绘制过程
        QMutexLocker locker(&m_mutex);

        // 增加图像有效性检查
        if (m_image.isNull()) {
            m_painter.begin(this);
            m_painter.fillRect(rect(), backgroundBrush());
            m_painter.end();
            incFrameCounter();
            return;
        }

        // 修正：避免直接比较m_image与m_surfacePixmap的尺寸
        // 旋转过程中如果尺寸不匹配或模式变化，避免绘制导致崩溃
        if (m_surfacePixmap.isNull() || 
            ((m_phoneMode == PORTRAIT && m_image.width() > m_image.height()) || 
             (m_phoneMode == LANDSCAPE && m_image.width() < m_image.height()))) {
            // 静默忽略当前帧，保持当前显示不变
            incFrameCounter();
            return;
        }

        m_painter.begin(&m_surfacePixmap);
        m_painter.drawImage(rect().topLeft(), m_image);
        m_painter.end();


        m_painter.begin(this);
        m_painter.setRenderHints(QPainter::SmoothPixmapTransform);
        m_painter.fillRect(rect(), backgroundBrush());
        if (scaled()) {
            DLOG << "Drawing scaled pixmap";
            m_surfaceRect.moveCenter(rect().center());
            m_painter.scale(m_scale, m_scale);
            m_painter.drawPixmap(m_surfaceRect.x() / m_scale, m_surfaceRect.y() / m_scale, m_surfacePixmap);
        } else {
            DLOG << "Drawing unscaled pixmap";
            m_painter.scale(1.0, 1.0);
            m_painter.drawPixmap((width() - m_surfacePixmap.width()) / 2, (height() - m_surfacePixmap.height()) / 2, m_surfacePixmap);
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
    // 安全检查
    if (surfaceSize.width() <= 0 || surfaceSize.height() <= 0) {
        DLOG << "Invalid surface size";
        return;
    }

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
    if (e) {
        // DLOG << "Calling QWidget::resizeEvent";
        QWidget::resizeEvent(e);
    }
}

bool VncViewer::event(QEvent *e)
{
    if (m_connected) {
        // DLOG << "Event received, type: " << e->type();
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            DLOG << "MouseButtonPress event";
        case QEvent::MouseButtonRelease:
            DLOG << "MouseButtonRelease event";
        case QEvent::MouseButtonDblClick:
            DLOG << "MouseButtonDblClick event";
            m_surfaceRect.moveCenter(rect().center()); // fall through!
        case QEvent::MouseMove: {
            DLOG << "MouseMove event";
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
            QPoint mappedPos = m_transform.map(mouseEvent->pos());
            int button = translateMouseButton(mouseEvent->button());

            switch (e->type()) {
            case QEvent::MouseButtonPress:
                DLOG << "Mouse button pressed";
                m_buttonMask |= button;
                break;

            case QEvent::MouseButtonRelease:
                DLOG << "Mouse button released";
                m_buttonMask &= ~button;
                break;

            case QEvent::MouseButtonDblClick:
                DLOG << "Mouse button double clicked";
                emit sendMouseState(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask | button);
                emit sendMouseState(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask);
                emit sendMouseState(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask | button);
                break;

            default:
                DLOG << "Unhandled mouse event type";
                break;
            }

            emit sendMouseState(m_rfbCli, mappedPos.x(), mappedPos.y(), m_buttonMask);
            break;
        }

        case QEvent::Wheel: { // 处理滚轮事件
            DLOG << "Wheel event";
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(e);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QPointF mappedPos = m_transform.map(wheelEvent->position());
#else
            QPoint mappedPos = m_transform.map(wheelEvent->pos());
#endif

            // 定义一个处理滚轮的辅助函数
            auto processWheelEvent = [&](int delta, int positiveButtonMask, int negativeButtonMask) {
                DLOG << "Processing wheel event, delta: " << delta;
                if (delta != 0) {
                    int steps = delta / 120; // 每120个单位视为一步
                    int buttonMask = (steps > 0) ? positiveButtonMask : negativeButtonMask;
                    for (int i = 0; i < abs(steps); ++i) {
                        emit sendMouseState(m_rfbCli, mappedPos.x(), mappedPos.y(), buttonMask);
                        emit sendMouseState(m_rfbCli, mappedPos.x(), mappedPos.y(), 0); // release event
                    }
                }
            };

            // 处理纵向和横向滚轮事件
            processWheelEvent(wheelEvent->angleDelta().y(), rfbButton4Mask, rfbButton5Mask);
            processWheelEvent(wheelEvent->angleDelta().x(), 0b01000000, 0b00100000);

            break;
        }

        case QEvent::KeyPress:
            DLOG << "KeyPress event";
        case QEvent::KeyRelease: {
            DLOG << "KeyRelease event";
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);
            emit sendKeyState(m_rfbCli, qt2keysym(keyEvent->key()), e->type() == QEvent::KeyPress);
            if (keyEvent->key() == Qt::Key_Alt) {
                DLOG << "Alt key pressed, setting focus";
                setFocus(); // avoid losing focus
            }
            return true; // prevent futher processing of event
        }

        default:
            DLOG << "Unhandled event type: " << e->type();
            break;
        }
    } else {
        DLOG << "Not connected, ignoring event";
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

void VncViewer ::closeEvent(QCloseEvent *event)
{
    emit fullWindowCloseSignal();
    QWidget::closeEvent(event);
}

void VncViewer::start()
{
    DLOG << "Starting VNC connection to" << m_serverIp.c_str() << ":" << m_serverPort;
    m_rfbCli = rfbGetClient(8, 3, 4);
    m_rfbCli->format.depth = 32;
    m_rfbCli->serverHost = strdup(m_serverIp.c_str());
    m_rfbCli->serverPort = m_serverPort;
    // m_rfbCli->appData.compressLevel = 9;
    // m_rfbCli->appData.qualityLevel = 9;
    // m_rfbCli->appData.scaleSetting = 1;
    m_rfbCli->appData.forceTrueColour = TRUE;
    m_rfbCli->appData.useRemoteCursor = FALSE;
    m_rfbCli->appData.encodingsString = "tight ultra";
    DLOG << "VNC client configured";

    rfbClientSetClientData(m_rfbCli, nullptr, this);

    if (rfbInitClient(m_rfbCli, 0, nullptr)) {
        m_connected = true;
        DLOG << "VNC connection established successfully";
    } else {
        DLOG << "Failed to establish VNC connection";
        m_connected = false;
        return;
    }

    DLOG << "VNC screen size:" << m_rfbCli->width << "x" << m_rfbCli->height;
#ifdef HIDED_CURSOR
    QPixmap pixmap(1, 1);
    pixmap.fill(Qt::transparent);
    setCursor(QCursor(pixmap));
    DLOG << "Cursor hidden";
#endif
    // 启动帧率计时器
    m_frameTimer->start();
    DLOG << "Frame timer started";

    // 保护初始化过程
    QMutexLocker locker(&m_mutex);

    int viewWidth;
    m_phoneMode = (m_rfbCli->width < m_rfbCli->height) ? PORTRAIT : LANDSCAPE;
    if (PORTRAIT == m_phoneMode) {
        m_phoneScale = static_cast<qreal>(m_rfbCli->height) / static_cast<qreal>(m_realSize.height());
        viewWidth = static_cast<int>(m_realSize.width() * m_phoneScale);
        DLOG << "Portrait mode - scale:" << m_phoneScale << " view width:" << viewWidth;
    } else {
        m_phoneScale = static_cast<qreal>(m_rfbCli->height) / static_cast<qreal>(m_realSize.width());
        viewWidth = static_cast<int>(m_realSize.height() * m_phoneScale);
        DLOG << "Landscape mode - scale:" << m_phoneScale << " view width:" << viewWidth;
    }
    const QSize size = {viewWidth, m_rfbCli->height};

    setSurfaceSize(size);
    emit sizeChanged(size);

    // 先启动发送线程
    _vncSendThread->start();
    // 最后启动接收线程，开始接收图像
    _vncRecvThread->startRun(m_rfbCli);
    DLOG << "Worker threads started";
}


void VncViewer::stop()
{
    if (!m_connected) {
        DLOG << "Already stopped, skipping";
        return;
    }

    DLOG << "Stopping VNC connection";
    m_frameTimer->stop();
    m_connected = false;

    _vncRecvThread->stopRun();
    _vncRecvThread->quit();
    _vncRecvThread->wait();
    DLOG << "Receive thread stopped";

    _vncSendThread->quit();
    _vncSendThread->wait();
    DLOG << "Send thread stopped";

    if (m_rfbCli) {
        rfbClientCleanup(m_rfbCli);
        m_rfbCli = nullptr;
        DLOG << "VNC client cleaned up";
    }
}
