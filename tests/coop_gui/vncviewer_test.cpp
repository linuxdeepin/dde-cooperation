// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 只测 VncViewer 纯逻辑(translateMouseButton/setMobileRealSize/onSizeChange/onShortcutAction),
// 跳过 socket/线程方法(start/stop/frameTimerTimeout/updateImage/paintEvent/mouse*)。
// BACK/HOME/RECENTS 枚举值在 vncviewer.cpp 匿名命名空间(BACK=0/HOME=1/RECENTS=2),用字面量。
// PORTRAIT=0/LANDSCAPE=1。SendKeyEvent 经 stub.h 替换避免真实 rfb 调用。

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QSize>
#include <rfb/rfbclient.h>
#include <rfb/keysym.h>
#include "gui/phone/vncviewer.h"
#include "addr_pri.h"
#include "stub.h"

using cooperation_core::VncViewer;

// 访问私有成员,用于注入 m_connected/m_realSize/m_phoneScale/m_phoneMode/m_rfbCli。
ACCESS_PRIVATE_FIELD(VncViewer, bool, m_connected)
ACCESS_PRIVATE_FIELD(VncViewer, QSize, m_realSize)
ACCESS_PRIVATE_FIELD(VncViewer, qreal, m_phoneScale)
ACCESS_PRIVATE_FIELD(VncViewer, int, m_phoneMode)
ACCESS_PRIVATE_FIELD(VncViewer, rfbClient *, m_rfbCli)

// translateMouseButton 是 protected,经子类 wrapper 暴露。
class VncViewerAccessor : public VncViewer
{
public:
    using VncViewer::translateMouseButton;
};

// ============ translateMouseButton ============

TEST(VncViewerTranslateMouseButtonTest, LeftMapsToRfbButton1)
{
    VncViewerAccessor v;
    EXPECT_EQ(v.translateMouseButton(Qt::LeftButton), rfbButton1Mask);
}

TEST(VncViewerTranslateMouseButtonTest, MiddleMapsToRfbButton2)
{
    VncViewerAccessor v;
    EXPECT_EQ(v.translateMouseButton(Qt::MiddleButton), rfbButton2Mask);
}

TEST(VncViewerTranslateMouseButtonTest, RightMapsToRfbButton3)
{
    VncViewerAccessor v;
    EXPECT_EQ(v.translateMouseButton(Qt::RightButton), rfbButton3Mask);
}

TEST(VncViewerTranslateMouseButtonTest, UnknownReturnsZero)
{
    VncViewerAccessor v;
    EXPECT_EQ(v.translateMouseButton(Qt::NoButton), 0);
}

// ============ setMobileRealSize ============

TEST(VncViewerTest, SetMobileRealSizeKeepsPortraitWhenWH)
{
    VncViewer v;
    v.setMobileRealSize(1080, 2400);  // w<h 保持
    auto &real = access_private_field::VncViewerm_realSize(v);
    EXPECT_EQ(real.width(), 1080);
    EXPECT_EQ(real.height(), 2400);
}

TEST(VncViewerTest, SetMobileRealSizeSwapsWhenWGreaterThanH)
{
    VncViewer v;
    v.setMobileRealSize(2400, 1080);  // w>h 交换成竖屏
    auto &real = access_private_field::VncViewerm_realSize(v);
    EXPECT_EQ(real.width(), 1080);
    EXPECT_EQ(real.height(), 2400);
}

// ============ onSizeChange ============

// m_connected=false → 早返回,不发 sizeChanged。
TEST(VncViewerTest, OnSizeChangeReturnsEarlyWhenNotConnected)
{
    VncViewer v;
    access_private_field::VncViewerm_connected(v) = false;
    QSignalSpy spy(&v, &VncViewer::sizeChanged);
    v.onSizeChange(100, 200);
    EXPECT_EQ(spy.count(), 0);
}

// m_connected=true + 模式从 LANDSCAPE 变 PORTRAIT → emit sizeChanged。
TEST(VncViewerTest, OnSizeChangeEmitsWhenConnectedAndRotated)
{
    VncViewer v;
    access_private_field::VncViewerm_connected(v) = true;
    access_private_field::VncViewerm_realSize(v) = QSize(1080, 2400);
    access_private_field::VncViewerm_phoneScale(v) = 0.5;
    access_private_field::VncViewerm_phoneMode(v) = 1;  // LANDSCAPE,触发模式切换
    QSignalSpy spy(&v, &VncViewer::sizeChanged);
    v.onSizeChange(100, 200);  // 100<200 → PORTRAIT,与 LANDSCAPE 不同
    EXPECT_GE(spy.count(), 1);
}

// ============ onShortcutAction ============
// BACK=0/HOME=1/RECENTS=2(匿名命名空间枚举值)。

// m_connected=false → 早返回。
TEST(VncViewerTest, OnShortcutActionReturnsEarlyWhenNotConnected)
{
    VncViewer v;
    access_private_field::VncViewerm_connected(v) = false;
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(0));  // BACK
    SUCCEED();
}

// m_connected=true + 未知 action → switch default,key=0,不调 SendKeyEvent。
TEST(VncViewerTest, OnShortcutActionUnknownActionDoesNotCallSendKey)
{
    VncViewer v;
    access_private_field::VncViewerm_connected(v) = true;
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(999));  // 未知 → default → key=0
    SUCCEED();
}

// stub SendKeyEvent(libvncclient 自由函数),覆盖 BACK/HOME/RECENTS switch 分支。
static rfbBool stub_SendKeyEvent(rfbClient *, uint32_t, rfbBool) { return true; }

TEST(VncViewerTest, OnShortcutActionBackHomeRecentsWithStubbedSendKey)
{
    VncViewer v;
    access_private_field::VncViewerm_connected(v) = true;
    access_private_field::VncViewerm_rfbCli(v) = reinterpret_cast<rfbClient *>(0x1);  // 非空占位

    Stub stub;
    stub.set(SendKeyEvent, stub_SendKeyEvent);
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(0));  // BACK → Escape
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(1));  // HOME → Home
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(2));  // RECENTS → PageUp
}
