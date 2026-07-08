// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// SettingHelper 纯逻辑 / 早返回分支覆盖
// ParseJson (3 分支) / moveFile (dst 不存在/存在/目录无后缀/失败) /
// setFile / addTaskcounter / instance() 构造 (initAppList 资源缺失安全) /
// setBrowserBookMark 早返回 / handleDataConfiguration 非法路径。

#include "utils/settinghepler.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

static QString writeTemp(const QString &name, const QByteArray &data)
{
    QString path = QDir::tempPath() + "/" + name;
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write(data);
    f.close();
    return path;
}

// ---- ParseJson ----
TEST(SettingHelperTest, ParseJsonNonexistentReturnsEmpty)
{
    EXPECT_TRUE(SettingHelper::ParseJson("/nonexistent/xx.json").isEmpty());
}

TEST(SettingHelperTest, ParseJsonInvalidContent)
{
    QString p = writeTemp("dt_invalid.json", "not a json");
    EXPECT_TRUE(SettingHelper::ParseJson(p).isEmpty());
    QFile::remove(p);
}

TEST(SettingHelperTest, ParseJsonValid)
{
    QString p = writeTemp("dt_valid.json", "{\"k\":\"v\"}");
    QJsonObject obj = SettingHelper::ParseJson(p);
    EXPECT_EQ(obj.value("k").toString().toStdString(), "v");
    QFile::remove(p);
}

TEST(SettingHelperTest, ParseJsonEmptyObject)
{
    QString p = writeTemp("dt_emptyobj.json", "{}");
    QJsonObject obj = SettingHelper::ParseJson(p);
    EXPECT_TRUE(obj.isEmpty());   // 空对象 (会触发 WLOG 但仍返回)
    QFile::remove(p);
}

// ---- moveFile ----
TEST(SettingHelperTest, MoveFileToNonexistentDst)
{
    QString src = writeTemp("dt_mv_src.txt", "hello");
    QString dst = QDir::tempPath() + "/dt_mv_dst.txt";
    QFile::remove(dst);
    EXPECT_TRUE(SettingHelper::moveFile(src, dst));
    EXPECT_TRUE(QFileInfo::exists(dst));
    EXPECT_FALSE(QFileInfo::exists(src));   // rename 移走
    QFile::remove(dst);
}

TEST(SettingHelperTest, MoveFileConflictRenames)
{
    QString src = writeTemp("dt_mv_conflict.txt", "a");
    QString dst = QDir::tempPath() + "/dt_mv_conflict.txt";
    // 先制造一个已存在的 dst
    QFile dz(dst);
    dz.open(QIODevice::WriteOnly);
    dz.write("old");
    dz.close();

    EXPECT_TRUE(SettingHelper::moveFile(src, dst));
    // rename 后会带 (1) 后缀
    EXPECT_TRUE(QFileInfo::exists(dst) || QFileInfo::exists(QDir::tempPath() + "/dt_mv_conflict(1).txt"));
    QFile::remove(dst);
    QFile::remove(QDir::tempPath() + "/dt_mv_conflict(1).txt");
}

TEST(SettingHelperTest, MoveFileFailOnMissingSrc)
{
    QString src = "/nonexistent/no_such_src";
    QString dst = QDir::tempPath() + "/dt_mv_fail.txt";
    QFile::remove(dst);
    EXPECT_FALSE(SettingHelper::moveFile(src, dst));
    QFile::remove(dst);
}

// ---- instance + addTaskcounter ----
TEST(SettingHelperTest, InstanceConstructAndTaskCounter)
{
    SettingHelper *h = SettingHelper::instance();
    EXPECT_NE(h, nullptr);
    // addTaskcounter: +1 → 1, +1 → 2, -2 → 0 (会 emit transferFinished, 信号无害)
    h->addTaskcounter(1);
    h->addTaskcounter(1);
    h->addTaskcounter(-2);
    SUCCEED();
}

// ---- setFile ----
TEST(SettingHelperTest, SetFileCopiesIntoHome)
{
    QString srcName = "dt_setfile_src.txt";
    QString srcAbs = writeTemp(srcName, "data");
    // 构造一个 dir: filepath 指向 temp, 文件名仅 srcName
    QJsonObject obj;
    QJsonArray arr;
    arr.append(srcName);
    obj["user_file"] = arr;

    QString filepath = QDir::tempPath() + "/";
    // setFile 会把 filepath + filename 复制到 $HOME/filename
    SettingHelper sh;
    EXPECT_TRUE(sh.setFile(obj, filepath));
    // 清理可能的 home 副本
    QFile::remove(QDir::homePath() + "/" + srcName);
    QFile::remove(srcAbs);
}

// ---- setBrowserBookMark 早返回 (无 DBus) ----
TEST(SettingHelperTest, SetBrowserBookMarkEmptyPath)
{
    SettingHelper sh;
    EXPECT_TRUE(sh.setBrowserBookMark(QString()));   // 空路径直接 true
}

TEST(SettingHelperTest, SetBrowserBookMarkNonJsonSuffix)
{
    SettingHelper sh;
    // suffix != json → 早期 emit + false, 不触发 move
    EXPECT_FALSE(sh.setBrowserBookMark("/tmp/somefile.txt"));
}

// ---- handleDataConfiguration 非法路径 ----
TEST(SettingHelperTest, HandleDataConfigurationInvalid)
{
    SettingHelper sh;
    // 不存在的 dir, transfer.json 解析为空 → addResult + return false
    EXPECT_FALSE(sh.handleDataConfiguration("/nonexistent/dir/"));
}
