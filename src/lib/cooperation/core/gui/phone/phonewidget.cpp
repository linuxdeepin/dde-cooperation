// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "phonewidget.h"
#include "gui/widgets/cooperationstatewidget.h"
#include "gui/widgets/devicelistwidget.h"
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

PhoneWidget::PhoneWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void PhoneWidget::initUI()
{
    stackedLayout = new QStackedLayout;

    nnWidget = new NoNetworkWidget(this);
    dlWidget = new DeviceListWidget(this);
    qrcodeWidget = new QRCodeWidget(this);
    dlWidget->setContentsMargins(10, 0, 10, 0);

    stackedLayout->addWidget(qrcodeWidget);
    stackedLayout->addWidget(dlWidget);
    stackedLayout->addWidget(nnWidget);
    stackedLayout->setCurrentIndex(0);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(stackedLayout);
    setLayout(mainLayout);
}

void PhoneWidget::setDeviceInfo(const DeviceInfoPointer info)
{
    switchWidget(PageName::kDeviceListWidget);
    dlWidget->appendItem(info);
}

void PhoneWidget::addOperation(const QVariantMap &map)
{
    dlWidget->addItemOperation(map);
}

void PhoneWidget::onSetQRcodeInfo(const QString &info)
{
    qrcodeWidget->setQRcodeInfo(info);
}

void PhoneWidget::switchWidget(PageName page)
{
    if (stackedLayout->currentIndex() == page || page == kUnknownPage)
        return;

    stackedLayout->setCurrentIndex(page);
}

QRCodeWidget::QRCodeWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void QRCodeWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();

    QLabel *title = new QLabel(tr("Scan code connection"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-weight: bold; font-size: 20px; color:rgba(0, 0, 0, 0.85);");

    QLabel *instruction = new QLabel(tr("Please use the cross end collaboration app to scan the code"), this);
    instruction->setAlignment(Qt::AlignCenter);
    instruction->setStyleSheet("font-weight: 500; font-size: 14px; color:rgba(0, 0, 0, 0.7);");

    QLabel *instruction2 = new QLabel(tr("Mobile phones and devices need to be connected to the same local area network"), this);
    instruction2->setAlignment(Qt::AlignCenter);
    instruction2->setStyleSheet("font-weight: 400; font-size: 12px; color:rgba(0, 0, 0, 0.6);");

    QFrame *qrFrame = new QFrame(this);
    qrFrame->setStyleSheet("background-color: rgba(0, 0, 0, 0.05); border-radius: 18px;");
    qrFrame->setLayout(new QVBoxLayout);
    qrFrame->setFixedSize(200, 200);

    qrCode = new QLabel(qrFrame);
    QPixmap qrImage = generateQRCode("", 7);
    qrCode->setPixmap(qrImage);
    qrCode->setAlignment(Qt::AlignCenter);
    qrCode->setStyleSheet("background-color : white;border-radius: 10px;");
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

void QRCodeWidget::setQRcodeInfo(const QString &info)
{
    QPixmap qrImage = generateQRCode(info, 7);
    qrCode->setPixmap(qrImage);
}

QPixmap QRCodeWidget::generateQRCode(const QString &text, int scale)
{
    // 创建二维码对象，scale 参数控制二维码的大小
    QRcode *qrcode = QRcode_encodeString(text.toStdString().c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, scale);

    if (!qrcode) {
        return QPixmap();   // 处理编码失败情况
    }

    // 计算二维码的实际大小，加入边框
    int size = (qrcode->width) * scale;   // 加上边框
    QImage image(size, size, QImage::Format_RGB32);
    image.fill(Qt::white);   // 填充白色背景

    // 在二维码中间绘制一个 75x75 的空白区域
    int iconSize = 77;   // 空白区域的尺寸
    int iconX = (size - iconSize) / 2;   // 图标X坐标
    int iconY = (size - iconSize) / 2;   // 图标Y坐标
    QRect iconRect(iconX, iconY, iconSize, iconSize);

    // 绘制白色矩形，留空区域
    QPainter painter(&image);
    painter.fillRect(iconRect, Qt::white);

    // 将二维码数据绘制到 QImage
    for (int i = 0; i < qrcode->width; i++) {
        for (int j = 0; j < qrcode->width; j++) {
            if (qrcode->data[j * qrcode->width + i] & 0x1) {
                // 设定黑色像素区域
                for (int x = 0; x < scale; ++x) {
                    for (int y = 0; y < scale; ++y) {
                        // 判断当前绘制的黑色像素是否在空白区域内
                        if (!(iconX <= i * scale + x && i * scale + x < iconX + iconSize && iconY <= j * scale + y && j * scale + y < iconY + iconSize)) {
                            image.setPixel(i * scale + x + 1, j * scale + y + 1, qRgb(0, 0, 0));   // 设置为黑色像素
                        }
                    }
                }
            }
        }
    }

    // 删除二维码对象
    QRcode_free(qrcode);

    // 使用 QPainter 在空白区域绘制图标
    QIcon icon(":/icons/deepin/builtin/icons/uos_assistant@3x.png");
    painter.drawPixmap(iconX, iconY, icon.pixmap(iconSize, iconSize));

    // 转换为 QPixmap 并返回
    return QPixmap::fromImage(image).scaled(170, 170, Qt::KeepAspectRatio);
}
