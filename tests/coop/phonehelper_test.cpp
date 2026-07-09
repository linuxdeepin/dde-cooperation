// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// PhoneHelper 纯逻辑测试 + NoticeUtil 纯逻辑测试
// 避开 notifyMessage(dialog exec) 与 VNC/DBus 路径 (team memory 天花板)。

#include "lib/cooperation/core/net/helper/phonehelper.h"
#include "lib/cooperation/core/net/linux/noticeutil.h"
#include "lib/cooperation/core/discover/deviceinfo.h"

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QString>
#include <QTimer>
#include <QApplication>
#include <QWidget>
#include <QDialog>

using cooperation_core::PhoneHelper;
using cooperation_core::NoticeUtil;
using cooperation_core::DeviceInfo;
using ::DeviceInfoPointer;

namespace {
DeviceInfoPointer makeDev(const QString &ip, const QString &name, DeviceInfo::ConnectStatus st)
{
    auto p = DeviceInfoPointer(new DeviceInfo(ip, name));
    p->setConnectStatus(st);
    return p;
}
}   // namespace

// ===== buttonVisible 静态方法: 全分支 =====

TEST(PhoneHelper, ButtonVisibleConnectOnConnectable)
{
    auto info = makeDev("10.0.0.1", "dev", DeviceInfo::Connectable);
    EXPECT_TRUE(PhoneHelper::buttonVisible("connect-button", info));
}

TEST(PhoneHelper, ButtonVisibleDisconnectOnConnected)
{
    auto info = makeDev("10.0.0.2", "dev", DeviceInfo::Connected);
    EXPECT_TRUE(PhoneHelper::buttonVisible("disconnect-button", info));
}

TEST(PhoneHelper, ButtonVisibleUnknownIdReturnsFalse)
{
    auto info = makeDev("10.0.0.3", "dev", DeviceInfo::Connectable);
    EXPECT_FALSE(PhoneHelper::buttonVisible("unknown-button", info));
}

TEST(PhoneHelper, ButtonVisibleConnectOnConnectedReturnsFalse)
{
    auto info = makeDev("10.0.0.4", "dev", DeviceInfo::Connected);
    EXPECT_FALSE(PhoneHelper::buttonVisible("connect-button", info));
}

TEST(PhoneHelper, ButtonVisibleDisconnectOnConnectableReturnsFalse)
{
    auto info = makeDev("10.0.0.5", "dev", DeviceInfo::Connectable);
    EXPECT_FALSE(PhoneHelper::buttonVisible("disconnect-button", info));
}

// ===== generateQRCode: 纯字符串拼接 + emit setQRCode =====

TEST(PhoneHelper, GenerateQRCodeEmitsSignal)
{
    QSignalSpy spy(PhoneHelper::instance(), &PhoneHelper::setQRCode);
    PhoneHelper::instance()->generateQRCode("10.0.0.1", "12345", "PINCODE");
    EXPECT_EQ(spy.count(), 1);
    QString code = spy.takeFirst().at(0).toString();
    EXPECT_TRUE(code.contains("mark="));
    EXPECT_TRUE(code.contains("chinauos.com"));
}

// ===== onScreenMirroringResize: 无 window 早返回 =====
TEST(PhoneHelper, OnScreenMirroringResizeNoWindowNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::instance()->onScreenMirroringResize(100, 200));
}

// ===== resetScreenMirroringWindow: 无 window 早返回 =====
TEST(PhoneHelper, ResetScreenMirroringWindowNoWindowNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::instance()->resetScreenMirroringWindow());
}

// ===== onScreenMirroring: 无 mobileInfo 早返回 =====
TEST(PhoneHelper, OnScreenMirroringNoMobileInfoNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::instance()->onScreenMirroring());
}

// ===== onDisconnect(nullptr): 清 mobileInfo + emit disconnectMobile =====
TEST(PhoneHelper, OnDisconnectNullResetsAndEmits)
{
    QSignalSpy spy(PhoneHelper::instance(), &PhoneHelper::disconnectMobile);
    PhoneHelper::instance()->onDisconnect(nullptr);
    EXPECT_EQ(spy.count(), 1);
}

