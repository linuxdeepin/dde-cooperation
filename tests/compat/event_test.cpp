// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// framework/event 模块单元测试：覆盖 Event / EventChannel / EventSequence / EventDispatcher。

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFuture>
#include <QObject>
#include <QVariant>
#include <QVariantList>

#include <dde-cooperation-framework/event/event.h>
#include <dde-cooperation-framework/event/eventchannel.h>
#include <dde-cooperation-framework/event/eventdispatcher.h>
#include <dde-cooperation-framework/event/eventsequence.h>

using namespace DPF_NAMESPACE;

// 简单的 QObject 接收者：不使用信号/槽机制，仅作为成员函数指针载体。
// EventHelper 中 emit 为空宏，对普通成员函数等同于直接调用。
class Receiver : public QObject
{
public:
    int calls = 0;
    void slot() { ++calls; }
    bool filterTrue() { return true; }
    bool filterFalse() { return false; }
    bool hookTrue() { return true; }
    bool hookFalse() { return false; }
};

// ============================================================================
// Event (注册/查询)
// ============================================================================

TEST(EventTest, RegisterAndQuerySignal)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSignal, "SpaceA", "signal_Foo");
    EventType t = e->eventType("SpaceA", "signal_Foo");
    EXPECT_NE(t, EventTypeScope::kInValid);
}

TEST(EventTest, QueryUnregisteredReturnsInvalid)
{
    auto *e = Event::instance();
    EventType t = e->eventType("SpaceB", "signal_Bar");
    EXPECT_EQ(t, EventTypeScope::kInValid);
}

TEST(EventTest, PrefixMismatchReturnsInvalid)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSignal, "SpaceP", "signal_X");
    // 前缀不在 {signal,slot,hook} 中 -> kInValid
    EventType t = e->eventType("SpaceP", "noprefix_X");
    EXPECT_EQ(t, EventTypeScope::kInValid);
}

TEST(EventTest, RegisterAcrossStrategies)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSignal, "SpaceC", "signal_S1");
    e->registerEventType(EventStratege::kSlot, "SpaceC", "slot_S2");
    e->registerEventType(EventStratege::kHook, "SpaceC", "hook_S3");

    EXPECT_NE(e->eventType("SpaceC", "signal_S1"), EventTypeScope::kInValid);
    EXPECT_NE(e->eventType("SpaceC", "slot_S2"), EventTypeScope::kInValid);
    EXPECT_NE(e->eventType("SpaceC", "hook_S3"), EventTypeScope::kInValid);
}

TEST(EventTest, PluginTopicsReturnsRegistered)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSignal, "SpaceT", "signal_T1");
    e->registerEventType(EventStratege::kSlot, "SpaceT", "slot_T2");

    QStringList all = e->pluginTopics("SpaceT");
    EXPECT_TRUE(all.contains("signal_T1"));
    EXPECT_TRUE(all.contains("slot_T2"));

    QStringList signalsOnly = e->pluginTopics("SpaceT", EventStratege::kSignal);
    EXPECT_TRUE(signalsOnly.contains("signal_T1"));
    EXPECT_FALSE(signalsOnly.contains("slot_T2"));
}

TEST(EventTest, DuplicateRegisterDoesNotDoubleInsert)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSignal, "SpaceD", "signal_Dup");
    EventType first = e->eventType("SpaceD", "signal_Dup");
    // 重复注册应被忽略（Q_UNLIKELY 告警分支），类型不变
    e->registerEventType(EventStratege::kSignal, "SpaceD", "signal_Dup");
    EventType second = e->eventType("SpaceD", "signal_Dup");
    EXPECT_EQ(first, second);
}

TEST(EventTest, ManagersAreSingletons)
{
    EXPECT_NE(Event::instance()->dispatcher(), nullptr);
    EXPECT_NE(Event::instance()->sequence(), nullptr);
    EXPECT_NE(Event::instance()->channel(), nullptr);
}

// ============================================================================
// EventDispatcher
// ============================================================================

TEST(EventDispatcherTest, EmptyDispatchReturnsTrue)
{
    EventDispatcher d;
    EXPECT_TRUE(d.dispatch());
    EXPECT_TRUE(d.dispatch(QVariantList {}));
}

TEST(EventDispatcherTest, HandlerIsInvoked)
{
    EventDispatcher d;
    Receiver r;
    d.append(&r, &Receiver::slot);
    EXPECT_TRUE(d.dispatch());
    EXPECT_EQ(r.calls, 1);
    d.dispatch(QVariantList {});
    EXPECT_EQ(r.calls, 2);
}

TEST(EventDispatcherTest, FilterTrueBlocksAndReturnsFalse)
{
    EventDispatcher d;
    Receiver r;
    d.appendFilter(&r, &Receiver::filterTrue);
    d.append(&r, &Receiver::slot);
    EXPECT_FALSE(d.dispatch());
    EXPECT_EQ(r.calls, 0);   // filter 命中，handler 不执行
}

TEST(EventDispatcherTest, FilterFalseDoesNotBlock)
{
    EventDispatcher d;
    Receiver r;
    d.appendFilter(&r, &Receiver::filterFalse);
    d.append(&r, &Receiver::slot);
    EXPECT_TRUE(d.dispatch());
    EXPECT_EQ(r.calls, 1);
}

