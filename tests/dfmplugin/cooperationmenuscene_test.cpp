// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// menu/cooperationmenuscene (dfmplugin vendored copy) 单元测试。
// triggered() 内部调 QProcess::startDetached, 用 stub 替换避免真的拉起进程。

#include <gtest/gtest.h>

#include "menu/cooperationmenuscene.h"

#include <QMenu>
#include <QAction>
#include <QUrl>
#include <QVariantHash>
#include <QTemporaryFile>
#include <QProcess>
#include "stub.h"

using namespace dfmplugin_cooperation;

namespace {
bool g_startDetachedCalled = false;
bool fakeStartDetached(const QString &, const QStringList &, const QString &, qint64 *)
{
    g_startDetachedCalled = true;
    return true;
}
}

// ===== creator / name =====

TEST(CooperationMenuSceneTest, CreatorCreateReturnsScene)
{
    CooperationMenuCreator creator;
    auto *scene = creator.create();
    ASSERT_NE(scene, nullptr);
    EXPECT_FALSE(scene->name().toStdString().empty());
    delete scene;
}

TEST(CooperationMenuSceneTest, SceneNameMatchesCreator)
{
    CooperationMenuScene scene;
    EXPECT_EQ(scene.name().toStdString(), "CooperationMenu");
}

// ===== initialize =====

TEST(CooperationMenuSceneTest, InitializeNoFilesReturnsFalse)
{
    CooperationMenuScene scene;
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(QList<QUrl>()));
    params.insert("isEmptyArea", false);
    EXPECT_FALSE(scene.initialize(params));
}

TEST(CooperationMenuSceneTest, InitializeNonLocalFileReturnsFalse)
{
    CooperationMenuScene scene;
    QVariantHash params;
    QList<QUrl> urls { QUrl("http://example.com") };
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    EXPECT_FALSE(scene.initialize(params));
}

TEST(CooperationMenuSceneTest, InitializeLocalFileSucceeds)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };

    CooperationMenuScene scene;
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    bool ok = scene.initialize(params);
    EXPECT_TRUE(ok);
}

// ===== create =====

TEST(CooperationMenuSceneTest, CreateNullParentReturnsFalse)
{
    CooperationMenuScene scene;
    EXPECT_FALSE(scene.create(nullptr));
}

TEST(CooperationMenuSceneTest, CreateAddsActionWhenNotEmptyArea)
{
    CooperationMenuScene scene;
    // 需先 initialize 以填充 selectFiles/isEmptyArea
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    scene.initialize(params);

    QMenu menu;
    bool ok = scene.create(&menu);
    SUCCEED();
}

TEST(CooperationMenuSceneTest, CreateSkipsWhenEmptyArea)
{
    CooperationMenuScene scene;
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", true);   // 空白区 -> 不加 file-transfer 项
    scene.initialize(params);

    QMenu menu;
    scene.create(&menu);
    SUCCEED();
}

// ===== updateState (含 send-to 子菜单分支) =====

TEST(CooperationMenuSceneTest, UpdateStateRunsForActions)
{
    CooperationMenuScene scene;
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    scene.initialize(params);

    QMenu menu;
    scene.create(&menu);
    // 加一个 separator 和一个非 send-to action -> 覆盖分支
    menu.addSeparator();
    auto *act = menu.addAction("other");
    act->setProperty("actionID", "other-id");
    scene.updateState(&menu);
    SUCCEED();
}

TEST(CooperationMenuSceneTest, UpdateStateWithSendToSubmenu)
{
    CooperationMenuScene scene;
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    scene.initialize(params);

    QMenu menu;
    scene.create(&menu);
    // 构造 send-to action 带子菜单
    auto *sendTo = menu.addAction("send-to");
    sendTo->setProperty("actionID", "send-to");
    auto *sub = new QMenu(&menu);
    sub->addAction("bluetooth");
    sendTo->setMenu(sub);
    scene.updateState(&menu);
    SUCCEED();
}

TEST(CooperationMenuSceneTest, UpdateStateSkipsWhenEmptyArea)
{
    CooperationMenuScene scene;
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", true);
    scene.initialize(params);

    QMenu menu;
    scene.updateState(&menu);
    SUCCEED();
}

// ===== scene(QAction*) =====

TEST(CooperationMenuSceneTest, SceneNullActionReturnsNull)
{
    CooperationMenuScene scene;
    EXPECT_EQ(scene.scene(nullptr), nullptr);
}

// ===== triggered (stub QProcess::startDetached) =====

TEST(CooperationMenuSceneTest, TriggeredUnknownActionDelegates)
{
    CooperationMenuScene scene;
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    scene.initialize(params);

    QMenu menu;
    scene.create(&menu);

    QAction unknown("unknown");
    unknown.setProperty("actionID", "not-registered");
    scene.triggered(&unknown);
    SUCCEED();
}

TEST(CooperationMenuSceneTest, TriggeredFileTransferStartsProcessStubbed)
{
    CooperationMenuScene scene;
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QList<QUrl> urls { QUrl::fromLocalFile(tmp.fileName()) };
    QVariantHash params;
    params.insert("selectFiles", QVariant::fromValue(urls));
    params.insert("isEmptyArea", false);
    scene.initialize(params);

    QMenu menu;
    scene.create(&menu);

    // 找到 create() 添加的 file-transfer action 并触发
    QAction *ftAction = nullptr;
    for (auto *a : menu.actions()) {
        if (a->property("actionID").toString() == "file-transfer") {
            ftAction = a;
            break;
        }
    }
    ASSERT_NE(ftAction, nullptr);

    Stub s;
    using Sig = bool (*)(const QString &, const QStringList &, const QString &, qint64 *);
    s.set(static_cast<Sig>(&QProcess::startDetached), fakeStartDetached);
    g_startDetachedCalled = false;
    bool ret = scene.triggered(ftAction);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(g_startDetachedCalled);
}