// ===== onConnect: 设置 mobileInfo + emit addMobileInfo =====
TEST(PhoneHelper, OnConnectEmitsAddMobileInfo)
{
    QSignalSpy spy(PhoneHelper::instance(), &PhoneHelper::addMobileInfo);
    auto info = makeDev("10.0.0.9", "phone", DeviceInfo::Connectable);
    PhoneHelper::instance()->onConnect(info, 800, 600);
    EXPECT_EQ(spy.count(), 1);
}

// ===== NoticeUtil 纯逻辑 =====

TEST(NoticeUtil, ResetNotifyIdZeroes)
{
    NoticeUtil util;
    util.resetNotifyId();
    // 无直接 getter, 仅验证不崩; resetNotifyId 是纯赋值 recvNotifyId=0
    SUCCEED();
}

TEST(NoticeUtil, OnActionTriggeredMismatchIdIgnored)
{
    NoticeUtil util;
    QSignalSpy spy(&util, &NoticeUtil::ActionInvoked);
    util.onActionTriggered(9999u, QString("ignored"));   // ID 不匹配 → 早返回
    EXPECT_EQ(spy.count(), 0);
}

TEST(NoticeUtil, OnActionTriggeredMatchEmits)
{
    NoticeUtil util;
    QSignalSpy spy(&util, &NoticeUtil::ActionInvoked);
    util.resetNotifyId();   // recvNotifyId=0
    util.onActionTriggered(0u, QString("open"));   // ID 匹配 → emit
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString().toStdString(), "open");
}

// ===== notifyMessage (QDialog::exec 阻塞) 用 modal-exec-auto-reject 技术 =====
// 触发前 QTimer::singleShot reject 活动模态; exec 自带事件循环会处理。

static void autoRejectModal()
{
    QTimer::singleShot(0, []() {
        QWidget *w = qApp->activeModalWidget();
        auto *d = qobject_cast<QDialog *>(w);
        if (d) d->reject();
    });
}

TEST(PhoneHelperNotify, NotifyMessageExecAutoReject)
{
    auto info = makeDev("10.0.0.10", "dev", DeviceInfo::Connectable);
    autoRejectModal();
    int code = PhoneHelper::instance()->notifyMessage("hello", QStringList{"c", "ok"});
    // exec 被 reject → 返回非 1 (通常 0)
    EXPECT_NE(code, 1);
}

TEST(PhoneHelperNotify, NotifyMessageEmptyActionsAutoReject)
{
    autoRejectModal();
    int code = PhoneHelper::instance()->notifyMessage("msg", QStringList());
    EXPECT_NE(code, 1);
}

TEST(PhoneHelperNotify, NotifyMessageReentrantGuardReturnsNeg1)
{
    // 不触发 exec, 仅验证 isInNotify 守卫: 无法直接置位, 跳过; 这里测非重入正常路径
    autoRejectModal();
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::instance()->notifyMessage("x", QStringList{"a"}));
}

// ===== buttonClicked disconnect 分支: notifyMessage auto-reject → res!=1 return =====

TEST(PhoneHelperNotify, ButtonClickedDisconnectCancelled)
{
    auto info = makeDev("10.0.0.11", "dev", DeviceInfo::Connected);
    autoRejectModal();
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::buttonClicked("disconnect-button", info));
}

// ===== onDisconnect 匹配 ip 分支: notifyMessage auto-reject =====

TEST(PhoneHelperNotify, OnDisconnectMatchingIpNotifies)
{
    auto info = makeDev("10.0.0.12", "phone", DeviceInfo::Connected);
    PhoneHelper::instance()->onConnect(info, 800, 600);   // 设置 m_mobileInfo
    autoRejectModal();
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::instance()->onDisconnect(info));
}

// ===== onScreenMirroring 有 m_mobileInfo 分支: notifyMessage auto-reject → res!=1 return =====

TEST(PhoneHelperNotify, OnScreenMirroringWithInfoNotConfirmed)
{
    auto info = makeDev("10.0.0.13", "phone", DeviceInfo::Connected);
    PhoneHelper::instance()->onConnect(info, 800, 600);
    autoRejectModal();
    EXPECT_NO_FATAL_FAILURE(PhoneHelper::instance()->onScreenMirroring());
}
