// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mobilewidget.h"
#include "cooperationstatewidget.h"
#include "devicelistwidget.h"
#include "common/commonutils.h"

#include <QMouseEvent>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QFile>
#include <QStackedLayout>

#include <qrencode.h>
#include <gui/utils/cooperationguihelper.h>

using namespace cooperation_core;

MobileWidget::MobileWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void MobileWidget::initUI()
{
    stackedLayout = new QStackedLayout;

    nnWidget = new NoNetworkWidget(this);
    dlWidget = new DeviceListWidget(this);
    conWidget = new ConnectWidget(this);
    dlWidget->setContentsMargins(10, 0, 10, 0);

    stackedLayout->addWidget(conWidget);
    stackedLayout->addWidget(dlWidget);
    stackedLayout->addWidget(nnWidget);
    stackedLayout->setCurrentIndex(0);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(stackedLayout);
    setLayout(mainLayout);

    DeviceInfoPointer info(new DeviceInfo("10.8.11.666", "my phone"));
    info->setConnectStatus(DeviceInfo::ConnectStatus::Connected);
    dlWidget->appendItem(info);
}

void MobileWidget::switchWidget(PageName page)
{
    if (stackedLayout->currentIndex() == page || page == kUnknownPage)
        return;

    stackedLayout->setCurrentIndex(page);
}

ConnectWidget::ConnectWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void ConnectWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    QLabel *title = new QLabel("扫码连接", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-weight: bold; font-size: 20px; color:rgba(0, 0, 0, 0.85);");

    QLabel *instruction = new QLabel("请使用跨端协同APP扫码", this);
    instruction->setAlignment(Qt::AlignCenter);
    instruction->setStyleSheet("font-weight: 500; font-size: 14px; color:rgba(0, 0, 0, 0.7);");

    QLabel *instruction2 = new QLabel("手机和设备需要连接同一局域网", this);
    instruction2->setAlignment(Qt::AlignCenter);
    instruction2->setStyleSheet("font-weight: 400; font-size: 12px; color:rgba(0, 0, 0, 0.6);");

    QFrame *qrFrame = new QFrame(this);
    qrFrame->setStyleSheet("background-color: lightgray; border-radius: 20px;");
    qrFrame->setLayout(new QVBoxLayout);
    qrFrame->setFixedSize(200, 200);

    qrCode = new QLabel(qrFrame);
    QPixmap qrImage = generateQRCode("wsnd", 7);
    qrCode->setPixmap(qrImage);
    qrCode->setAlignment(Qt::AlignCenter);
    qrCode->setStyleSheet("background-color : white;border-radius: 20px;");
    qrCode->setFixedSize(185, 185);
    qrFrame->layout()->setAlignment(Qt::AlignCenter);
    qrFrame->layout()->addWidget(qrCode);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(qrFrame);
    hLayout->setAlignment(Qt::AlignCenter);

    mainLayout->setSpacing(10);
    mainLayout->addSpacing(80);
    mainLayout->addWidget(title);
    mainLayout->addWidget(instruction);
    mainLayout->addWidget(instruction2);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(hLayout);
    mainLayout->addSpacing(150);
    setLayout(mainLayout);
}

void ConnectWidget::setInfo(const QString &info)
{
    QPixmap qrImage = generateQRCode(info, 7);
    qrCode->setPixmap(qrImage);
}

QPixmap ConnectWidget::generateQRCode(const QString &text, int scale)
{
    // 创建二维码对象，scale参数控制二维码的大小
    QRcode *qrcode = QRcode_encodeString(text.toStdString().c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, scale);

    if (!qrcode) {
        return QPixmap();   // 处理编码失败情况
    }

    // 计算二维码的实际大小，加入边框
    int size = (qrcode->width) * scale;   // 加上边框
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(Qt::white);   // 填充白色背景

    // 将二维码数据绘制到 QImage
    for (int i = 0; i < qrcode->width; i++) {
        for (int j = 0; j < qrcode->width; j++) {
            if (qrcode->data[j * qrcode->width + i] & 0x1) {
                // 设定黑色像素区域
                for (int x = 0; x < scale; ++x) {
                    for (int y = 0; y < scale; ++y) {
                        image.setPixel(i * scale + x + 1, j * scale + y + 1, qRgb(0, 0, 0));   // 设置为黑色像素
                    }
                }
            }
        }
    }

    // 删除二维码对象
    QRcode_free(qrcode);

    // 转换为 QPixmap 并返回
    return QPixmap::fromImage(image);
}
