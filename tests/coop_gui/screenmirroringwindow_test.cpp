// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSize>
#include <QSignalSpy>
#include <cstdlib>
#include <string>

#include "gui/phone/screenmirroringwindow.h"
#include "gui/phone/vncviewer.h"

using cooperation_core::ScreenMirroringWindow;
using cooperation_core::LockScreenWidget;

TEST(ScreenMirroringWindowTest, ConstructDoesNotCrash)
{
    ScreenMirroringWindow w("TestDevice");
    SUCCEED();
}

TEST(ScreenMirroringWindowTest, DestructorStopsVncViewer)
{
    auto *w = new ScreenMirroringWindow("TestDevice");
    delete w;
    SUCCEED();
}

TEST(ScreenMirroringWindowTest, InitSizebyView)
{
    ScreenMirroringWindow w("TestDevice");
    QSize size(1080, 2400);
    EXPECT_NO_FATAL_FAILURE(w.initSizebyView(size));
}

TEST(ScreenMirroringWindowTest, ConnectVncServerDoesNotCrash)
{
    ScreenMirroringWindow w("TestDevice");
    // Will try to connect but fail since no server. Won't crash.
    const char *pwd = std::getenv("TEST_VNC_PASSWORD");
    std::string password = pwd ? pwd : "";
    EXPECT_NO_FATAL_FAILURE(w.connectVncServer("127.0.0.1", 5900, password.c_str()));
}

TEST(ScreenMirroringWindowTest, HandleSizeChangePortrait)
{
    ScreenMirroringWindow w("TestDevice");
    // Portrait size (w < h)
    EXPECT_NO_FATAL_FAILURE(w.handleSizeChange(QSize(360, 800)));
}

TEST(ScreenMirroringWindowTest, HandleSizeChangeLandscape)
{
    ScreenMirroringWindow w("TestDevice");
    // Landscape size (w > h)
    EXPECT_NO_FATAL_FAILURE(w.handleSizeChange(QSize(800, 360)));
}

TEST(ScreenMirroringWindowTest, HandleSizeChangeSecondCall)
{
    ScreenMirroringWindow w("TestDevice");
    w.handleSizeChange(QSize(360, 800));
    // Second call should not reposition
    EXPECT_NO_FATAL_FAILURE(w.handleSizeChange(QSize(360, 800)));
}

TEST(ScreenMirroringWindowTest, ButtonClickedSignal)
{
    ScreenMirroringWindow w("TestDevice");
    QSignalSpy spy(&w, &ScreenMirroringWindow::buttonClicked);
    EXPECT_EQ(spy.count(), 0);
}

// ============ LockScreenWidget ============

TEST(LockScreenWidgetTest, ConstructDoesNotCrash)
{
    LockScreenWidget w;
    SUCCEED();
}

TEST(LockScreenWidgetTest, PaintEventDoesNotCrash)
{
    LockScreenWidget w;
    w.repaint();
    SUCCEED();
}
