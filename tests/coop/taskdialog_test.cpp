// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// CooperationTaskDialog + CooperationTransDialog 纯 UI 方法测试
// offscreen 下构造 dialog 调 switch*/show* 方法, 验证不崩 + 信号可发。

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QString>
#include <QTimer>
#include <QApplication>
#include <QWidget>
#include <QDialog>

#include "lib/cooperation/core/net/helper/dialogs/cooperationtaskdialog.h"
#include "lib/cooperation/core/net/helper/dialogs/cooperationdialog.h"

using cooperation_core::CooperationTaskDialog;
using cooperation_core::CooperationTransDialog;

// ===== CooperationTaskDialog switch* 全方法 =====

TEST(CooperationTaskDialogTest, SwitchWaitPageNoCrash)
{
    CooperationTaskDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.switchWaitPage("waiting"));
}

TEST(CooperationTaskDialogTest, SwitchFailPageRetryTrue)
{
    CooperationTaskDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.switchFailPage("fail", "msg", true));
}

TEST(CooperationTaskDialogTest, SwitchFailPageRetryFalse)
{
    CooperationTaskDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.switchFailPage("fail", "msg", false));
}

TEST(CooperationTaskDialogTest, SwitchConfirmPageNoCrash)
{
    CooperationTaskDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.switchConfirmPage("confirm", "body"));
}

TEST(CooperationTaskDialogTest, SwitchInfomationPageNoCrash)
{
    CooperationTaskDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.switchInfomationPage("info", "body"));
}

TEST(CooperationTaskDialogTest, SetTaskTitleNoCrash)
{
    CooperationTaskDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.setTaskTitle("title"));
}

TEST(CooperationTaskDialogTest, WaitCanceledSignalEmittable)
{
    CooperationTaskDialog dlg;
    QSignalSpy spy(&dlg, &CooperationTaskDialog::waitCanceled);
    emit dlg.waitCanceled();
    EXPECT_EQ(spy.count(), 1);
}

TEST(CooperationTaskDialogTest, RetryConnectedSignalEmittable)
{
    CooperationTaskDialog dlg;
    QSignalSpy spy(&dlg, &CooperationTaskDialog::retryConnected);
    emit dlg.retryConnected();
    EXPECT_EQ(spy.count(), 1);
}

TEST(CooperationTaskDialogTest, RejectRequestSignalEmittable)
{
    CooperationTaskDialog dlg;
    QSignalSpy spy(&dlg, &CooperationTaskDialog::rejectRequest);
    emit dlg.rejectRequest();
    EXPECT_EQ(spy.count(), 1);
}

TEST(CooperationTaskDialogTest, AcceptRequestSignalEmittable)
{
    CooperationTaskDialog dlg;
    QSignalSpy spy(&dlg, &CooperationTaskDialog::acceptRequest);
    emit dlg.acceptRequest();
    EXPECT_EQ(spy.count(), 1);
}

// ===== CooperationTransDialog show* 全方法 =====

TEST(CooperationTransDialogTest, ShowConfirmDialogNoCrash)
{
    CooperationTransDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.showConfirmDialog("nick"));
}

TEST(CooperationTransDialogTest, ShowWaitConfirmDialogNoCrash)
{
    CooperationTransDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.showWaitConfirmDialog());
}

TEST(CooperationTransDialogTest, ShowResultDialogSuccess)
{
    CooperationTransDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.showResultDialog(true, "done", true));
}

TEST(CooperationTransDialogTest, ShowResultDialogFail)
{
    CooperationTransDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.showResultDialog(false, "fail", false));
}

TEST(CooperationTransDialogTest, ShowProgressDialogNoCrash)
{
    CooperationTransDialog dlg;
    EXPECT_NO_FATAL_FAILURE(dlg.showProgressDialog("sending"));
}

TEST(CooperationTransDialogTest, UpdateProgressNoCrash)
{
    CooperationTransDialog dlg;
    dlg.showProgressDialog("sending");
    EXPECT_NO_FATAL_FAILURE(dlg.updateProgress(50, "00:01:00"));
}
