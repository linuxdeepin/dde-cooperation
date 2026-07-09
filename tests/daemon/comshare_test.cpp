// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "service/comshare.h"

#include <QSignalSpy>

TEST(ComshareTest, InstanceSingleton)
{
    auto *a = Comshare::instance();
    auto *b = Comshare::instance();
    EXPECT_EQ(a, b);
}

TEST(ComshareTest, UpdateAndReadStatus)
{
    auto *cs = Comshare::instance();
    cs->updateStatus(CURRENT_STATUS_DISCONNECT);
    EXPECT_EQ(cs->currentStatus(), CURRENT_STATUS_DISCONNECT);

    cs->updateStatus(CURRENT_STATUS_TRAN_CONNECT);
    EXPECT_EQ(cs->currentStatus(), CURRENT_STATUS_TRAN_CONNECT);

    cs->updateStatus(CURRENT_STATUS_SHARE_START);
    EXPECT_EQ(cs->currentStatus(), CURRENT_STATUS_SHARE_START);

    // cleanup
    cs->updateStatus(CURRENT_STATUS_DISCONNECT);
}

TEST(ComshareTest, CheckTransCanConnect)
{
    auto *cs = Comshare::instance();
    cs->updateStatus(CURRENT_STATUS_DISCONNECT);
    EXPECT_TRUE(cs->checkTransCanConnect());

    cs->updateStatus(CURRENT_STATUS_SHARE_START);
    // SHARE_START > TRAN_FILE_RCV => true
    EXPECT_TRUE(cs->checkTransCanConnect());

    cs->updateStatus(CURRENT_STATUS_TRAN_CONNECT);
    EXPECT_FALSE(cs->checkTransCanConnect());

    // cleanup
    cs->updateStatus(CURRENT_STATUS_DISCONNECT);
}

TEST(ComshareTest, UpdateComdataAndQuery)
{
    auto *cs = Comshare::instance();
    cs->updateComdata("app1", "targetApp1", "192.168.1.1");
    EXPECT_EQ(cs->targetAppName("app1"), QString("targetApp1"));
    EXPECT_EQ(cs->targetIP("app1"), QString("192.168.1.1"));

    // update to new values
    cs->updateComdata("app1", "targetApp2", "10.0.0.1");
    EXPECT_EQ(cs->targetAppName("app1"), QString("targetApp2"));
    EXPECT_EQ(cs->targetIP("app1"), QString("10.0.0.1"));

    // query nonexistent
    EXPECT_TRUE(cs->targetAppName("nonexistent").isEmpty());
    EXPECT_TRUE(cs->targetIP("nonexistent").isEmpty());
}

TEST(ComshareTest, SearchIpAndCheck)
{
    auto *cs = Comshare::instance();

    cs->searchIp("token1", 1000);
    cs->searchIp("token2", 2000);

    // token1 exists, time=500 <= 1000 -> erased, returns true
    EXPECT_TRUE(cs->checkSearchRes("token1", 500));

    // token1 was erased by previous call, now not found -> false
    EXPECT_FALSE(cs->checkSearchRes("token1", 500));

    // token2 exists, time=3000 > 2000 -> not erased, returns true
    EXPECT_TRUE(cs->checkSearchRes("token2", 3000));

    // nonexistent token
    EXPECT_FALSE(cs->checkSearchRes("nonexistent", 100));
}
