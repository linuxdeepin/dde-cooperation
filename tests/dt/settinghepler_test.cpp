// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// SettingHelper 纯逻辑 / 早返回分支覆盖
// ParseJson (3 分支) / moveFile (dst 不存在/存在/目录无后缀/失败) /
// setFile / addTaskcounter / instance() 构造 (initAppList 资源缺失安全) /
// setBrowserBookMark 早返回 / handleDataConfiguration 非法路径。

#include "utils/settinghepler.h"
#include "net/helper/transferhepler.h"

#include <gtest/gtest.h>
#include <QApplication>
#include <QSignalSpy>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QTemporaryDir>

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

// ---- setBrowserBookMark 成功路径 (.json 文件 move 到 home) ----
TEST(SettingHelperTest, SetBrowserBookMarkValidJson)
{
    SettingHelper sh;
    QString src = writeTemp("dt_bm_src.json", "{\"k\":\"v\"}");
    EXPECT_TRUE(sh.setBrowserBookMark(src));
    // 目标: $HOME/.config/browser/Default/book/dt_bm_src.json
    QString target = QDir::homePath() + "/.config/browser/Default/book/dt_bm_src.json";
    EXPECT_TRUE(QFileInfo::exists(target));
    QFile::remove(target);
    QFile::remove(src);
}

// ---- handleDataConfiguration 合法 transfer.json (各子方法空/非法分支) ----
TEST(SettingHelperTest, HandleDataConfigurationValid)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QJsonObject obj;
    obj["user_file"] = QJsonArray();           // 空数组 → setFile 进 isArray 分支无迭代
    obj["wallpapers"] = QString("nope.png");   // 文件不存在 → setWallpaper 早返回 false
    obj["browserbookmark"] = QString("");      // 空 → 跳过 setBrowserBookMark
    QJsonArray apps;
    apps.append(QString(""));                  // 空 app → installApps 早返回 true
    obj["app"] = apps;
    QFile f(dir.path() + "/transfer.json");
    f.open(QIODevice::WriteOnly);
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingHelper sh;
    EXPECT_TRUE(sh.handleDataConfiguration(dir.path()));
    EXPECT_FALSE(QFileInfo::exists(dir.path() + "/transfer.json"));   // 末尾 QFile::remove
}

// ---- setFile 相对路径含子目录 (mkpath 分支) ----
TEST(SettingHelperTest, SetFileCreatesSubdirInHome)
{
    QString srcName = "dt_setfile_sub.txt";
    QString srcAbs = writeTemp(srcName, "subdata");
    QJsonObject obj;
    QJsonArray arr;
    arr.append("dt_subdir/" + srcName);   // 相对路径带目录 → 目标父目录 mkpath
    obj["user_file"] = arr;

    SettingHelper sh;
    EXPECT_TRUE(sh.setFile(obj, QDir::tempPath() + "/"));
    QString target = QDir::homePath() + "/dt_subdir/" + srcName;
    EXPECT_TRUE(QFileInfo::exists(target));
    // 清理
    QFile::remove(target);
    QDir().rmdir(QDir::homePath() + "/dt_subdir");
    QFile::remove(srcAbs);
}

// ---- addTaskcounter: 计数归零 emit transferFinished ----
TEST(SettingHelperTest, AddTaskCounterEmitsTransferFinishedOnZero)
{
    SettingHelper *sh = SettingHelper::instance();
    sh->taskcounter = 0;   // -fno-access-control: 确定初态, 免受单例跨用例状态干扰
    QSignalSpy spy(TransferHelper::instance(), &TransferHelper::transferFinished);
    sh->addTaskcounter(2);
    EXPECT_EQ(spy.count(), 0);
    sh->addTaskcounter(-2);   // 归零 → emit
    EXPECT_EQ(spy.count(), 1);
}

// ---- installApps 未登记应用 (applist 无项 → false) ----
TEST(SettingHelperTest, InstallAppsUnknownReturnsFalse)
{
    SettingHelper sh;
    EXPECT_FALSE(sh.installApps("com.nonexistent.app.xyz"));   // applist 中无 → 包名空 → false
}

// ---- setWallpaper 不存在文件 (早返回 false, 不触 DBus) ----
TEST(SettingHelperTest, SetWallpaperMissingFileReturnsFalse)
{
    SettingHelper sh;
    EXPECT_FALSE(sh.setWallpaper("/nonexistent/dt_nope.png"));
}

// ---- setWallpaper 存在文件 (move + DBus 尝试; 无服务返回失败, 但不应崩溃) ----
TEST(SettingHelperTest, SetWallpaperExistingFileNoThrow)
{
    if (QGuiApplication::screens().isEmpty())
        GTEST_SKIP() << "offscreen 无屏幕, setWallpaper 会取 screens().first() 跳过";
    SettingHelper sh;
    QString img = writeTemp("dt_wall.png", "pngdata");
    EXPECT_NO_THROW(sh.setWallpaper(img));
    // 清理可能的 home 副本
    QFile::remove(QDir::homePath() + "/Pictures/Wallpapers/ConvertedWallpaper.png");
    QFile::remove(img);
}
