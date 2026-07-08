// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QPixmap>
#include <QVariantMap>
#include "gui/phone/phonewidget.h"
#include "discover/deviceinfo.h"

using cooperation_core::PhoneWidget;
using cooperation_core::QRCodeWidget;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

// ============ PhoneWidget ============

TEST(PhoneWidgetTest, ConstructDoesNotCrash)
{
    PhoneWidget w;
    SUCCEED();
}

TEST(PhoneWidgetTest, SwitchWidgetAllPagesDoesNotCrash)
{
    PhoneWidget w;
    w.switchWidget(PhoneWidget::kDeviceListWidget);
    w.switchWidget(PhoneWidget::kNoNetworkWidget);
    w.switchWidget(PhoneWidget::kQRCodeWidget);
    SUCCEED();
}

TEST(PhoneWidgetTest, SwitchWidgetUnknownPageEarlyReturns)
{
    PhoneWidget w;
    w.switchWidget(PhoneWidget::kUnknownPage);
    SUCCEED();
}

TEST(PhoneWidgetTest, SetDeviceInfoDoesNotCrash)
{
    PhoneWidget w;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.1", "DevA"));
    EXPECT_NO_FATAL_FAILURE(w.setDeviceInfo(info));
}

TEST(PhoneWidgetTest, AddOperationEmptyMapDoesNotCrash)
{
    PhoneWidget w;
    EXPECT_NO_FATAL_FAILURE(w.addOperation(QVariantMap{}));
}

TEST(PhoneWidgetTest, OnSetQRcodeInfoDoesNotCrash)
{
    PhoneWidget w;
    EXPECT_NO_FATAL_FAILURE(w.onSetQRcodeInfo("some qrcode payload"));
}

// ============ QRCodeWidget::generateQRCode (纯逻辑,qrencode 库) ============

TEST(QRCodeWidgetTest, GenerateQRCodeEmptyTextReturnsNullPixmap)
{
    QRCodeWidget w;
    QPixmap pm = w.generateQRCode("", 7);
    EXPECT_TRUE(pm.isNull());  // 空文本 QRcode_encodeString 失败 → 返回空 QPixmap
}

TEST(QRCodeWidgetTest, GenerateQRCodeValidTextReturnsPixmap)
{
    QRCodeWidget w;
    QPixmap pm = w.generateQRCode("https://example.com", 7);
    EXPECT_FALSE(pm.isNull());
}

TEST(QRCodeWidgetTest, SetQRcodeInfoValidTextDoesNotCrash)
{
    QRCodeWidget w;
    EXPECT_NO_FATAL_FAILURE(w.setQRcodeInfo("payload data"));
}
