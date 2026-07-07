// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QCloseEvent>
#include <QCoreApplication>
#include "net/helper/dialogs/cooperationdialog.h"

using cooperation_core::ConfirmWidget;
using cooperation_core::ProgressWidget;
using cooperation_core::ResultWidget;
using cooperation_core::WaitConfirmWidget;
using cooperation_core::CooperationTransDialog;

// ============ ConfirmWidget ============

TEST(ConfirmWidgetTest, ConstructDoesNotCrash)
{
    ConfirmWidget w;
    SUCCEED();
}

TEST(ConfirmWidgetTest, SetDeviceNameDoesNotCrash)
{
    ConfirmWidget w;
    w.setDeviceName("TestDev");
    SUCCEED();
}

// ============ ProgressWidget ============

TEST(ProgressWidgetTest, SetTitleAndProgressDoNotCrash)
{
    ProgressWidget w;
    w.setTitle("Transferring");
    w.setProgress(42, "00:12");
    SUCCEED();
}

TEST(ProgressWidgetTest, ProgressBoundaryValuesDoNotCrash)
{
    ProgressWidget w;
    w.setProgress(0, "00:00");
    w.setProgress(100, "00:30");
    SUCCEED();
}

// ============ ResultWidget ============

TEST(ResultWidgetTest, SetSuccessResultAndGetReturnsTrue)
{
    ResultWidget w;
    w.setResult(true, "ok", true);
    EXPECT_TRUE(w.getResult());
}

TEST(ResultWidgetTest, SetFailureResultAndGetReturnsFalse)
{
    ResultWidget w;
    w.setResult(false, "fail", false);
    EXPECT_FALSE(w.getResult());
}

TEST(ResultWidgetTest, SetSuccessWithoutViewDoesNotCrash)
{
    ResultWidget w;
    w.setResult(true, "ok", false);
    EXPECT_TRUE(w.getResult());
}

// ============ WaitConfirmWidget ============

TEST(WaitConfirmWidgetTest, StartMovieDoesNotCrash)
{
    WaitConfirmWidget w;
    w.startMovie();
    SUCCEED();
}

// ============ CooperationTransDialog: show* / updateProgress ============

TEST(CooperationTransDialogTest, ShowConfirmDialogDoesNotCrash)
{
    CooperationTransDialog dlg;
    dlg.showConfirmDialog("DevA");
    SUCCEED();
}

TEST(CooperationTransDialogTest, ShowWaitConfirmDialogDoesNotCrash)
{
    CooperationTransDialog dlg;
    dlg.showWaitConfirmDialog();
    SUCCEED();
}

TEST(CooperationTransDialogTest, ShowResultDialogSuccessAndFailure)
{
    CooperationTransDialog dlg;
    dlg.showResultDialog(true, "ok", true);
    dlg.showResultDialog(false, "fail", false);
    SUCCEED();
}

TEST(CooperationTransDialogTest, ShowProgressDialogDoesNotCrash)
{
    CooperationTransDialog dlg;
    dlg.showProgressDialog("Transferring");
    SUCCEED();
}

// 首次 showProgressDialog 后 currentWidget 已是 progressWidget,
// 再次调用走早返回分支 (cooperationdialog.cpp:240-241)。
TEST(CooperationTransDialogTest, ShowProgressDialogTwiceHitsEarlyReturn)
{
    CooperationTransDialog dlg;
    dlg.showProgressDialog("First");
    dlg.showProgressDialog("Second");
    SUCCEED();
}

TEST(CooperationTransDialogTest, UpdateProgressDoesNotCrash)
{
    CooperationTransDialog dlg;
    dlg.showProgressDialog("Title");
    dlg.updateProgress(55, "00:10");
    SUCCEED();
}

// ============ CooperationTransDialog: closeEvent 三条安全分支 ============
// 注:resultWidget 分支会调 qApp->exit()(onlyTransfer && success),会杀掉测试进程,
// 故避开——不切到 resultWidget 后发关闭事件。

TEST(CooperationTransDialogCloseEventTest, ClosingConfirmPageEmitsRejected)
{
    CooperationTransDialog dlg;
    dlg.showConfirmDialog("Dev");
    QSignalSpy spy(&dlg, &CooperationTransDialog::rejected);
    QCloseEvent e;
    QCoreApplication::sendEvent(&dlg, &e);
    EXPECT_EQ(spy.count(), 1);
}

TEST(CooperationTransDialogCloseEventTest, ClosingProgressPageEmitsCancel)
{
    CooperationTransDialog dlg;
    dlg.showProgressDialog("Transferring");
    QSignalSpy spy(&dlg, &CooperationTransDialog::cancel);
    QCloseEvent e;
    QCoreApplication::sendEvent(&dlg, &e);
    EXPECT_EQ(spy.count(), 1);
}

TEST(CooperationTransDialogCloseEventTest, ClosingWaitConfirmPageEmitsCancelApply)
{
    CooperationTransDialog dlg;
    dlg.showWaitConfirmDialog();
    QSignalSpy spy(&dlg, &CooperationTransDialog::cancelApply);
    QCloseEvent e;
    QCoreApplication::sendEvent(&dlg, &e);
    EXPECT_EQ(spy.count(), 1);
}

// 未 show() 的对话框 isVisible()=false, 走 closeEvent 的 accept 分支
// (cooperationdialog.cpp:257-260)。注: 源码该分支后无 return 会继续 fall-through
// 到 currentWidget 判断, 默认 confirmWidget 仍会 emit rejected, 此处只验不崩。
TEST(CooperationTransDialogCloseEventTest, ClosingUnshownDialogDoesNotCrash)
{
    CooperationTransDialog dlg;
    QCloseEvent e;
    EXPECT_NO_FATAL_FAILURE(QCoreApplication::sendEvent(&dlg, &e));
}
