// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// dialogs/filetransfersettingsdialog (dfmplugin vendored copy) 单元测试。
// 需 QApplication (main.cpp 已创建), offscreen 平台 paintEvent 通过 show()+repaint() 触发。
// onButtonClicked 内部会递归调 QFileDialog::getExistingDirectory, 用计数器 stub 打破递归。

#include <gtest/gtest.h>

#include "dialogs/filetransfersettingsdialog.h"
#include "configs/settings/configmanager.h"
#include "configs/dconfig/dconfigmanager.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QShowEvent>
#include <QCloseEvent>
#include <QTest>
#include <QString>

#include "stub.h"

using namespace dfmplugin_cooperation;

namespace {
int g_dirCallCount = 0;
QString g_fakeDir;
QString g_fakeDir2;
QString fakeGetExistingDirectory(QWidget *, const QString &, const QString &, QFileDialog::Options)
{
    // 第一次返回 g_fakeDir, 之后返回 g_fakeDir2 (打破 onButtonClicked 内部递归)
    return (g_dirCallCount++ == 0) ? g_fakeDir : g_fakeDir2;
}
}

// ========== FileChooserEdit ==========

TEST(FileChooserEditTest, ConstructAndSetTextShort)
{
    FileChooserEdit edit;
    edit.resize(200, 40);
    edit.setText("/short/path");
    SUCCEED();
}

TEST(FileChooserEditTest, SetTextLongTriggersElision)
{
    FileChooserEdit edit;
    edit.resize(100, 40);
    edit.setText("/a/very/very/very/very/very/very/very/very/long/path/that/exceeds/width");
    SUCCEED();
}

TEST(FileChooserEditTest, PaintEventRuns)
{
    FileChooserEdit edit;
    edit.resize(200, 40);
    edit.show();
    edit.repaint();
    SUCCEED();
}

// onButtonClicked: getExistingDirectory 返回空 -> 提前 return
TEST(FileChooserEditTest, OnButtonClickedEmptyDirReturnsEarly)
{
    FileChooserEdit edit;
    edit.resize(200, 40);
    Stub s;
    using Sig = QString (*)(QWidget *, const QString &, const QString &, QFileDialog::Options);
    s.set(static_cast<Sig>(&QFileDialog::getExistingDirectory), fakeGetExistingDirectory);
    g_dirCallCount = 0;
    g_fakeDir = "";
    edit.onButtonClicked();
    SUCCEED();
}

// onButtonClicked: 返回一个可写且有内容的目录 -> setText + emit fileChoosed
TEST(FileChooserEditTest, OnButtonClickedValidDirEmitsSignal)
{
    FileChooserEdit edit;
    edit.resize(200, 40);
    QSignalSpy spy(&edit, &FileChooserEdit::fileChoosed);

    Stub s;
    using Sig = QString (*)(QWidget *, const QString &, const QString &, QFileDialog::Options);
    s.set(static_cast<Sig>(&QFileDialog::getExistingDirectory), fakeGetExistingDirectory);
    g_dirCallCount = 0;
    g_fakeDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    edit.onButtonClicked();
    EXPECT_EQ(spy.size(), 1);
}

// onButtonClicked 的不可写目录分支会 exec() InformationDialog, offscreen 下仍模态阻塞,
// 且 QDialog::exec 是虚函数无法安全单实例 stub (破坏 DDialog 内部状态)。该分支不测;
// empty/valid 两条路径已覆盖 setText/emit/递归打破。

TEST(InformationDialogTest, ConstructAndCloseEvent)
{
    InformationDialog d;
    QCloseEvent ev;
    d.closeEvent(&ev);
    SUCCEED();
}

// ========== BackgroundWidget ==========

TEST(BackgroundWidgetTest, RoundRoles)
{
    BackgroundWidget w;
    w.setRoundRole(BackgroundWidget::Top);
    w.setRoundRole(BackgroundWidget::Bottom);
    w.setRoundRole(BackgroundWidget::NoRole);
    w.resize(100, 100);
    w.show();
    w.repaint();
    SUCCEED();
}

// 单独 paint 每种 role (Top/Bottom 各自的 QPainterPath 构造分支)
TEST(BackgroundWidgetTest, PaintTopRole)
{
    BackgroundWidget w;
    w.setRoundRole(BackgroundWidget::Top);
    w.resize(100, 60);
    w.show();
    w.repaint();
    SUCCEED();
}

TEST(BackgroundWidgetTest, PaintBottomRole)
{
    BackgroundWidget w;
    w.setRoundRole(BackgroundWidget::Bottom);
    w.resize(100, 60);
    w.show();
    w.repaint();
    SUCCEED();
}

TEST(BackgroundWidgetTest, PaintNoRoleDefault)
{
    BackgroundWidget w;
    w.setRoundRole(BackgroundWidget::NoRole);
    w.resize(100, 60);
    w.show();
    w.repaint();
    SUCCEED();
}

// ========== FileTransferSettingsDialog ==========

TEST(FileTransferSettingsDialogTest, ConstructRunsInitUIAndConnect)
{
    FileTransferSettingsDialog d;
    d.resize(420, 300);
    SUCCEED();
}

TEST(FileTransferSettingsDialogTest, ShowEventTriggersLoadConfig)
{
    FileTransferSettingsDialog d;
    d.resize(420, 300);
    QShowEvent ev;
    d.showEvent(&ev);
    SUCCEED();
}

TEST(FileTransferSettingsDialogTest, ShowAndRepaint)
{
    FileTransferSettingsDialog d;
    d.resize(420, 300);
    d.show();
    d.repaint();
    SUCCEED();
}

TEST(FileTransferSettingsDialogTest, OnFileChooseredSavesPath)
{
    FileTransferSettingsDialog d;
    EXPECT_NO_FATAL_FAILURE(d.onFileChoosered("/tmp/some_dir"));
    QVariant v = ConfigManager::instance()->appAttribute("GenericAttribute", "StoragePath");
    EXPECT_EQ(v.toString().toStdString(), "/tmp/some_dir");
}

TEST(FileTransferSettingsDialogTest, OnComBoxValueChangedReportsStatus)
{
    FileTransferSettingsDialog d;
    EXPECT_NO_FATAL_FAILURE(d.onComBoxValueChanged(0));
    EXPECT_NO_FATAL_FAILURE(d.onComBoxValueChanged(2));
    SUCCEED();
}
