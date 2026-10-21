// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "net/linux/noticeutil.h"

#include <QSignalSpy>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

using namespace cooperation_core;

namespace {

class FakeNoticeUtil : public NoticeUtil
{
public:
    struct NotifyCall
    {
        uint replacesId { 0 };
        QString body;
        int timeout { 0 };
    };

    QVector<NotifyCall> calls;
    QVector<uint> closed;
    uint nextId { 1 };

protected:
    uint sendNotify(const QString &title, const QString &body, const QStringList &actions,
                    const QVariantMap &hints, int expireTimeout, uint replacesId) override
    {
        Q_UNUSED(title)
        Q_UNUSED(actions)
        Q_UNUSED(hints)
        calls.append(NotifyCall { replacesId, body, expireTimeout });
        // Mimics org.freedesktop.Notifications: replacing an existing id
        // returns that same id, a new notification gets a fresh one.
        return replacesId == 0 ? nextId++ : replacesId;
    }

    void sendClose(uint notificationId) override
    {
        closed.append(notificationId);
    }
};

const QVariantMap kHiddenHints { { "x-deepin-ShowInNotifyCenter", false } };

}   // namespace

TEST(NoticeUtilTest, RequestAfterDismissalStartsNewBubble)
{
    FakeNoticeUtil util;

    util.notifyMessage("File transfer", "request 1", { "accept" }, kHiddenHints, 10000);
    ASSERT_EQ(1, util.calls.size());
    EXPECT_EQ(0u, util.calls[0].replacesId);

    // Progress updates are the only notifications allowed to replace a bubble.
    util.notifyMessage("File transfer", "progress", { "cancel" }, kHiddenHints, 15000, true);
    ASSERT_EQ(2, util.calls.size());
    EXPECT_EQ(1u, util.calls[1].replacesId);

    // Acting on the request dismisses the notification.
    ASSERT_TRUE(QMetaObject::invokeMethod(&util, "onActionTriggered",
                                          Q_ARG(uint, 1), Q_ARG(QString, QStringLiteral("accept"))));

    // The next request is a new notification, not an update of the dismissed
    // one. Some daemons never present an update for an id they already closed.
    util.notifyMessage("File transfer", "request 2", { "accept" }, kHiddenHints, 10000);
    ASSERT_EQ(3, util.calls.size());
    EXPECT_EQ(0u, util.calls[2].replacesId);
}

TEST(NoticeUtilTest, CloseReleasesNotificationId)
{
    FakeNoticeUtil util;

    util.notifyMessage("File transfer", "request", { "accept" }, kHiddenHints, 10000);
    util.closeNotification();
    ASSERT_EQ(1, util.closed.size());
    EXPECT_EQ(1u, util.closed[0]);

    util.notifyMessage("File transfer", "another request", { "accept" }, kHiddenHints, 10000);
    ASSERT_EQ(2, util.calls.size());
    EXPECT_EQ(0u, util.calls[1].replacesId);
}

TEST(NoticeUtilTest, ActionFromAnotherNotificationIsIgnored)
{
    FakeNoticeUtil util;
    QSignalSpy spy(&util, &NoticeUtil::ActionInvoked);

    util.notifyMessage("File transfer", "request", { "accept" }, kHiddenHints, 10000);
    ASSERT_TRUE(QMetaObject::invokeMethod(&util, "onActionTriggered",
                                          Q_ARG(uint, 99), Q_ARG(QString, QStringLiteral("reject"))));
    EXPECT_EQ(0, spy.count());

    // The notification is still on screen, so progress may still replace it.
    util.notifyMessage("File transfer", "progress", { "cancel" }, kHiddenHints, 15000, true);
    ASSERT_EQ(2, util.calls.size());
    EXPECT_EQ(1u, util.calls[1].replacesId);
}
