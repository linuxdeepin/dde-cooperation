// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QQueue>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

#include <dde-cooperation-framework/lifecycle/lifecycle.h>
#include <dde-cooperation-framework/lifecycle/plugin.h>
#include <dde-cooperation-framework/lifecycle/plugindepend.h>
#include <dde-cooperation-framework/lifecycle/pluginmanager.h>
#include <dde-cooperation-framework/lifecycle/pluginmetaobject.h>

#include "pluginmanager_p.h"
#include "pluginmetaobject_p.h"

DPF_USE_NAMESPACE

namespace {
class DummyPlugin : public Plugin {
public:
    bool start() override { return true; }
};
}   // namespace

TEST(PluginBaseTest, InitializeAndStopAreSafe)
{
    DummyPlugin p;
    EXPECT_NO_THROW(p.initialize());
    EXPECT_NO_THROW(p.stop());
}

TEST(PluginMetaObjectExtraTest, UncoveredDefaultGetters)
{
    PluginMetaObject obj;
    EXPECT_TRUE(obj.description().isEmpty());
    EXPECT_TRUE(obj.category().isEmpty());
    EXPECT_TRUE(obj.urlLink().isEmpty());
    EXPECT_TRUE(obj.depends().isEmpty());
    EXPECT_EQ(obj.pluginState(), PluginMetaObject::kInvalid);
    EXPECT_TRUE(obj.plugin().isNull());
}

TEST(PluginMetaObjectExtraTest, DebugStreamForMetaObjectAndPointer)
{
    PluginMetaObject obj;
    QString captured;
    {
        QDebug dbg(&captured);
        dbg << obj;
    }
    EXPECT_FALSE(captured.isEmpty());

    PluginMetaObjectPointer ptr = PluginMetaObjectPointer::create();
    QString capturedPtr;
    {
        QDebug dbg(&capturedPtr);
        dbg << ptr;
    }
    EXPECT_FALSE(capturedPtr.isEmpty());
}

TEST(PluginManagerPrivateJsonToMetaTest, FillsAllFieldsAndDepends)
{
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    QJsonObject metaData;
    metaData.insert(kPluginVersion, "1.2.3");
    metaData.insert(kPluginCategory, "util");
    metaData.insert(kPluginDescription, "a plugin");
    metaData.insert(kPluginUrlLink, "http://example.com");
    QJsonArray deps;
    QJsonObject d1;
    d1.insert(kPluginName, "core");
    d1.insert(kPluginVersion, "1.0");
    deps.append(d1);
    metaData.insert(kPluginDepends, deps);

    PluginManagerPrivate::jsonToMeta(meta, metaData);

    EXPECT_EQ(meta->version(), "1.2.3");
    EXPECT_EQ(meta->category(), "util");
    EXPECT_EQ(meta->description(), "a plugin");
    EXPECT_EQ(meta->urlLink(), "http://example.com");
    ASSERT_EQ(meta->depends().size(), 1);
    EXPECT_EQ(meta->depends().at(0).name(), "core");
    EXPECT_EQ(meta->depends().at(0).version(), "1.0");
    EXPECT_EQ(meta->pluginState(), PluginMetaObject::kReaded);
}

TEST(PluginManagerPrivateJsonToMetaTest, EmptyJsonYieldsDefaultsButReaded)
{
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    PluginManagerPrivate::jsonToMeta(meta, QJsonObject{});
    EXPECT_TRUE(meta->version().isEmpty());
    EXPECT_TRUE(meta->category().isEmpty());
    EXPECT_TRUE(meta->description().isEmpty());
    EXPECT_TRUE(meta->urlLink().isEmpty());
    EXPECT_TRUE(meta->depends().isEmpty());
    EXPECT_EQ(meta->pluginState(), PluginMetaObject::kReaded);
}

TEST(PluginManagerPrivateJsonToMetaTest, MultipleDependsParsed)
{
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    QJsonObject metaData;
    QJsonArray deps;
    QJsonObject d1;
    d1.insert(kPluginName, "core");
    d1.insert(kPluginVersion, "1.0");
    QJsonObject d2;
    d2.insert(kPluginName, "network");
    d2.insert(kPluginVersion, "2.3");
    deps.append(d1);
    deps.append(d2);
    metaData.insert(kPluginDepends, deps);

    PluginManagerPrivate::jsonToMeta(meta, metaData);

    ASSERT_EQ(meta->depends().size(), 2);
    EXPECT_EQ(meta->depends().at(0).name(), "core");
    EXPECT_EQ(meta->depends().at(1).name(), "network");
    EXPECT_EQ(meta->depends().at(1).version(), "2.3");
}

