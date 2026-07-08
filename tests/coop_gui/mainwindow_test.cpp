// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QVariantMap>
#include <QStringList>
#include <QScopedPointer>
#include <QCloseEvent>
#include <QDialog>
#include <QTimer>
#include <QCoreApplication>
#include "gui/mainwindow.h"
#include "gui/mainwindow_p.h"
#include "addr_pri.h"
#include "discover/deviceinfo.h"

using cooperation_core::MainWindow;
using cooperation_core::MainWindowPrivate;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

// d 是 QScopedPointer<MainWindowPrivate>,含逗号需 typedef 消歧。
using MWPrivatePtr = QScopedPointer<MainWindowPrivate>;
ACCESS_PRIVATE_FIELD(MainWindow, MWPrivatePtr, d)

// MainWindow 构造重(PIMPL + titlebar + moveCenter + initConnect 连 DiscoverController),
// 用 EXPECT_NO_FATAL_FAILURE 包裹;slot 多委托 workspaceWidget/phoneWidget。

class MainWindowTest : public ::testing::Test {
protected:
    MainWindow *w {nullptr};
    void SetUp() override { w = new MainWindow(); }
    void TearDown() override { delete w; }
};

TEST_F(MainWindowTest, ConstructDoesNotCrash)
{
    ASSERT_NE(w, nullptr);
}

TEST_F(MainWindowTest, SetFirstTipVisibleDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->setFirstTipVisible());
}

TEST_F(MainWindowTest, OnFindDeviceEmitsSearchDevice)
{
    QSignalSpy spy(w, &MainWindow::searchDevice);
    w->onFindDevice("10.0.0.1");
    EXPECT_EQ(spy.count(), 1);
}

// onDiscoveryFinished:无发现 + 先前 onFindDevice 置 _userAction=true → 切 NoResult 页。
TEST_F(MainWindowTest, OnDiscoveryFinishedNotFoundAfterUserActionDoesNotCrash)
{
    w->onFindDevice("10.0.0.2");  // 置 _userAction
    EXPECT_NO_FATAL_FAILURE(w->onDiscoveryFinished(false));
}

TEST_F(MainWindowTest, OnDiscoveryFinishedFoundDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->onDiscoveryFinished(true));
}

TEST_F(MainWindowTest, OnSwitchModePcAndMobileDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->onSwitchMode(kPC));
    EXPECT_NO_FATAL_FAILURE(w->onSwitchMode(kMobile));
}

TEST_F(MainWindowTest, RemoveDeviceDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->removeDevice("10.0.0.3"));
}

TEST_F(MainWindowTest, OnRegistOperationsEmptyMapDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->onRegistOperations(QVariantMap{}));
}

TEST_F(MainWindowTest, AddDeviceEmptyListDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->addDevice(QList<DeviceInfoPointer>{}));
}

// onLookingForDevices 查 CooperationUtil::localIPAddress 后切页。
TEST_F(MainWindowTest, OnLookingForDevicesDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->onLookingForDevices());
}

TEST_F(MainWindowTest, OnlineStateChangedDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->onlineStateChanged("10.0.0.5"));
    EXPECT_NO_FATAL_FAILURE(w->onlineStateChanged(""));  // 无效 IP 分支
}

TEST_F(MainWindowTest, AddMobileDeviceDoesNotCrash)
{
    DeviceInfoPointer info(new DeviceInfo("10.0.0.6", "PhoneA"));
    EXPECT_NO_FATAL_FAILURE(w->addMobileDevice(info));
}

TEST_F(MainWindowTest, MinimizedAPPDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->minimizedAPP());
}

// ENABLE_PHONE 下 public slot。
TEST_F(MainWindowTest, DisconnectMobileDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->disconnectMobile());
}

TEST_F(MainWindowTest, OnSetQRCodeDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->onSetQRCode("some-qrcode-payload"));
}

// ============ MainWindowPrivate 私有方法深挖(经 ACCESS d 指针) ============

static MainWindowPrivate *priv(MainWindow *w)
{
    return access_private_field::MainWindowd(*w).data();
}

// handleSettingMenuTriggered(kSettings):首次创建 SettingDialog;再次因 SettingDialogShown
// 属性早返回。两个分支都覆盖。SettingDialog 父对象是 q,TearDown 随 MainWindow 析构。
TEST_F(MainWindowTest, HandleSettingMenuSettingsCreatesDialogThenEarlyReturn)
{
    auto *p = priv(w);
    EXPECT_NO_FATAL_FAILURE(p->handleSettingMenuTriggered(kSettings));   // 创建
    EXPECT_NO_FATAL_FAILURE(p->handleSettingMenuTriggered(kSettings));   // 早返回
}

TEST_F(MainWindowTest, HandleSettingMenuDownloadWindowClientDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(priv(w)->handleSettingMenuTriggered(kDownloadWindowClient));
}

TEST_F(MainWindowTest, HandleSettingMenuDownloadMobileClientDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(priv(w)->handleSettingMenuTriggered(kDownloadMobileClient));
}

TEST_F(MainWindowTest, HandleSettingMenuUnknownActionDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(priv(w)->handleSettingMenuTriggered(999));
}

TEST_F(MainWindowTest, PrivateSetIPDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(priv(w)->setIP("10.0.0.7"));
}

TEST_F(MainWindowTest, PrivateMoveCenterDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(priv(w)->moveCenter());
}

// addMobileOperation(ENABLE_PHONE public)委托 phoneWidget->addOperation。
TEST_F(MainWindowTest, AddMobileOperationDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(w->addMobileOperation(QVariantMap{}));
}

// closeEvent(onlyTransfer=false 默认)→ showCloseDialog → CooperationDialog::exec 阻塞。
// 用 QTimer::singleShot 在 exec 事件循环内 reject 活动模态对话框,使 exec 返回 Rejected
// (跳过 accept 分支,不 quit),closeEvent 继续 event->ignore。强制 onlyTransfer=false 防 quit。
TEST_F(MainWindowTest, CloseEventShowsDialogAutoRejectedDoesNotCrash)
{
    qApp->setProperty("onlyTransfer", false);
    QTimer::singleShot(50, []() {
        if (auto *modal = qobject_cast<QDialog *>(qApp->activeModalWidget()))
            modal->reject();
    });
    QCloseEvent e;
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(w, &e));
}
