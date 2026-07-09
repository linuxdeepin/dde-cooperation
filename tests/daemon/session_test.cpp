// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "ipc/session.h"

TEST(SessionTest, ConstructorAndGetName)
{
    Session s("app1", "session123", "signalSlot");
    EXPECT_EQ(s.getName(), QString("app1"));
    EXPECT_EQ(s.getSession(), QString("session123"));
    EXPECT_EQ(s.signal(), QString("signalSlot"));
}

TEST(SessionTest, AliveFlag)
{
    Session s("app", "ses", "sig");
    // _pingOK initialized to true in constructor
    EXPECT_TRUE(s.alive());
}

TEST(SessionTest, AddRemoveJob)
{
    Session s("app", "ses", "sig");

    s.addJob(1);
    s.addJob(2);
    s.addJob(3);
    EXPECT_EQ(s.hasJob(1), 0);
    EXPECT_EQ(s.hasJob(2), 1);
    EXPECT_EQ(s.hasJob(3), 2);
    EXPECT_EQ(s.hasJob(99), -1);

    EXPECT_TRUE(s.removeJob(2));
    EXPECT_EQ(s.hasJob(2), -1);
    EXPECT_EQ(s.hasJob(1), 0);
    EXPECT_EQ(s.hasJob(3), 1);

    // remove non-existent
    EXPECT_FALSE(s.removeJob(99));

    // remove already removed
    EXPECT_FALSE(s.removeJob(2));
}

TEST(SessionTest, AddDuplicateJob)
{
    Session s("app", "ses", "sig");
    s.addJob(5);
    s.addJob(5);
    EXPECT_EQ(s.hasJob(5), 0);

    EXPECT_TRUE(s.removeJob(5));
    // second instance still at index 0
    EXPECT_EQ(s.hasJob(5), 0);
    EXPECT_TRUE(s.removeJob(5));
    EXPECT_FALSE(s.removeJob(5));
}