TEST(PluginManagerPrivateScanTest, ScanfRealPluginAppendsAndSetsName)
{
    QQueue<PluginMetaObjectPointer> dest;
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    QJsonObject data;
    data.insert(kPluginName, "myplugin");

    PluginManagerPrivate::scanfRealPlugin(&dest, meta, data, {});

    ASSERT_EQ(dest.size(), 1);
    EXPECT_EQ(dest.head()->name(), "myplugin");
    EXPECT_FALSE(dest.head()->isVirtual());
    EXPECT_EQ(dest.head()->pluginState(), PluginMetaObject::kReaded);
}

TEST(PluginManagerPrivateScanTest, ScanfRealPluginBlacklistedSkipped)
{
    QQueue<PluginMetaObjectPointer> dest;
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    QJsonObject data;
    data.insert(kPluginName, "evil");

    PluginManagerPrivate::scanfRealPlugin(&dest, meta, data, {"evil"});

    EXPECT_TRUE(dest.isEmpty());
}

TEST(PluginManagerPrivateScanTest, ScanfRealPluginEmptyNameStillAppended)
{
    QQueue<PluginMetaObjectPointer> dest;
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();

    PluginManagerPrivate::scanfRealPlugin(&dest, meta, QJsonObject{}, {});

    ASSERT_EQ(dest.size(), 1);
    EXPECT_TRUE(dest.head()->name().isEmpty());
}

TEST(PluginManagerPrivateScanTest, ScanfVirtualPluginAddsEachVirtual)
{
    QQueue<PluginMetaObjectPointer> dest;
    QJsonObject metaDataJson;
    metaDataJson.insert(kPluginName, "host");
    QJsonArray virtualList;
    QJsonObject v1;
    v1.insert(kPluginName, "v-one");
    QJsonObject v2;
    v2.insert(kPluginName, "v-two");
    virtualList.append(v1);
    virtualList.append(v2);
    QJsonObject data;
    data.insert(kVirtualPluginMeta, metaDataJson);
    data.insert(kVirtualPluginList, virtualList);

    PluginManagerPrivate::scanfVirtualPlugin(&dest, "/path/to/host.so", data, {});

    ASSERT_EQ(dest.size(), 2);
    EXPECT_EQ(dest.at(0)->name(), "v-one");
    EXPECT_EQ(dest.at(1)->name(), "v-two");
    EXPECT_TRUE(dest.at(0)->isVirtual());
    EXPECT_TRUE(dest.at(1)->isVirtual());
}

TEST(PluginManagerPrivateScanTest, ScanfVirtualPluginBlacklistedRealSkipsAll)
{
    QQueue<PluginMetaObjectPointer> dest;
    QJsonObject metaDataJson;
    metaDataJson.insert(kPluginName, "host");
    QJsonArray virtualList;
    QJsonObject v1;
    v1.insert(kPluginName, "v-one");
    virtualList.append(v1);
    QJsonObject data;
    data.insert(kVirtualPluginMeta, metaDataJson);
    data.insert(kVirtualPluginList, virtualList);

    PluginManagerPrivate::scanfVirtualPlugin(&dest, "/host.so", data, {"host"});

    EXPECT_TRUE(dest.isEmpty());
}

TEST(PluginManagerPrivateScanTest, ScanfVirtualPluginBlacklistedSingleVirtualSkipped)
{
    QQueue<PluginMetaObjectPointer> dest;
    QJsonObject metaDataJson;
    metaDataJson.insert(kPluginName, "host");
    QJsonArray virtualList;
    QJsonObject v1;
    v1.insert(kPluginName, "v-keep");
    QJsonObject v2;
    v2.insert(kPluginName, "v-drop");
    virtualList.append(v1);
    virtualList.append(v2);
    QJsonObject data;
    data.insert(kVirtualPluginMeta, metaDataJson);
    data.insert(kVirtualPluginList, virtualList);

    PluginManagerPrivate::scanfVirtualPlugin(&dest, "/host.so", data, {"v-drop"});

    ASSERT_EQ(dest.size(), 1);
    EXPECT_EQ(dest.at(0)->name(), "v-keep");
}

