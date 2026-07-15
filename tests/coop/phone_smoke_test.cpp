// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QApplication>
#include <QBrush>
#include <QImage>
#include <QPixmap>
#include <QSize>
#include <QString>

#include "lib/cooperation/core/gui/phone/vncviewer.h"
#include "lib/cooperation/core/gui/phone/vncrecvthread.h"
#include "lib/cooperation/core/gui/phone/vncsendworker.h"
#include "lib/cooperation/core/gui/phone/screenmirroringwindow.h"
#include "lib/cooperation/core/gui/phone/phonewidget.h"
#include "lib/cooperation/core/net/helper/phonehelper.h"
#include "lib/cooperation/core/discover/deviceinfo.h"

using cooperation_core::VncViewer;
using ::VNCRecvThread;
using ::VNCSendWorker;
using cooperation_core::ScreenMirroringWindow;
using cooperation_core::LockScreenWidget;
using cooperation_core::PhoneWidget;
using cooperation_core::QRCodeWidget;
using cooperation_core::PhoneHelper;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

TEST(VncViewerSmoke, ConstructAndDestroy)
{
    EXPECT_NO_FATAL_FAILURE({
        VncViewer *v = new VncViewer();
        delete v;
    });
}

TEST(VncViewerSmoke, BackgroundBrushRoundTrip)
{
    VncViewer v;
    QBrush brush(Qt::red);
    v.setBackgroundBrush(brush);
    EXPECT_EQ(v.backgroundBrush(), brush);
}

TEST(VncViewerSmoke, ScaledRoundTrip)
{
    VncViewer v;
    EXPECT_TRUE(v.scaled());
    v.setScaled(false);
    EXPECT_FALSE(v.scaled());
    v.setScaled(true);
    EXPECT_TRUE(v.scaled());
}

TEST(VncViewerSmoke, FpsAndFrameCounterRoundTrip)
{
    VncViewer v;
    v.setFrameCounter(0);
    EXPECT_EQ(v.frameCounter(), 0);
    v.incFrameCounter();
    EXPECT_EQ(v.frameCounter(), 1);
    v.setFrameCounter(42);
    EXPECT_EQ(v.frameCounter(), 42);
    v.setCurrentFps(7);
    EXPECT_EQ(v.currentFps(), 7);
}

TEST(VncViewerSmoke, SetServesNoCrash)
{
    VncViewer v;
    EXPECT_NO_FATAL_FAILURE(v.setServes(std::string("127.0.0.1"), 5900, std::string("pwd")));
}

TEST(VncViewerSmoke, SetMobileRealSizeNoCrash)
{
    VncViewer v;
    EXPECT_NO_FATAL_FAILURE(v.setMobileRealSize(400, 800));
    EXPECT_NO_FATAL_FAILURE(v.setMobileRealSize(800, 400));
}

TEST(VncViewerSmoke, FrameTimerTimeoutResetsCounterAndSetsFps)
{
    VncViewer v;
    v.setFrameCounter(5);
    EXPECT_NO_FATAL_FAILURE(v.frameTimerTimeout());
    EXPECT_EQ(v.frameCounter(), 0);
    EXPECT_EQ(v.currentFps(), 5);
}

TEST(VncViewerSmoke, TranslateMouseButtonMapping)
{
    VncViewer v;
    EXPECT_NE(v.translateMouseButton(Qt::LeftButton), 0);
    EXPECT_NE(v.translateMouseButton(Qt::MiddleButton), 0);
    EXPECT_NE(v.translateMouseButton(Qt::RightButton), 0);
    EXPECT_EQ(v.translateMouseButton(Qt::NoButton), 0);
}

TEST(VncViewerSmoke, StopWhenNotConnectedEarlyReturn)
{
    VncViewer v;
    EXPECT_NO_FATAL_FAILURE(v.stop());
}

TEST(VncViewerSmoke, OnSizeChangeWhenNotConnectedIgnored)
{
    VncViewer v;
    v.m_connected = false;
    EXPECT_NO_FATAL_FAILURE(v.onSizeChange(100, 200));
}

