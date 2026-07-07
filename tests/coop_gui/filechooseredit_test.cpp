// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include "gui/widgets/filechooseredit.h"
#include "stub.h"

using cooperation_core::FileChooserEdit;

// Stub for QFileDialog::getExistingDirectory (static).
// Signature must match: QString(QWidget*, const QString&, const QString&, QFileDialog::Options)
static QString g_stubDirPath;
static QString stub_getExistingDirectory(QWidget *, const QString &, const QString &, QFileDialog::Options)
{
    return g_stubDirPath;
}

TEST(FileChooserEditTest, SetTextDoesNotCrash)
{
    FileChooserEdit edit;
    edit.setText("/some/long/path/to/show/in/the/edit/control");
    SUCCEED();
}

TEST(FileChooserEditTest, OnButtonClickedEmitsFileChoosedWithStubbedPath)
{
    // Prepare a writable, non-empty temp directory so onButtonClicked's
    // validation (QFileInfo::isWritable + QDir::entryInfoList non-empty) passes.
    QTemporaryDir tempDir;
    ASSERT_TRUE(tempDir.isValid()) << "Failed to create temp dir";
    {
        QTemporaryFile marker(tempDir.path() + "/markerXXXXXX");
        ASSERT_TRUE(marker.open()) << "Failed to create marker file";
        marker.write("x");
        marker.close();
    }
    ASSERT_FALSE(QDir(tempDir.path()).entryInfoList().isEmpty());
    ASSERT_TRUE(QFileInfo(tempDir.path()).isWritable());
    g_stubDirPath = tempDir.path();

    FileChooserEdit edit;
    QSignalSpy spy(&edit, &FileChooserEdit::fileChoosed);
    ASSERT_TRUE(spy.isValid());

    Stub stub;
    // QFileDialog::getExistingDirectory is static; ADDR() yields a function
    // pointer (not a member-function pointer) for static members, which
    // Stub::set accepts. Fall back to a raw void* cast if ADDR doesn't compile.
    stub.set(ADDR(QFileDialog, getExistingDirectory), stub_getExistingDirectory);

    // onButtonClicked is a private slot; invokeMethod resolves it via moc metadata.
    ASSERT_TRUE(QMetaObject::invokeMethod(&edit, "onButtonClicked"));

    // The stub makes the call synchronous; spin the loop briefly to flush signals.
    spy.wait(1000);

    ASSERT_GE(spy.count(), 1) << "fileChoosed signal not emitted";
    auto args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString().toStdString(), g_stubDirPath.toStdString());
}
