// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QImage>
#include <QSignalSpy>
#include <QThread>
#include <memory>
#include <cstring>

#include "gui/phone/vncrecvthread.h"
#include "gui/phone/vncsendworker.h"
#include "stub.h"

#include <rfb/rfbclient.h>

// Stub libvncclient functions to avoid real network calls
static rfbBool stub_SendPointerEvent(rfbClient *, int, int, int) { return true; }
static rfbBool stub_SendKeyEvent(rfbClient *, uint32_t, rfbBool) { return true; }
static rfbBool stub_SendIncrementalFramebufferUpdateRequest(rfbClient *) { return true; }

// ============ VNCRecvThread ============

TEST(VNCRecvThreadTest, ConstructDefault)
{
    VNCRecvThread t;
    SUCCEED();
}

TEST(VNCRecvThreadTest, StopRunWhenNotRunning)
{
    VNCRecvThread t;
    EXPECT_NO_FATAL_FAILURE(t.stopRun());
}

TEST(VNCRecvThreadTest, StartAndStopCycle)
{
    VNCRecvThread t;
    auto fakeClient = std::make_unique<rfbClient>();
    memset(fakeClient.get(), 0, sizeof(rfbClient));

    t.startRun(fakeClient.get());
    // Thread starts, WaitForMessage on fake client fails quickly
    QThread::msleep(100);

    t.stopRun();
    ASSERT_TRUE(t.wait(2000)) << "Thread did not stop in time";
}

TEST(VNCRecvThreadTest, StopRunTwiceSafe)
{
    VNCRecvThread t;
    t.stopRun();
    t.stopRun();
    SUCCEED();
}

// ============ VNCSendWorker ============

TEST(VNCSendWorkerTest, ConstructDoesNotCrash)
{
    VNCSendWorker w;
    SUCCEED();
}

TEST(VNCSendWorkerTest, SendMouseUpdateMsgDoesNotCrash)
{
    Stub stub;
    stub.set(SendPointerEvent, stub_SendPointerEvent);
    stub.set(SendIncrementalFramebufferUpdateRequest, stub_SendIncrementalFramebufferUpdateRequest);

    VNCSendWorker w;
    auto fakeClient = std::make_unique<rfbClient>();
    memset(fakeClient.get(), 0, sizeof(rfbClient));
    EXPECT_NO_FATAL_FAILURE(w.sendMouseUpdateMsg(fakeClient.get(), 100, 200, 1));
}

TEST(VNCSendWorkerTest, SendKeyUpdateMsgDoesNotCrash)
{
    Stub stub;
    stub.set(SendKeyEvent, stub_SendKeyEvent);
    stub.set(SendIncrementalFramebufferUpdateRequest, stub_SendIncrementalFramebufferUpdateRequest);

    VNCSendWorker w;
    auto fakeClient = std::make_unique<rfbClient>();
    memset(fakeClient.get(), 0, sizeof(rfbClient));
    EXPECT_NO_FATAL_FAILURE(w.sendKeyUpdateMsg(fakeClient.get(), 0x41, true));
    EXPECT_NO_FATAL_FAILURE(w.sendKeyUpdateMsg(fakeClient.get(), 0x41, false));
}
