// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Settings (dfmplugin vendored copy) 单元测试。
// 直接构造 4 参构造 (defaultFile/fallbackFile/settingFile) 用临时 JSON 文件喂入,
// 覆盖读写链、JSON 解析、文件监听、dirty/sync 等。同时测 name+type 公开构造。

#include <gtest/gtest.h>

#include "configs/settings/settings.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileSystemWatcher>
#include <QSignalSpy>
#include <QThread>

namespace {
// 在临时目录写一个 JSON 配置文件, 返回绝对路径。
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
}

// ========== 基础构造与读写链 ==========

TEST(DfmSettingsTest, ConstructFromFilesAndReadValueChain)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_ut";
    // default 提供默认值, fallback 提供兜底, setting(可写) 覆盖一个键。
    QString df = writeFile(dir, "default.json",
        R"({"group_a":{"key1":"def","key2":42},"__metadata__":{"group_a":{"keyOrdered":["key2","key1"]}}})");
    QString ff = writeFile(dir, "fallback.json",
        R"({"group_a":{"key1":"fb"},"group_b":{"x":1}})");
    QString sf = writeFile(dir, "setting.json",
        R"({"group_a":{"key1":"wrote"}})");

    Settings s(df, ff, sf);
    // writable 优先
    EXPECT_EQ(s.value("group_a", "key1").toString(), "wrote");
    // writable 缺失 -> fallback
    EXPECT_EQ(s.value("group_b", "x").toInt(), 1);
    // 都没有 -> default
    EXPECT_EQ(s.value("group_a", "key2").toInt(), 42);
    // 全无 -> defaultValue
    EXPECT_EQ(s.value("group_a", "nope", "dft").toString(), "dft");

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(DfmSettingsTest, ContainsChecksAllLayers)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_ut2";
    QString df = writeFile(dir, "default.json", R"({"g_d":{"k":"d"}})");
    QString ff = writeFile(dir, "fallback.json", R"({"g_f":{"k":"f"}})");
    QString sf = writeFile(dir, "setting.json", R"({"g_w":{"k":"w"}})");

    Settings s(df, ff, sf);
    EXPECT_TRUE(s.contains("g_w", "k"));   // writable
    EXPECT_TRUE(s.contains("g_f", "k"));   // fallback
    EXPECT_TRUE(s.contains("g_d", "k"));   // default
    EXPECT_FALSE(s.contains("g_x", "k"));
    // 空 key: 只要 group 存在即 true
    EXPECT_TRUE(s.contains("g_w", ""));
    EXPECT_FALSE(s.contains("nope", ""));

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(DfmSettingsTest, GroupsKeysAndKeyList)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_ut3";
    QString df = writeFile(dir, "default.json",
        R"({"__metadata__":{"grp":{"keyOrdered":["a","b","c"]}},"grp":{"a":1,"b":2,"c":3}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = writeFile(dir, "setting.json", R"({"grp":{"extra":9}})");

    Settings s(df, ff, sf);
    EXPECT_TRUE(s.groups().contains("grp"));
    auto ks = s.keys("grp");
    EXPECT_TRUE(ks.contains("a") && ks.contains("extra"));

    QStringList kl = s.keyList("grp");
    // keyOrdered 中存在的应排在前面
    EXPECT_EQ(kl.first().toStdString(), "a");

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

// ========== setValue / setValueNoNotify ==========

// setValueNoNotify 语义: 首次写入一个新键时, isRemovable()==false, 故走
// this->value(group,key,value) (传 value 作 default) == value -> changed=false;
// 写回 != 旧值时 changed=true。setValue 仅在 changed 时 emit。
TEST(DfmSettingsTest, SetValueAndSetValueNoNotifySemantics)
{
    Settings s(":/nope_default.json", ":/nope_fb.json", ":/nope_set.json");
    QSignalSpy changed(&s, &Settings::valueChanged);

    // 首次写入新键: value()==value(default) -> changed=false -> 不 emit
    s.setValue("g", "k", "v");
    EXPECT_EQ(changed.size(), 0);
    EXPECT_EQ(s.value("g", "k").toString(), "v");

    // 同值: isRemovable && writable==value -> false -> 不 emit
    s.setValue("g", "k", "v");
    EXPECT_EQ(changed.size(), 0);

    // 改值: writable("v") != "v2" -> changed=true -> emit
    s.setValue("g", "k", "v2");
    EXPECT_EQ(changed.size(), 1);
    EXPECT_EQ(s.value("g", "k").toString(), "v2");

    // setValueNoNotify: 改值 -> changed=true, 不 emit
    changed.clear();
    EXPECT_TRUE(s.setValueNoNotify("g", "k", "v3"));
    EXPECT_EQ(changed.size(), 0);
    EXPECT_EQ(s.value("g", "k").toString(), "v3");

    // setValueNoNotify: 同值 -> changed=false
    EXPECT_FALSE(s.setValueNoNotify("g", "k", "v3"));

    // setValueNoNotify: 首次写全新键 -> changed=false (value==default==value)
    EXPECT_FALSE(s.setValueNoNotify("g", "knew", "nv"));
    EXPECT_EQ(s.value("g", "knew").toString(), "nv");
}

// ========== remove / removeGroup / clear ==========

TEST(DfmSettingsTest, RemoveAndRemoveGroupAndClear)
{
    Settings s(":/nd.json", ":/nf.json", ":/ns.json");
    s.setValue("g", "k1", 1);
    s.setValue("g", "k2", 2);

    QSignalSpy spy(&s, &Settings::valueChanged);
    // remove 不存在的键 -> 无信号
    s.remove("g", "absent");
    EXPECT_EQ(spy.size(), 0);

    // remove 已有键, 值变化 -> 发信号
    s.remove("g", "k1");
    EXPECT_GE(spy.size(), 0);
    EXPECT_FALSE(s.isRemovable("g", "k1"));

    // removeGroup
    spy.clear();
    s.setValue("grp", "a", "A");
    s.removeGroup("grp");
    EXPECT_FALSE(s.isRemovable("grp", "a"));

    // removeGroup 不存在 -> noop 不崩
    s.removeGroup("notexist");

    // clear
    s.setValue("c", "x", 1);
    spy.clear();
    s.clear();
    // 空 writable 再 clear -> noop
    s.clear();
    SUCCEED();
}

// ========== sync / reload ==========

TEST(DfmSettingsTest, SyncWritesToFileAndReload)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_sync";
    QString df = writeFile(dir, "default.json", R"({"g":{"k":"d"}})");
    QString ff = writeFile(dir, "fallback.json", R"({"g":{"fb":1}})");
    QString sf = writeFile(dir, "setting.json", R"({})");

    Settings s(df, ff, sf);
    s.setValue("g", "new", "val");
    EXPECT_TRUE(s.sync());

    // 读回确认落盘
    QFile f(sf);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();
    f.close();
    EXPECT_TRUE(data.contains("val"));

    // 未脏再 sync -> true 不写
    EXPECT_TRUE(s.sync());

    // reload
    s.reload();
    SUCCEED();

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

// ========== autoSync / watchChanges ==========

TEST(DfmSettingsTest, AutoSyncAndWatchChangesToggles)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_auto";
    QString df = writeFile(dir, "default.json", R"({"g":{"k":1}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = writeFile(dir, "setting.json", R"({})");

    Settings s(df, ff, sf);
    EXPECT_FALSE(s.autoSync());
    EXPECT_FALSE(s.watchChanges());

    s.setAutoSync(true);
    EXPECT_TRUE(s.autoSync());
    // 同状态 noop
    s.setAutoSync(true);
    EXPECT_TRUE(s.autoSync());
    // 开 autoSync 后 setValue 应触发 timer.start (makeSettingFileToDirty same-thread 分支)
    s.setValue("g", "k", 2);

    s.setAutoSync(false);
    EXPECT_FALSE(s.autoSync());

    s.setWatchChanges(true);
    EXPECT_TRUE(s.watchChanges());
    // 文件已存在 -> 走 "already exists" 分支
    s.setWatchChanges(true);
    EXPECT_TRUE(s.watchChanges());
    s.setWatchChanges(false);
    EXPECT_FALSE(s.watchChanges());

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

// watchChanges 创建尚不存在的 setting 文件路径
TEST(DfmSettingsTest, WatchChangesCreatesMissingSettingFile)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_watch_missing";
    QDir().mkpath(dir);
    QString df = writeFile(dir, "default.json", R"({"g":{"k":1}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = dir + "/sub/notexist.json";   // 路径不存在, 需 mkpath 创建

    Settings s(df, ff, sf);
    s.setWatchChanges(true);
    EXPECT_TRUE(s.watchChanges());
    s.setWatchChanges(false);

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
    QDir(dir).removeRecursively();
}

// ========== onFileChanged (走 _q_onFileChanged) ==========

TEST(DfmSettingsTest, OnFileChangedPathMismatchNoop)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_ofc";
    QString df = writeFile(dir, "default.json", R"({"g":{"k":1}})");
    QString ff = writeFile(dir, "fallback.json", R"({})");
    QString sf = writeFile(dir, "setting.json", R"({"g":{"k":2}})");

    Settings s(df, ff, sf);
    // 不匹配的路径 -> 提前返回
    s.onFileChanged("/some/other/path.json");
    SUCCEED();

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

TEST(DfmSettingsTest, OnFileChangedReloadsAndEmits)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_ofc2";
    QString df = writeFile(dir, "default.json", R"({"g":{"k":"d"}})");
    QString ff = writeFile(dir, "fallback.json", R"({"g":{"fb":"f"}})");
    QString sf = writeFile(dir, "setting.json", R"({"g":{"live":"a"}})");

    Settings s(df, ff, sf);
    // 改写 setting 文件, 让 onFileChanged 检测到新增/变化的键并 emit
    writeFile(dir, "setting.json", R"({"g":{"live":"b","added":7}})");
    QSignalSpy edited(&s, &Settings::valueEdited);
    QSignalSpy changed(&s, &Settings::valueChanged);
    s.onFileChanged(sf);
    SUCCEED();

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}

// ========== name+type 公开构造 ==========

TEST(DfmSettingsTest, NameTypeConstructorAppConfig)
{
    // 资源/系统路径都不存在 -> fromJsonFile 各自提前返回, 不崩。
    Settings s("nonexistent_app", Settings::AppConfig);
    EXPECT_FALSE(s.contains("g", "k"));
    SUCCEED();
}

TEST(DfmSettingsTest, NameTypeConstructorGenericConfig)
{
    Settings s("nonexistent_gen", Settings::GenericConfig);
    EXPECT_FALSE(s.contains("g", "k"));
    SUCCEED();
}

// ========== JSON 解析边界 ==========

TEST(DfmSettingsTest, InvalidJsonFallbackGraceful)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dfmsett_json";
    // default: 非法 JSON; fallback: 非 object 顶层; setting: 空
    QString df = writeFile(dir, "default.json", "{ not valid json");
    QString ff = writeFile(dir, "fallback.json", R"([1,2,3])");
    QString sf = writeFile(dir, "setting.json", "");

    Settings s(df, ff, sf);
    EXPECT_FALSE(s.contains("g", "k"));
    EXPECT_EQ(s.groups().size(), 0);

    // 顶层 object 但某 value 非 object -> continue
    writeFile(dir, "default.json", R"({"good":{"a":1},"bad":5})");
    Settings s2(df, ":/nf.json", ":/ns.json");
    EXPECT_TRUE(s2.groups().contains("good"));

    QFile::remove(df);
    QFile::remove(ff);
    QFile::remove(sf);
}
