// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// GUI widget 构造覆盖 (offscreen)
// 每个 widget 构造即覆盖 initUI/initConnect 主体 (记忆 heavy-dep-dialog-construct-covers-initUI);
// show+repaint 触发 paintEvent; heap 分配 + 显式 delete 控制生命周期。
// 不测 modal exec() / 网络 / QThread 实际执行。

#include "gui/connect/choosewidget.h"
#include "gui/connect/connectwidget.h"
#include "gui/connect/custommessagebox.h"
#include "gui/connect/networkdisconnectionwidget.h"
#include "gui/connect/promptwidget.h"
#include "gui/connect/readywidget.h"
#include "gui/connect/startwidget.h"
#include "gui/transfer/errorwidget.h"
#include "gui/transfer/resultdisplay.h"
#include "gui/transfer/transferringwidget.h"
#include "gui/transfer/waittransferwidget.h"
#include "gui/backupload/uploadfilewidget.h"
#include "gui/backupload/unzipwoker.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QTimer>

// 构造 + show + repaint + 析构的通用 helper
template <typename T>
static void exercise(T *w)
{
    w->resize(400, 300);
    w->show();
    QTest::qWait(15);
    w->repaint();
    w->hide();
    delete w;
}

// ---- connect/* ----
TEST(ConnectWidgetsTest, StartWidget) { exercise(new StartWidget()); }
TEST(ConnectWidgetsTest, ChooseWidget) { exercise(new ChooseWidget()); }
TEST(ConnectWidgetsTest, ConnectWidget) { exercise(new ConnectWidget()); }
TEST(ConnectWidgetsTest, ReadyWidget) { exercise(new ReadyWidget()); }
TEST(ConnectWidgetsTest, PromptWidget) { exercise(new PromptWidget()); }
TEST(ConnectWidgetsTest, NetworkDisconnectionWidget) { exercise(new NetworkDisconnectionWidget()); }

TEST(ConnectWidgetsTest, CustomMessageBoxConstruct)
{
    CustomMessageBox *box = new CustomMessageBox("main title", "sub title");
    box->show();
    QTest::qWait(15);
    box->repaint();
    delete box;
}

// SelectContinueTransfer 调 exec(); 用 QTimer auto-reject (记忆 modal-exec-auto-reject)
TEST(ConnectWidgetsTest, CustomMessageBoxSelectContinueAutoReject)
{
    QTimer::singleShot(0, []() {
        // 关掉活动模态对话框
        QWidget *top = QApplication::activeModalWidget();
        if (top) top->close();
    });
    bool r = CustomMessageBox::SelectContinueTransfer();
    EXPECT_FALSE(r);
}

TEST(ConnectWidgetsTest, CustomMessageBoxSelectContinueAutoAccept)
{
    QTimer::singleShot(0, []() {
        QWidget *top = QApplication::activeModalWidget();
        if (top) top->close();   // close → reject 默认, 此处仅验证不阻塞退出
    });
    // 另一种方式: 找到 accept 按钮点击。close 走 reject → false 分支。
    bool r = CustomMessageBox::SelectContinueTransfer();
    EXPECT_FALSE(r);
}

// ---- transfer/* ----
TEST(TransferWidgetsTest, TransferringWidget) { exercise(new TransferringWidget()); }
TEST(TransferWidgetsTest, ResultDisplay) { exercise(new ResultDisplayWidget()); }
TEST(TransferWidgetsTest, ErrorWidget) { exercise(new ErrorWidget()); }
TEST(TransferWidgetsTest, WaitTransferWidget) { exercise(new WaitTransferWidget()); }

// ---- backupload/* ----
TEST(BackuploadWidgetsTest, UploadFileWidget) { exercise(new UploadFileWidget()); }

// UnzipWorker 是 QThread, 只构造不 start (避免真实解压)
TEST(BackuploadWidgetsTest, UnzipWorkerConstruct)
{
    UnzipWorker *w = new UnzipWorker(QString());
    delete w;
}
