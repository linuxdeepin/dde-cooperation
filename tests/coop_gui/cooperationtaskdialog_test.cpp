// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "net/helper/dialogs/cooperationtaskdialog.h"

using cooperation_core::CooperationTaskDialog;

TEST(CooperationTaskDialogTest, ConstructDoesNotCrash)
{
    CooperationTaskDialog dlg;
    SUCCEED();
}

TEST(CooperationTaskDialogTest, SwitchWaitPageDoesNotCrash)
{
    CooperationTaskDialog dlg;
    dlg.switchWaitPage("Waiting");
    SUCCEED();
}

// retry=true 让 retryBtn 可见,retry=false 隐藏 (cooperationtaskdialog.cpp:33)。
TEST(CooperationTaskDialogTest, SwitchFailPageWithRetryToggleDoesNotCrash)
{
    CooperationTaskDialog dlg;
    dlg.switchFailPage("Failed", "network error", true);
    dlg.switchFailPage("Failed", "timeout", false);
    SUCCEED();
}

TEST(CooperationTaskDialogTest, SwitchConfirmPageDoesNotCrash)
{
    CooperationTaskDialog dlg;
    dlg.switchConfirmPage("Confirm?", "10.0.0.1 wants to connect");
    SUCCEED();
}

TEST(CooperationTaskDialogTest, SwitchInfomationPageDoesNotCrash)
{
    CooperationTaskDialog dlg;
    dlg.switchInfomationPage("Info", "transfer finished");
    SUCCEED();
}

// 多次切换页面,覆盖 switchLayout->setCurrentIndex 不同分支。
TEST(CooperationTaskDialogTest, SequentialPageSwitchesDoNotCrash)
{
    CooperationTaskDialog dlg;
    dlg.switchWaitPage("W");
    dlg.switchFailPage("F", "msg", true);
    dlg.switchConfirmPage("C", "msg");
    dlg.switchInfomationPage("I", "msg");
    dlg.switchWaitPage("W2");
    SUCCEED();
}