TEST(VncViewerSmoke, OnShortcutActionWhenNotConnectedIgnored)
{
    VncViewer v;
    v.m_connected = false;
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(0));
    EXPECT_NO_FATAL_FAILURE(v.onShortcutAction(1));
}

TEST(VncViewerSmoke, UpdateImageWithNullImageIgnored)
{
    VncViewer v;
    QImage nullImg;
    EXPECT_NO_FATAL_FAILURE(v.updateImage(nullImg));
}

TEST(VncViewerSmoke, UpdateSurfaceNoCrash)
{
    VncViewer v;
    v.resize(200, 300);
    EXPECT_NO_FATAL_FAILURE(v.updateSurface());
}

TEST(VncRecvThreadSmoke, ConstructAndStopRunIdle)
{
    VNCRecvThread t;
    EXPECT_NO_FATAL_FAILURE(t.stopRun());
}

TEST(VncSendWorkerSmoke, Construct)
{
    EXPECT_NO_FATAL_FAILURE({ VNCSendWorker w; });
}

TEST(LockScreenWidgetSmoke, Construct)
{
    EXPECT_NO_FATAL_FAILURE({ LockScreenWidget w; });
}

TEST(ScreenMirroringWindowSmoke, ConstructAndDestroy)
{
    EXPECT_NO_FATAL_FAILURE({
        ScreenMirroringWindow *w = new ScreenMirroringWindow("device-A");
        delete w;
    });
}

TEST(ScreenMirroringWindowSmoke, InitSizeByViewNoCrash)
{
    ScreenMirroringWindow *w = new ScreenMirroringWindow("device-B");
    QSize sz(360, 640);
    EXPECT_NO_FATAL_FAILURE(w->initSizebyView(sz));
    delete w;
}

TEST(ScreenMirroringWindowSmoke, HandleSizeChangeNoCrash)
{
    ScreenMirroringWindow *w = new ScreenMirroringWindow("device-C");
    EXPECT_NO_FATAL_FAILURE(w->handleSizeChange(QSize(360, 640)));
    delete w;
}

TEST(PhoneWidgetSmoke, ConstructAndSwitchPages)
{
    PhoneWidget pw;
    EXPECT_NO_FATAL_FAILURE(pw.switchWidget(PhoneWidget::kDeviceListWidget));
    EXPECT_NO_FATAL_FAILURE(pw.switchWidget(PhoneWidget::kNoNetworkWidget));
    EXPECT_NO_FATAL_FAILURE(pw.switchWidget(PhoneWidget::kQRCodeWidget));
}

TEST(PhoneWidgetSmoke, SwitchToUnknownPageNoOp)
{
    PhoneWidget pw;
    EXPECT_NO_FATAL_FAILURE(pw.switchWidget(PhoneWidget::kUnknownPage));
}

TEST(PhoneWidgetSmoke, OnSetQRcodeInfoNoCrash)
{
    PhoneWidget pw;
    EXPECT_NO_FATAL_FAILURE(pw.onSetQRcodeInfo("some-info"));
}

TEST(PhoneWidgetSmoke, SetDeviceInfoNoCrash)
{
    PhoneWidget pw;
    DeviceInfoPointer info(new DeviceInfo("10.0.0.1", "phone"));
    info->setConnectStatus(DeviceInfo::Connectable);
    EXPECT_NO_FATAL_FAILURE(pw.setDeviceInfo(info));
}

TEST(QRCodeWidgetSmoke, GenerateQRCodeReturnsPixmap)
{
    QRCodeWidget w;
    QPixmap pm = w.generateQRCode("hello", 4);
    EXPECT_FALSE(pm.isNull());
}

TEST(QRCodeWidgetSmoke, GenerateQRCodeEmptyTextReturnsNull)
{
    QRCodeWidget w;
    QPixmap pm = w.generateQRCode("", 4);
    EXPECT_TRUE(pm.isNull());
}

TEST(PhoneHelperSmoke, InstanceNonNull)
{
    EXPECT_NE(PhoneHelper::instance(), nullptr);
}

TEST(PhoneHelperSmoke, InstanceSamePointer)
{
    EXPECT_EQ(PhoneHelper::instance(), PhoneHelper::instance());
}
