// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// TransferUtil 静态方法覆盖 (避免触发 instance() 的网络监控定时器)
// generateRandomNumber / DownLoadDir / tempCacheDir / getJsonfile /
// getRemainSize / isUnfinishedJob / checkSize / getTransferJson

#include "utils/transferutil.h"
#include "utils/optionsmanager.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>

TEST(TransferUtilTest, GenerateRandomNumberIs6Digits)
{
    QString r = TransferUtil::generateRandomNumber();
    EXPECT_EQ(r.size(), 6);
    for (QChar c : r) {
        EXPECT_TRUE(c.isDigit());
    }
}

TEST(TransferUtilTest, DownLoadDir)
{
    QString complete = TransferUtil::DownLoadDir(true);
    EXPECT_TRUE(complete.endsWith("deepin-data-transfer-temp/"));
    QString relative = TransferUtil::DownLoadDir(false);
    EXPECT_EQ(relative, "deepin-data-transfer-temp");
}

TEST(TransferUtilTest, TempCacheDirExists)
{
    QString dir = TransferUtil::tempCacheDir();
    EXPECT_TRUE(QDir(dir).exists());
}

TEST(TransferUtilTest, GetRemainSizeNonNegative)
{
    int r = TransferUtil::getRemainSize();
    EXPECT_GE(r, 0);
}

TEST(TransferUtilTest, GetJsonfileDefaultPath)
{
    QJsonObject obj;
    obj["k"] = "v";
    QString path = TransferUtil::getJsonfile(obj, QString());
    EXPECT_EQ(path, QString("./transfer.json"));
    EXPECT_TRUE(QFileInfo::exists(path));
    QFile::remove(path);
}

TEST(TransferUtilTest, GetJsonfileCustomSave)
{
    QString tmp = QDir::tempPath();
    QJsonObject obj;
    obj["a"] = 1;
    QString path = TransferUtil::getJsonfile(obj, tmp);
    EXPECT_EQ(path, tmp + "/transfer.json");
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    EXPECT_EQ(doc.object().value("a").toInt(), 1);
    QFile::remove(path);
}

TEST(TransferUtilTest, IsUnfinishedJobNoFile)
{
    QString content;
    bool r = TransferUtil::isUnfinishedJob(content, "1.2.3.4_nonexistent");
    EXPECT_FALSE(r);
}

TEST(TransferUtilTest, IsUnfinishedJobWithFile)
{
    QString ip = "9.9.9.9";
    QString cacheDir = TransferUtil::tempCacheDir();
    QString path = cacheDir + ip + "transfer-temp.json";
    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("{\"job\":\"test\"}");
        f.close();
    }
    QString content;
    bool r = TransferUtil::isUnfinishedJob(content, ip);
    EXPECT_TRUE(r);
    EXPECT_EQ(content.toStdString(), "{\"job\":\"test\"}");
    QFile::remove(path);
}

// ParseJson 返回空 → false
TEST(TransferUtilTest, CheckSizeEmptyJson)
{
    EXPECT_FALSE(TransferUtil::checkSize("/nonexistent/file.json"));
}

// 小 user_data → size 不足 → true (size < remain)
TEST(TransferUtilTest, CheckSizeSmall)
{
    QString tmp = QDir::tempPath() + "/dt_checksize_small.json";
    {
        QFile f(tmp);
        f.open(QIODevice::WriteOnly);
        f.write("{\"user_data\":\"1\"}");
        f.close();
    }
    EXPECT_TRUE(TransferUtil::checkSize(tmp));
    QFile::remove(tmp);
}

// 巨大 user_data → size >= remain → 触发 outOfStorage 路径 (stub 后 cancel/disconnect 安全)
TEST(TransferUtilTest, CheckSizeHugeTriggersOutOfStorage)
{
    QString tmp = QDir::tempPath() + "/dt_checksize_huge.json";
    {
        QFile f(tmp);
        f.open(QIODevice::WriteOnly);
        // 99999999999 GB → size 必然 >= remain
        f.write("{\"user_data\":\"99999999999999999\"}");
        f.close();
    }
    EXPECT_FALSE(TransferUtil::checkSize(tmp));
    QFile::remove(tmp);
}

TEST(TransferUtilTest, GetTransferJsonWritesFile)
{
    OptionsManager::instance()->clear();
    OptionsManager::instance()->addUserOption(Options::KSelectFileSize, QStringList{"1"});

    QString tmpDir = QDir::tempPath() + "/dt_transferjson_dir";
    QDir().mkpath(tmpDir);

    QStringList apps{"app1", "app2"};
    QStringList files{QDir::homePath() + "/doc.txt", "/etc/hosts"};
    QStringList browsers{"firefox"};
    QString path = TransferUtil::getTransferJson(apps, files, browsers,
                                                 "bookmarks", "wallpaper", tmpDir);
    EXPECT_TRUE(path.endsWith("transfer.json"));
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    f.close();
    EXPECT_EQ(obj.value("user_data").toString().toStdString(), "1");
    EXPECT_EQ(obj.value("app").toArray().size(), 2);
    EXPECT_EQ(obj.value("browsersName").toArray().size(), 1);
    EXPECT_EQ(obj.value("wallpapers").toString().toStdString(), "wallpaper");
    // home 路径已被替换
    EXPECT_FALSE(obj.value("user_file").toArray().at(0).toString().contains(QDir::homePath()));
    QFile::remove(path);
    QDir().rmdir(tmpDir);
}