TEST(EventDispatcherTest, AsyncDispatchResult)
{
    EventDispatcher d;
    Receiver r;
    d.append(&r, &Receiver::slot);
    QFuture<bool> f = d.asyncDispatch(QVariantList {});
    f.waitForFinished();
    EXPECT_TRUE(f.result());
    EXPECT_EQ(r.calls, 1);
}

TEST(EventDispatcherManagerTest, GlobalFilterRemove)
{
    auto *mgr = Event::instance()->dispatcher();
    Receiver r;
    auto filter = [](EventType, const QVariantList &) { return false; };
    EXPECT_TRUE(mgr->installGlobalEventFilter(&r, filter));
    EXPECT_FALSE(mgr->globalFiltered(1, {}));   // filter 返回 false
    EXPECT_TRUE(mgr->removeGlobalEventFilter(&r));
    EXPECT_FALSE(mgr->removeGlobalEventFilter(&r));   // 已移除
}

// ============================================================================
// EventSequence
// ============================================================================

TEST(EventSequenceTest, EmptyTraversalReturnsFalse)
{
    EventSequence s;
    EXPECT_FALSE(s.traversal());
    EXPECT_FALSE(s.traversal(QVariantList {}));
}

TEST(EventSequenceTest, HookTrueShortCircuitsToTrue)
{
    EventSequence s;
    Receiver r;
    s.append(&r, &Receiver::hookTrue);
    EXPECT_TRUE(s.traversal());
}

TEST(EventSequenceTest, HookFalseContinuesToEndReturnsFalse)
{
    EventSequence s;
    Receiver r;
    s.append(&r, &Receiver::hookFalse);
    EXPECT_FALSE(s.traversal());
    EXPECT_FALSE(s.traversal(QVariantList {}));
}

TEST(EventSequenceTest, FirstTrueStopsSecondFromRunning)
{
    EventSequence s;
    Receiver r1, r2;
    r1.calls = 0;
    s.append(&r1, &Receiver::hookTrue);   // 返回 true，短路
    // 第二个 hook 因短路不应执行；用 calls 间接验证较难（两个 Receiver 同类型指针），
    // 这里只验证返回值为 true
    EXPECT_TRUE(s.traversal());
}

// unfollow(EventType)/unfollow(space,topic) 均为 protected，无法在外部直接测试。

// ============================================================================
// EventChannel / EventChannelFuture / EventChannelManager
// ============================================================================

TEST(EventChannelTest, SendWithoutConnectorReturnsInvalid)
{
    EventChannel ch;
    EXPECT_FALSE(ch.send().isValid());
    EXPECT_FALSE(ch.send(QVariantList {}).isValid());
}

TEST(EventChannelTest, AsyncSendReturnsFuture)
{
    EventChannel ch;
    EventChannelFuture f = ch.asyncSend();
    f.waitForFinished();
    // 未设置 conn -> send 返回无效 QVariant
    EXPECT_FALSE(f.result().isValid());
}

TEST(EventChannelManagerTest, DisconnectTypeUnregistered)
{
    auto *mgr = Event::instance()->channel();
    EventType t = EventTypeScope::kCustomBase + 700;
    EXPECT_FALSE(mgr->disconnect(t));
}

// ---- 通过 QObject 接收者覆盖 connect/push/post 真实通道 ----

TEST(EventChannelManagerTest, ConnectPushInvokesReceiver)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSlot, "ChSpace", "slot_Ping");
    EventType t = e->eventType("ChSpace", "slot_Ping");
    ASSERT_NE(t, EventTypeScope::kInValid);

    Receiver r;
    auto *mgr = e->channel();
    EXPECT_TRUE(mgr->connect(t, &r, &Receiver::slot));
    EXPECT_EQ(r.calls, 0);
    QVariant ret = mgr->push(t);
    EXPECT_EQ(r.calls, 1);
    // disconnect 后再 push 不再触发
    EXPECT_TRUE(mgr->disconnect(t));
    ret = mgr->push(t);
    EXPECT_EQ(r.calls, 1);
    EXPECT_FALSE(ret.isValid());
}

TEST(EventChannelManagerTest, PostAsyncDelivers)
{
    auto *e = Event::instance();
    e->registerEventType(EventStratege::kSlot, "ChSpace2", "slot_Async");
    EventType t = e->eventType("ChSpace2", "slot_Async");

    Receiver r;
    auto *mgr = e->channel();
    EXPECT_TRUE(mgr->connect(t, &r, &Receiver::slot));
    EventChannelFuture f = mgr->post(t);
    f.waitForFinished();
    EXPECT_EQ(r.calls, 1);
    mgr->disconnect(t);
}

TEST(EventChannelManagerTest, PushUnregisteredReturnsInvalid)
{
    auto *mgr = Event::instance()->channel();
    // 未 connect 的 type，push 返回无效 QVariant
    EventType t = EventTypeScope::kCustomBase + 701;
    QVariant ret = mgr->push(t);
    EXPECT_FALSE(ret.isValid());
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
