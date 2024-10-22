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
    m_frameTimer->setInterval(1000);

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

    qWarning() << " FPS: " << currentFps();
}

void VncViewer::onShortcutAction(int action)
{
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

    if (key > 0 && m_rfbCli) {
        SendKeyEvent(m_rfbCli, qt2keysym(key), true);
    }
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
    // QElapsedTimer timer; // 创建计时器
    // timer.start(); // 启动计时

    // 检查宽度和高度是否倒置
    // if (originalSize.width() == (cl->height * m_scale) && originalSize.height() == (cl->width * m_scale)) {
    //     setSurfaceSize({cl->width, cl->height});
    // }

    m_image = QImage(cl->frameBuffer, cl->width, cl->height, QImage::Format_RGB16);

    update();

    // // 计算耗时并打印，以纳秒为单位
    // qint64 elapsed = timer.nsecsElapsed(); // 获取耗时（纳秒）
    // qDebug() << "finishedFramebufferUpdate 耗时:" << elapsed << "纳秒";
}

// std::thread* VncViewer::vncThread()
// {
//     return m_vncThread;
// }

void VncViewer::paintEvent(QPaintEvent *event)
{
    event->accept();

    QPainter painter(this);
    painter.drawImage(this->rect(), m_image);

    // painter.begin(this);
    // // painter.setRenderHints(QPainter::SmoothPixmapTransform);
    // // painter.fillRect(rect(), m_backgroundBrush);
    // if ( scaled() ) {
    //     m_surfaceRect.moveCenter(rect().center());
    //     painter.scale(m_scale, m_scale);
    //     painter.drawPixmap(m_surfaceRect.x() / m_scale, m_surfaceRect.y() / m_scale, m_surfacePixmap);
    // } else {
    //   painter.scale(1.0, 1.0);
    //   painter.drawPixmap((width() - m_surfacePixmap.width()) / 2, (height() - m_surfacePixmap.height()) / 2, m_surfacePixmap);
    // }
    // painter.end();

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

    // 使用 resize 代替 setFixedSize，使得大小可变
    // this->resize(m_surfaceRect.width() * m_scale, m_surfaceRect.height() * m_scale);

    m_originalSize = QSize(m_surfaceRect.width() * m_scale, m_surfaceRect.height() * m_scale); // 记录原始大小
    this->resize(m_originalSize); // 初始设置

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

            // 按比例调整窗口大小
            int newWidth = m_originalSize.width() * m_scale;
            int newHeight = m_originalSize.height() * m_scale;
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
    m_rfbCli->format.depth = 24;
    m_rfbCli->format.bitsPerPixel = 16;
    m_rfbCli->format.redShift = 11;
    m_rfbCli->format.greenShift = 5;
    m_rfbCli->format.blueShift = 0;
    m_rfbCli->format.redMax = 0x1f;
    m_rfbCli->format.greenMax = 0x3f;
    m_rfbCli->format.blueMax = 0x1f;
    m_rfbCli->appData.compressLevel = 9;
    m_rfbCli->appData.qualityLevel = 1;
    m_rfbCli->appData.encodingsString = "tight ultra";
    m_rfbCli->FinishedFrameBufferUpdate = finishedFramebufferUpdateStatic;
    m_rfbCli->serverHost = strdup(m_serverIp.c_str());
    m_rfbCli->serverPort = m_serverPort;
    m_rfbCli->appData.forceTrueColour = TRUE;
    m_rfbCli->appData.useRemoteCursor = FALSE;

    rfbClientSetClientData(m_rfbCli, nullptr, this);

    if (rfbInitClient(m_rfbCli, 0, nullptr)) {
        m_connected = true;
    } else {
        std::cout << "[INFO] disconnected" << std::endl;
        m_connected = false;
        return;
    }

    std::cout << "[INFO] vnc screen: " << m_rfbCli->width << " x " << m_rfbCli->height << std::endl;
    // QPixmap pixmap(1, 1);
    // pixmap.fill(Qt::transparent);
    // setCursor(QCursor(pixmap));
    // 启动帧率计时器
    m_frameTimer->start();
    m_stop = false;

    const QSize screenSize = {m_rfbCli->width, m_rfbCli->height};
    emit sizeChanged(screenSize);

    setSurfaceSize(screenSize);

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
    std::cout << "[INFO] stop............" << std::endl;
    m_stop = true;
    m_frameTimer->stop();
    m_vncThread->join();
    rfbClientCleanup(m_rfbCli);
}