TEST(PluginManagerPrivateScanTest, ScanfVirtualPluginEmptyListAppendsNothing)
{
    QQueue<PluginMetaObjectPointer> dest;
    QJsonObject metaDataJson;
    metaDataJson.insert(kPluginName, "host");
    QJsonObject data;
    data.insert(kVirtualPluginMeta, metaDataJson);
    data.insert(kVirtualPluginList, QJsonArray{});

    PluginManagerPrivate::scanfVirtualPlugin(&dest, "/host.so", data, {});

    EXPECT_TRUE(dest.isEmpty());
}

namespace {
PluginMetaObjectPointer makeNamedPlugin(const QString &name)
{
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    QQueue<PluginMetaObjectPointer> tmp;
    QJsonObject data;
    data.insert(kPluginName, name);
    PluginManagerPrivate::scanfRealPlugin(&tmp, meta, data, {});
    return meta;
}

void addDepend(const PluginMetaObjectPointer &meta, const QString &depName, const QString &depVer)
{
    QJsonObject metaData;
    QJsonArray deps;
    QJsonObject d;
    d.insert(kPluginName, depName);
    d.insert(kPluginVersion, depVer);
    deps.append(d);
    metaData.insert(kPluginDepends, deps);
    PluginManagerPrivate::jsonToMeta(meta, metaData);
}
}   // namespace

TEST(PluginManagerPrivateSortTest, DependsSortPutsDependencyFirst)
{
    auto a = makeNamedPlugin("A");
    auto b = makeNamedPlugin("B");
    addDepend(b, "A", "1.0");
    ASSERT_EQ(b->depends().size(), 1);

    QQueue<PluginMetaObjectPointer> src;
    src.append(b);
    src.append(a);

    QQueue<PluginMetaObjectPointer> dst;
    PluginManagerPrivate::dependsSort(&dst, &src);

    ASSERT_EQ(dst.size(), 2);
    EXPECT_EQ(dst.at(0)->name(), "A");
    EXPECT_EQ(dst.at(1)->name(), "B");
}

TEST(PluginManagerPrivateSortTest, DependsSortChainOfThree)
{
    auto a = makeNamedPlugin("A");
    auto b = makeNamedPlugin("B");
    auto c = makeNamedPlugin("C");
    addDepend(b, "A", "1.0");
    addDepend(c, "B", "1.0");

    QQueue<PluginMetaObjectPointer> src;
    src.append(c);
    src.append(b);
    src.append(a);

    QQueue<PluginMetaObjectPointer> dst;
    PluginManagerPrivate::dependsSort(&dst, &src);

    ASSERT_EQ(dst.size(), 3);
    EXPECT_EQ(dst.at(0)->name(), "A");
    EXPECT_EQ(dst.at(1)->name(), "B");
    EXPECT_EQ(dst.at(2)->name(), "C");
}

TEST(PluginManagerPrivateSortTest, DependsSortNoDependsKeepsAll)
{
    auto a = makeNamedPlugin("A");
    auto b = makeNamedPlugin("B");

    QQueue<PluginMetaObjectPointer> src;
    src.append(a);
    src.append(b);

    QQueue<PluginMetaObjectPointer> dst;
    PluginManagerPrivate::dependsSort(&dst, &src);

    ASSERT_EQ(dst.size(), 2);
}

TEST(PluginManagerPrivateSortTest, CircularDependsFallsBackToSourceOrder)
{
    auto a = makeNamedPlugin("A");
    auto b = makeNamedPlugin("B");
    addDepend(a, "B", "1.0");
    addDepend(b, "A", "1.0");

    QQueue<PluginMetaObjectPointer> src;
    src.append(a);
    src.append(b);

    QQueue<PluginMetaObjectPointer> dst;
    PluginManagerPrivate::dependsSort(&dst, &src);

    ASSERT_EQ(dst.size(), 2);
    EXPECT_EQ(dst.at(0)->name(), src.at(0)->name());
    EXPECT_EQ(dst.at(1)->name(), src.at(1)->name());
}

