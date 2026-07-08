// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// MainWindow 构造 (覆盖 mainwindow.cpp + mainwindow_p_linux.cpp 的 initWindow/initWidgets/moveCenter)
// + 各 widget 公有方法深挖 (themeChanged/nextPage/backPage/clear/addResult/setErrorType 等)
// 网络/IPC 调用经 NetworkUtil stub 安全; tryConnect 等会 emit 信号, stub 不实际连网。

#include "gui/mainwindow.h"
#include "gui/transfer/transferringwidget.h"
#include "gui/connect/connectwidget.h"
#include "gui/connect/readywidget.h"
#include "gui/connect/choosewidget.h"
#include "gui/transfer/resultdisplay.h"
#include "gui/transfer/errorwidget.h"
#include "gui/transfer/waittransferwidget.h"
#include "gui/backupload/uploadfilewidget.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QDialog>
#include <QMainWindow>
#include <QTimer>
#include <QTest>

using namespace data_transfer_core;

TEST(MainWindowTest, ConstructShowDestroy)
{
    MainWindow *w = new MainWindow();
    w->show();
    QTest::qWait(20);
    w->repaint();
    w->hide();
    // 故意不 delete: MainWindowPrivate::initWidgets 用 lambda 捕获 this 连接
    // TransferHelper::changeWidget; Qt 对 functor 连接无法在 receiver 析构时自动断开,
    // 析构后留下悬空连接, 后续任何 nextPage()/backPage()/cancel() emit changeWidget
    // 都会 UAF 崩溃 (见 mainwindow_p_linux.cpp:170)。保持 MainWindow 存活让连接有效。
    // 测试二进制内存泄漏无害 (process exit 即回收)。
}

// ---- TransferringWidget 公有方法 ----
TEST(TransferringDeepTest, AllPublicMethods)
{
    TransferringWidget *w = new TransferringWidget();
    w->updateProcess("Transfering", "file.txt", 50, 30);
    w->updateProcess("Transfering", "file.txt", 100, 0);
    w->changeTimeLabel("00:30");
    w->changeProgressLabel(75);
    w->themeChanged(0);
    w->themeChanged(1);
    w->updateInformationPage();
    w->resetContent("type", "content");
    w->getTransferFileName("/home/uos/a.txt", "/tmp/");
    w->clear();
    delete w;
}

// ProgressBarLabel 构造 + setProgress + paintEvent
TEST(TransferringDeepTest, ProgressBarLabel)
{
    ProgressBarLabel *p = new ProgressBarLabel();
    p->setProgress(50);
    p->resize(100, 10);
    p->show();
    QTest::qWait(15);
    p->repaint();
    delete p;
}

// ProcessWindow updateContent/changeTheme
TEST(TransferringDeepTest, ProcessWindowMethods)
{
    ProcessWindow *pw = new ProcessWindow();
    pw->updateContent("name", "type");
    pw->changeTheme(0);
    pw->changeTheme(1);
    delete pw;
}

// ---- ConnectWidget ----
TEST(ConnectDeepTest, PublicMethods)
{
    ConnectWidget *w = new ConnectWidget();
    // 注: ctor 已调用 initConnectLayout, 二次调用会导致内部 widget 双重父子化引发崩溃
    w->themeChanged(0);
    w->themeChanged(1);
    w->nextPage();
    w->backPage();
    delete w;
}

// ---- ReadyWidget ----
TEST(ReadyDeepTest, PublicMethods)
{
    ReadyWidget *w = new ReadyWidget();
    w->onLineTextChange();
    w->setnextButEnable(true);
    w->setnextButEnable(false);
    w->connectFailed();
    w->nextPage();
    w->backPage();
    w->clear();
    delete w;
}

// ---- ChooseWidget ----
TEST(ChooseDeepTest, PublicMethods)
{
    ChooseWidget *w = new ChooseWidget();
    w->themeChanged(0);
    w->themeChanged(1);
    w->nextPage();
    delete w;
}

// ---- ResultDisplayWidget ----
TEST(ResultDeepTest, PublicMethods)
{
    ResultDisplayWidget *w = new ResultDisplayWidget();
    w->addResult("app1", true, "ok");
    w->addResult("app2", false, "fail");
    w->setStatus(true);
    w->setStatus(false);
    w->themeChanged(0);
    w->themeChanged(1);
    w->nextPage();
    w->clear();
    delete w;
}

// ---- ErrorWidget ----
TEST(ErrorDeepTest, PublicMethods)
{
    ErrorWidget *w = new ErrorWidget();
    w->setErrorType(networkError, 5);
    w->setErrorType(outOfStorageError);
    w->setErrorType(noError);
    w->checkNetworkAndUpdate();
    w->themeChanged(0);
    w->themeChanged(1);
    w->backPage();
    w->retryPage();
    delete w;
}

// ---- WaitTransferWidget ----
// 注: cancel() 在无 activeWindow 时 activeMainWindow->pos() 空指针解引用 (真实 bug),
// 且调 DDialog::exec() 阻塞。这里单独构造带活动窗口 + auto-reject 的用例覆盖。
TEST(WaitDeepTest, PublicMethods)
{
    WaitTransferWidget *w = new WaitTransferWidget();
    w->themeChanged(0);
    w->themeChanged(1);
    w->nextPage();
    w->backPage();
    delete w;
}

TEST(WaitDeepTest, CancelWithActiveWindow)
{
    QMainWindow *host = new QMainWindow();
    host->show();
    host->activateWindow();
    QTest::qWait(10);
    WaitTransferWidget *w = new WaitTransferWidget();
    QTimer::singleShot(0, []() {
        if (auto *dlg = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
            dlg->reject();   // auto-reject DDialog::exec()
    });
    w->cancel();   // code != 1 → 不走 disconnectRemote
    delete w;
    delete host;
}

// ---- UploadFileWidget (构造即可, Initial 为 private slot) ----
TEST(UploadDeepTest, Construct)
{
    UploadFileWidget *w = new UploadFileWidget();
    w->show();
    QTest::qWait(15);
    delete w;
}
