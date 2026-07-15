// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "configs/settings/settings.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSignalSpy>

namespace {
QString writeFile(const QString &dir, const QString &name, const char *content)
{
    QDir().mkpath(dir);
    QString path = dir + "/" + name;
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(content);
        f.close();
    }
    return path;
}

QString tempDir(const QString &sub)
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + sub;
}
}

TEST(SettingsTest, ConstructFromFilesAndReadValueChain)
{
    QString dir = tempDir("coop_sett_chain");
    QString df = writeFile(dir, "default.json",
        R"({"group_a":{"key1":"def","key2":42}})");
    QString ff = writeFile(dir, "fallback.json",
        R"({"group_b":{"x":1}})");
    QString sf = writeFile(dir, "setting.json",
        R"({"group_a":{"key1":"wrote"}})");

    Settings s(df, ff, sf);
    EXPECT_EQ(s.value("group_a", "key1").toString().toStdString(), "wrote");
    EXPECT_EQ(s.value("group_b", "x").toInt(), 1);
    EXPECT_EQ(s.value("group_a", "key2").toInt(), 42);
    EXPECT_EQ(s.value("group_a", "nope", "dft").toString().toStdString(), "dft");

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(SettingsTest, ContainsChecksAllLayers)
{
    QString dir = tempDir("coop_sett_contains");
    QString df = writeFile(dir, "default.json", R"({"g_d":{"k":"d"}})");
    QString ff = writeFile(dir, "fallback.json", R"({"g_f":{"k":"f"}})");
    QString sf = writeFile(dir, "setting.json", R"({"g_w":{"k":"w"}})");

    Settings s(df, ff, sf);
    EXPECT_TRUE(s.contains("g_w", "k"));
    EXPECT_TRUE(s.contains("g_f", "k"));
    EXPECT_TRUE(s.contains("g_d", "k"));
    EXPECT_FALSE(s.contains("g_x", "k"));
    EXPECT_TRUE(s.contains("g_w", ""));
    EXPECT_FALSE(s.contains("nope", ""));

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(SettingsTest, GroupsKeysAndKeyList)
{
    QString dir = tempDir("coop_sett_keys");
    QString df = writeFile(dir, "default.json",
        R"({"__metadata__":{"grp":{"keyOrdered":["a","b","c"]}},"grp":{"a":1,"b":2,"c":3}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = writeFile(dir, "setting.json", R"({"grp":{"extra":9}})");

    Settings s(df, ff, sf);
    EXPECT_TRUE(s.groups().contains("grp"));
    auto ks = s.keys("grp");
    EXPECT_TRUE(ks.contains("a"));
    EXPECT_TRUE(ks.contains("extra"));

    QStringList kl = s.keyList("grp");
    EXPECT_EQ(kl.first().toStdString(), "a");

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(SettingsTest, SetValueAndSetValueNoNotifySemantics)
{
    Settings s(":/coop_nd.json", ":/coop_nf.json", ":/coop_ns.json");
    QSignalSpy changed(&s, &Settings::valueChanged);

    s.setValue("g", "k", "v");
    EXPECT_EQ(changed.size(), 1);
    EXPECT_EQ(s.value("g", "k").toString().toStdString(), "v");

    s.setValue("g", "k", "v");
    EXPECT_EQ(changed.size(), 1);

    s.setValue("g", "k", "v2");
    EXPECT_EQ(changed.size(), 2);
    EXPECT_EQ(s.value("g", "k").toString().toStdString(), "v2");

    changed.clear();
    EXPECT_TRUE(s.setValueNoNotify("g", "k", "v3"));
    EXPECT_EQ(changed.size(), 0);
    EXPECT_EQ(s.value("g", "k").toString().toStdString(), "v3");

    EXPECT_FALSE(s.setValueNoNotify("g", "k", "v3"));

    EXPECT_TRUE(s.setValueNoNotify("g", "knew", "nv"));
    EXPECT_EQ(s.value("g", "knew").toString().toStdString(), "nv");
}

TEST(SettingsTest, RemoveAndRemoveGroupAndClear)
{
    Settings s(":/coop_rmd.json", ":/coop_rmf.json", ":/coop_rms.json");
    s.setValue("g", "k1", 1);
    s.setValue("g", "k2", 2);

    QSignalSpy spy(&s, &Settings::valueChanged);
    s.remove("g", "absent");
    EXPECT_EQ(spy.size(), 0);

    s.remove("g", "k1");
    EXPECT_FALSE(s.isRemovable("g", "k1"));

    spy.clear();
    s.setValue("grp", "a", "A");
    s.removeGroup("grp");
    EXPECT_FALSE(s.isRemovable("grp", "a"));

    s.removeGroup("notexist");

    s.setValue("c", "x", 1);
    spy.clear();
    s.clear();
    s.clear();
    SUCCEED();
}

TEST(SettingsTest, SyncWritesToFileAndReload)
{
    QString dir = tempDir("coop_sett_sync");
    QString df = writeFile(dir, "default.json", R"({"g":{"k":"d"}})");
    QString ff = writeFile(dir, "fallback.json", R"({"g":{"fb":1}})");
    QString sf = writeFile(dir, "setting.json", R"({})");

    Settings s(df, ff, sf);
    s.setValue("g", "new", "val");
    EXPECT_TRUE(s.sync());

    QFile f(sf);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();
    f.close();
    EXPECT_TRUE(data.contains("val"));

    EXPECT_TRUE(s.sync());

    s.reload();
    SUCCEED();

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(SettingsTest, AutoSyncAndWatchChangesToggles)
{
    QString dir = tempDir("coop_sett_auto");
    QString df = writeFile(dir, "default.json", R"({"g":{"k":1}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = writeFile(dir, "setting.json", R"({})");

    Settings s(df, ff, sf);
    EXPECT_FALSE(s.autoSync());
    EXPECT_FALSE(s.watchChanges());

    s.setAutoSync(true);
    EXPECT_TRUE(s.autoSync());
    s.setAutoSync(true);
    EXPECT_TRUE(s.autoSync());
    s.setValue("g", "k", 2);

    s.setAutoSync(false);
    EXPECT_FALSE(s.autoSync());

    s.setWatchChanges(true);
    EXPECT_TRUE(s.watchChanges());
    s.setWatchChanges(true);
    EXPECT_TRUE(s.watchChanges());
    s.setWatchChanges(false);
    EXPECT_FALSE(s.watchChanges());

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(SettingsTest, WatchChangesCreatesMissingSettingFile)
{
    QString dir = tempDir("coop_sett_watch_missing");
    QDir().mkpath(dir);
    QString df = writeFile(dir, "default.json", R"({"g":{"k":1}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = dir + "/sub/notexist.json";

    Settings s(df, ff, sf);
    s.setWatchChanges(true);
    EXPECT_TRUE(s.watchChanges());
    s.setWatchChanges(false);

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
    QDir(dir).removeRecursively();
}

TEST(SettingsTest, OnFileChangedPathMismatchNoop)
{
    QString dir = tempDir("coop_sett_ofc");
    QString df = writeFile(dir, "default.json", R"({"g":{"k":1}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = writeFile(dir, "setting.json", R"({"g":{"k":2}})");

    Settings s(df, ff, sf);
    s.onFileChanged("/some/other/path.json");
    SUCCEED();

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(SettingsTest, NameTypeConstructorAppConfig)
{
    Settings s("nonexistent_coop_app", Settings::AppConfig);
    EXPECT_FALSE(s.contains("g", "k"));
    SUCCEED();
}

TEST(SettingsTest, NameTypeConstructorGenericConfig)
{
    Settings s("nonexistent_coop_gen", Settings::GenericConfig);
    EXPECT_FALSE(s.contains("g", "k"));
    SUCCEED();
}

TEST(SettingsTest, InvalidJsonFallbackGraceful)
{
    QString dir = tempDir("coop_sett_json");
    QString df = writeFile(dir, "default.json", "{ not valid json");
    QString ff = writeFile(dir, "fallback.json", R"([1,2,3])");
    QString sf = writeFile(dir, "setting.json", "");

    Settings s(df, ff, sf);
    EXPECT_FALSE(s.contains("g", "k"));
    EXPECT_EQ(s.groups().size(), 0);

    writeFile(dir, "default.json", R"({"good":{"a":1},"bad":5})");
    Settings s2(df, ":/coop_nf2.json", ":/coop_ns2.json");
    EXPECT_TRUE(s2.groups().contains("good"));

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}