TEST(PluginManagerPrivateSortTest, UnknownDependIgnoredAndOthersSorted)
{
    auto a = makeNamedPlugin("A");
    auto b = makeNamedPlugin("B");
    addDepend(b, "A", "1.0");
    addDepend(b, "Ghost", "9.9");

    QQueue<PluginMetaObjectPointer> src;
    src.append(b);
    src.append(a);

    QQueue<PluginMetaObjectPointer> dst;
    PluginManagerPrivate::dependsSort(&dst, &src);

    ASSERT_EQ(dst.size(), 2);
    EXPECT_EQ(dst.at(0)->name(), "A");
    EXPECT_EQ(dst.at(1)->name(), "B");
}

TEST(PluginManagerPrivateReadJsonTest, EmptyMetaDataReturnsEarlyAndStaysReading)
{
    PluginMetaObjectPointer meta = PluginMetaObjectPointer::create();
    PluginManagerPrivate::readJsonToMeta(meta);
    EXPECT_EQ(meta->pluginState(), PluginMetaObject::kReading);
    EXPECT_TRUE(meta->iid().isEmpty());
}

TEST(PluginManagerPrivateSinglePluginTest, InvalidStateLoadInitStartStopReturnFalse)
{
    PluginManager pm;
    PluginManagerPrivate p(&pm);
    PluginMetaObjectPointer ptr = PluginMetaObjectPointer::create();
    EXPECT_EQ(ptr->pluginState(), PluginMetaObject::kInvalid);
    EXPECT_FALSE(p.loadPlugin(ptr));
    EXPECT_FALSE(p.initPlugin(ptr));
    EXPECT_FALSE(p.startPlugin(ptr));
    EXPECT_FALSE(p.stopPlugin(ptr));
}

TEST(PluginManagerPrivateSinglePluginTest, PluginMetaObjMissOnEmptyQueueReturnsNull)
{
    PluginManager pm;
    PluginManagerPrivate p(&pm);
    EXPECT_EQ(p.pluginMetaObj("anything"), nullptr);
}

TEST(LifeCycleNamespaceTest, InitializeRecordsIidsAndPaths)
{
    LifeCycle::shutdownPlugins();
    LifeCycle::initialize({"org.test.Lifecycle.IID"}, {"/tmp/no-such-lifecycle-path"});
    EXPECT_TRUE(LifeCycle::pluginIIDs().contains("org.test.Lifecycle.IID"));
    EXPECT_TRUE(LifeCycle::pluginPaths().contains("/tmp/no-such-lifecycle-path"));
}

TEST(LifeCycleNamespaceTest, InitializeWithBlackAndLazyNames)
{
    LifeCycle::shutdownPlugins();
    LifeCycle::initialize({"org.test.Lifecycle.IID2"},
                          {"/tmp/no-such-lifecycle-path-2"},
                          {"black-plugin-x"},
                          {"lazy-plugin-y"});
    EXPECT_TRUE(LifeCycle::blackList().contains("black-plugin-x"));
    EXPECT_TRUE(LifeCycle::lazyLoadList().contains("lazy-plugin-y"));
}

TEST(LifeCycleNamespaceTest, PluginMetaObjLookupMissReturnsNull)
{
    EXPECT_EQ(LifeCycle::pluginMetaObj("does-not-exist"), nullptr);
}

TEST(LifeCycleNamespaceTest, ReadPluginsOnEmptyIsSafe)
{
    EXPECT_NO_THROW((void)LifeCycle::readPlugins());
}

TEST(LifeCycleNamespaceTest, LoadAndShutdownOnEmptyIsSafe)
{
    LifeCycle::initialize({"org.test.Lifecycle.Load"}, {"/no/plugins/here"});
    EXPECT_NO_THROW((void)LifeCycle::loadPlugins());
    EXPECT_NO_THROW(LifeCycle::shutdownPlugins());
    SUCCEED();
}

TEST(LifeCycleNamespaceTest, StateQueriesAreSafe)
{
    EXPECT_NO_THROW((void)LifeCycle::isAllPluginsInitialized());
    EXPECT_NO_THROW((void)LifeCycle::isAllPluginsStarted());
}
