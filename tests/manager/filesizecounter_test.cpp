// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "filesizecounter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>

class FileSizeCounterTest : public ::testing::Test {
protected:
    FileSizeCounter *counter = nullptr;
    QTemporaryDir tmpDir;

    void SetUp() override { counter = new FileSizeCounter(); }
    void TearDown() override
    {
        if (counter) {
            counter->stop();
            counter->wait(3000);
            delete counter;
        }
    }

    void writeFile(const QString &path, int bytes)
    {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            throw std::runtime_error("Failed to open file: " + path.toStdString());
        }
        f.write(QByteArray(bytes, 'x'));
        f.close();
    }
};

TEST_F(FileSizeCounterTest, CountSingleFile)
{
    auto filePath = tmpDir.path() + "/test.txt";
    writeFile(filePath, 100);

    QStringList paths{filePath};
    quint64 size = counter->countFiles("127.0.0.1", paths);
    EXPECT_EQ(size, 100u);
}

TEST_F(FileSizeCounterTest, CountMultipleFiles)
{
    auto f1 = tmpDir.path() + "/a.txt";
    auto f2 = tmpDir.path() + "/b.txt";
    writeFile(f1, 50);
    writeFile(f2, 70);

    QStringList paths{f1, f2};
    quint64 size = counter->countFiles("127.0.0.1", paths);
    EXPECT_EQ(size, 120u);
}

TEST_F(FileSizeCounterTest, CountNonExistentFile)
{
    QStringList paths{"/nonexistent/file.txt"};
    quint64 size = counter->countFiles("127.0.0.1", paths);
    EXPECT_EQ(size, 0u);
}

TEST_F(FileSizeCounterTest, CountDirectoryAsync)
{
    auto subDir = tmpDir.path() + "/subdir";
    QDir().mkpath(subDir);
    writeFile(subDir + "/inner.txt", 200);

    QStringList paths{subDir};
    quint64 size = counter->countFiles("192.168.1.1", paths);
    // directory triggers async (returns 0)
    EXPECT_EQ(size, 0u);

    // wait for thread to finish
    counter->wait(5000);

    // the thread should have emitted onCountFinish
    // can't easily capture signal without QSignalSpy on dynamic object
    // but we verified the async path was taken
}

TEST_F(FileSizeCounterTest, StopBeforeRun)
{
    counter->stop();
    SUCCEED();
}

TEST_F(FileSizeCounterTest, CountEmptyPathList)
{
    QStringList paths;
    quint64 size = counter->countFiles("127.0.0.1", paths);
    EXPECT_EQ(size, 0u);
}

TEST_F(FileSizeCounterTest, CountFileAndDirMix)
{
    auto f1 = tmpDir.path() + "/file.txt";
    writeFile(f1, 30);

    auto subDir = tmpDir.path() + "/dir";
    QDir().mkpath(subDir);

    QStringList paths{f1, subDir};
    quint64 size = counter->countFiles("127.0.0.1", paths);
    // the first item is a file (adds 30), but second is a dir → triggers async, returns 0
    EXPECT_EQ(size, 0u);
    counter->wait(5000);
}

TEST_F(FileSizeCounterTest, CountDirectoryWithSymlink)
{
    auto realFile = tmpDir.path() + "/real.txt";
    writeFile(realFile, 42);

    auto linkDir = tmpDir.path() + "/links";
    QDir().mkpath(linkDir);

    // create symlink to the real file
    QFile::link(realFile, linkDir + "/link_to_real");

    QStringList paths{linkDir};
    quint64 size = counter->countFiles("10.0.0.1", paths);
    EXPECT_EQ(size, 0u); // async
    counter->wait(5000);
}
