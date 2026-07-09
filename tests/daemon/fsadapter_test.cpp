// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/fsadapter.h"
#include "common/commonstruct.h"
#include "utils/utils.h"

#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QDir>

#include <fcntl.h>
#include <unistd.h>

// getFileEntry: valid file
TEST(FSAdapterTest, GetFileEntryValidFile)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("test.txt");
    QFile f(fpath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("hello", 5);
    f.close();

    FileEntry *entry = new FileEntry();
    int ret = FSAdapter::getFileEntry(fpath.toLocal8Bit().constData(), &entry);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(entry->filetype, FileType::FILE_B);
    EXPECT_FALSE(entry->hidden);
    delete entry;
}

TEST(FSAdapterTest, GetFileEntryHiddenFile)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath(".hiddenfile");
    QFile f(fpath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();

    FileEntry *entry = new FileEntry();
    int ret = FSAdapter::getFileEntry(fpath.toLocal8Bit().constData(), &entry);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(entry->hidden);
    delete entry;
}

TEST(FSAdapterTest, GetFileEntryDirectory)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString dpath = dir.filePath("subdir");
    QDir().mkpath(dpath);

    FileEntry *entry = new FileEntry();
    int ret = FSAdapter::getFileEntry(dpath.toLocal8Bit().constData(), &entry);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(entry->filetype, FileType::DIR);
    delete entry;
}

TEST(FSAdapterTest, GetFileEntryNonExistent)
{
    FileEntry *entry = new FileEntry();
    int ret = FSAdapter::getFileEntry("/nonexistent/path/xyz", &entry);
    EXPECT_EQ(ret, -1);
    delete entry;
}

TEST(FSAdapterTest, NewFileCreatesRegularFile)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("newfile");
    bool ret = FSAdapter::newFileByFullPath(fpath.toLocal8Bit().constData(), false);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(QFile::exists(fpath));
}

TEST(FSAdapterTest, NewFileCreatesDirectory)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString dpath = dir.filePath("newdir");
    bool ret = FSAdapter::newFileByFullPath(dpath.toLocal8Bit().constData(), true);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(QDir(dpath).exists());
}

TEST(FSAdapterTest, NewFileByFullPathNestedDir)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString dpath = dir.filePath("a/b/c");
    bool ret = FSAdapter::newFileByFullPath(dpath.toLocal8Bit().constData(), true);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(QDir(dpath).exists());
}

TEST(FSAdapterTest, NewFileCreatesRegularFileRelative)
{
    // newFile uses DaemonConfig::getStorageDir() as base, not cwd
    bool ret = FSAdapter::newFile("rel_test_file_xyz.txt", false);
    EXPECT_TRUE(ret);
}

TEST(FSAdapterTest, NewFileCreatesDirRelative)
{
    bool ret = FSAdapter::newFile("rel_test_dir_xyz", true);
    EXPECT_TRUE(ret);
}
TEST(FSAdapterTest, WriteBlockSimple)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb.txt");
    // create file first
    ASSERT_TRUE(FSAdapter::newFileByFullPath(fpath.toLocal8Bit().constData(), false));
    const char data[] = "testdata";
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, data, 8);
    EXPECT_TRUE(ret);
}

TEST(FSAdapterTest, WriteBlockWithOffset)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb2.txt");
    ASSERT_TRUE(FSAdapter::newFileByFullPath(fpath.toLocal8Bit().constData(), false));
    const char data[] = "ABCDEFGH";
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 100, data, 8);
    EXPECT_TRUE(ret);
}

TEST(FSAdapterTest, WriteBlockCreateFlag)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb3.txt");
    fs::file *fx = nullptr;
    const char data[] = "newdata";
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, data, 7, JobTransFileOp::FIlE_CREATE, &fx);
    EXPECT_TRUE(ret);
}

TEST(FSAdapterTest, ReacquirePathNonExistentReturnsFalse)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("notexist.txt");
    fastring newpath;
    bool ret = FSAdapter::reacquirePath(fpath.toLocal8Bit().constData(), &newpath);
    // file doesn't exist -> returns false
    EXPECT_FALSE(ret);
}

TEST(FSAdapterTest, ReacquirePathExisting)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("exist.txt");
    QFile f(fpath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    fastring newpath;
    bool ret = FSAdapter::reacquirePath(fpath.toLocal8Bit().constData(), &newpath);
    // file exists, should rename -> newpath contains "(1)"
    EXPECT_TRUE(ret);
    EXPECT_TRUE(newpath.find("(1)") != fastring::npos);
}

// ---- Extended coverage: writeBlock branches ----

TEST(FSAdapterTest, WriteBlockFileNotExist)
{
    bool ret = FSAdapter::writeBlock("/nonexistent/xyz/file", 0, "data", 4);
    EXPECT_FALSE(ret);
}

TEST(FSAdapterTest, WriteBlockFlagsCreateFxNotNull)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb_fx.txt");
    fs::file *fx = new fs::file(fpath.toLocal8Bit().constData(), 'm');
    // FIlE_CREATE flag but fx already set -> error path
    const char data[] = "x";
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, data, 1,
                                     JobTransFileOp::FIlE_CREATE, &fx);
    EXPECT_FALSE(ret);
    // fx was deleted inside
}

TEST(FSAdapterTest, WriteBlockFlagsCreateFxNullSizeZero)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb_create.txt");
    fs::file *fx = nullptr;
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, "", 0,
                                     JobTransFileOp::FIlE_CREATE, &fx);
    EXPECT_TRUE(ret);
    // fx now set to the new file
    if (fx) { fx->close(); delete fx; }
}

TEST(FSAdapterTest, WriteBlockFlagsCreateWithDataAndClose)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb_close.txt");
    fs::file *fx = nullptr;
    const char data[] = "hello";
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, data, 5,
                                     JobTransFileOp::FIlE_CREATE | JobTransFileOp::FILE_CLOSE, &fx);
    EXPECT_TRUE(ret);
    // FILE_CLOSE -> fx deleted inside
}

TEST(FSAdapterTest, WriteBlockFlagsCreateWriteContinue)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb_cont.txt");
    fs::file *fx = nullptr;
    const char data[] = "world";
    // create without close -> fx stays open
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, data, 5,
                                     JobTransFileOp::FIlE_CREATE, &fx);
    EXPECT_TRUE(ret);
    ASSERT_NE(fx, nullptr);
    // second write without create flag, same fx
    bool ret2 = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 5, "more", 4,
                                      JobTransFileOp::FIlE_NONE, &fx);
    EXPECT_TRUE(ret2);
    if (fx) { fx->close(); delete fx; }
}

TEST(FSAdapterTest, WriteBlockFxNullNoCreate)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString fpath = dir.filePath("wb_nullfx.txt");
    fs::file *fx = nullptr;
    // no create flag + null fx -> error
    bool ret = FSAdapter::writeBlock(fpath.toLocal8Bit().constData(), 0, "x", 1,
                                     JobTransFileOp::FIlE_NONE, &fx);
    EXPECT_FALSE(ret);
}

TEST(FSAdapterTest, GetFileEntryNonExistentTemp)
{
    FileEntry *entry = new FileEntry();
    int ret = FSAdapter::getFileEntry("/tmp/nonexistent_daemon_test_xyz_123/file", &entry);
    EXPECT_EQ(ret, -1);
    delete entry;
}
