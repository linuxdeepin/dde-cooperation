// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// UploadFileWidget / UploadFileFrame / UnzipWorker 静态方法覆盖
// checkBackupFile (无效路径分支) / clear (emit Initial) / nextPage/backPage (stackedWidget null 分支) /
// themeChanged (0/1 + 委托给 frame) / frame drag 事件模拟 / UnzipWorker::isValid/getNumFiles 静态。

#include "gui/backupload/uploadfilewidget.h"
#include "gui/backupload/unzipwoker.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSignalSpy>

// ---- UnzipWorker 静态方法 ----
TEST(UnzipWorkerStaticTest, IsValidNonexistentReturnsFalse)
{
    EXPECT_FALSE(UnzipWorker::isValid("/nonexistent/no.zip"));
}

TEST(UnzipWorkerStaticTest, GetNumFilesNonexistent)
{
    // 不存在的 zip, 返回 0 或负数 (libzip 打开失败)
    int n = UnzipWorker::getNumFiles("/nonexistent/no.zip");
    EXPECT_LE(n, 0);
}

// ---- UploadFileWidget ----
TEST(UploadWidgetTest, CheckBackupFileInvalid)
{
    UploadFileWidget w;
    EXPECT_FALSE(w.checkBackupFile("/nonexistent/backup.zip"));
}

TEST(UploadWidgetTest, ClearEmitsInitial)
{
    UploadFileWidget w;
    QSignalSpy spy(&w, &UploadFileWidget::Initial);
    w.clear();
    EXPECT_EQ(spy.count(), 1);
}

TEST(UploadWidgetTest, PageNavigationNoStackedParent)
{
    UploadFileWidget w;   // 无 QStackedWidget 父 → 走 nullptr 分支 (不崩溃)
    w.nextPage();
    w.backPage();
    SUCCEED();
}

TEST(UploadWidgetTest, ThemeChangedBoth)
{
    UploadFileWidget w;
    w.themeChanged(0);   // dark
    w.themeChanged(1);   // light
    SUCCEED();
}

// ---- UploadFileFrame drag 事件模拟 ----
TEST(UploadFrameTest, DragEnterMoveLeaveDrop)
{
    UploadFileFrame frame;
    frame.resize(200, 100);
    frame.show();
    QTest::qWait(10);

    // dragEnterEvent
    {
        QMimeData *mime = new QMimeData;
        mime->setUrls({QUrl::fromLocalFile("/tmp/test.zip")});
        QDragEnterEvent enter(QPoint(10, 10), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
        qApp->sendEvent(&frame, &enter);
        delete mime;
    }
    // dragMoveEvent
    {
        QMimeData *mime = new QMimeData;
        mime->setUrls({QUrl::fromLocalFile("/tmp/test.zip")});
        QDragMoveEvent move(QPoint(20, 20), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
        qApp->sendEvent(&frame, &move);
        delete mime;
    }
    // dragLeaveEvent
    {
        QDragLeaveEvent leave;
        qApp->sendEvent(&frame, &leave);
    }
    // dropEvent (路径无效 → checkBackupFile false, 但覆盖事件处理逻辑)
    {
        QMimeData *mime = new QMimeData;
        mime->setUrls({QUrl::fromLocalFile("/nonexistent/x.zip")});
        QDropEvent drop(QPoint(10, 10), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
        qApp->sendEvent(&frame, &drop);
        delete mime;
    }
    // frame 公有方法
    frame.themeChanged(0);
    frame.themeChanged(1);
    (void)frame.getZipFilePath();
    frame.hide();
}
